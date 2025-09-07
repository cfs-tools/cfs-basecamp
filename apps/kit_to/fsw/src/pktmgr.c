/*
**  Copyright 2022 bitValence, Inc.
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it
**  under the terms of the GNU Affero General Public License
**  as published by the Free Software Foundation; version 3 with
**  attribution addendums as found in the LICENSE.txt
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Affero General Public License for more details.
**
**  Purpose:
**    Manage Packet Table that defines which packets will be sent from
**    the software bus to a socket.
**
**  Notes:
**   1. This has some of the features of a flight app such as packet
**      filtering but it would need design/code reviews to transition it to a
**      flight mission. For starters it uses UDP sockets and it doesn't
**      regulate output bit rates.
**
*/

/*
** Include Files:
*/

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "osapi.h"

#include "cfe_msgids.h"
#include "cfe_config.h"
#include "edslib_datatypedb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include "app_cfg.h"
#include "pktmgr.h"


/******************************/
/** File Function Prototypes **/
/******************************/

static void  ComputeStats(uint16 PktsSent, uint32 BytesSent);
static void  DestructorCallback(void);
static void  FlushTlmPipe(void);
static bool  LoadPktTbl(PKTTBL_Data_t *NewTbl);
static int32 PackEdsOutputMessage(void *DestBuffer, const CFE_MSG_Message_t *SrcBuffer, 
                                  size_t SrcBufferSize, size_t *EdsDataSize);
static int32 SubscribeNewPkt(PKTTBL_Pkt_t *NewPkt);

/**********************/
/** Global File Data **/
/**********************/

static PKTMGR_Class_t *PktMgr = NULL;
static CFE_HDR_TelemetryHeader_PackedBuffer_t SocketBuffer;
static uint16 SocketBufferLen = sizeof(SocketBuffer);

/******************************************************************************
** Function: PKTMGR_Constructor
**
*/
void PKTMGR_Constructor(PKTMGR_Class_t *PktMgrPtr,
                        INITBL_Class_t *IniTbl,
                        TBLMGR_Class_t *TblMgr)
{

   PktMgr = PktMgrPtr;

   PktMgr->IniTbl       = IniTbl;
   PktMgr->DownlinkOn   = false;
   PktMgr->SuppressSend = true;
   PktMgr->TlmSource    = KIT_TO_TlmSource_LOCAL;
   PktMgr->TlmSockId    = 0;
   PktMgr->TlmUdpPort   = INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_UDP_TLM_PORT);
   strncpy(PktMgr->TlmDestIp, "000.000.000.000", PKTMGR_IP_STR_LEN);
   PktMgr->SubWrappedTlmMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_SUB_WRAPPED_TLM_TOPICID));
   PktMgr->PubWrappedTlmMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_PUB_WRAPPED_TLM_TOPICID));

   PKTMGR_InitStats(INITBL_GetIntConfig(IniTbl, CFG_APP_RUN_LOOP_DELAY),
                    INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_STATS_INIT_DELAY));

   PKTTBL_SetTblToUnused(&(PktMgr->PktTbl.Data));

   CFE_MSG_Init(CFE_MSG_PTR(PktMgr->PktTblTlm), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_PKT_TBL_TLM_TOPICID)), 
                sizeof(KIT_TO_PktTblTlm_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(PktMgr->PubWrappedTlm), PktMgr->PubWrappedTlmMid, sizeof(KIT_TO_WrappedSbMsgTlm_t));
   
   OS_TaskInstallDeleteHandler(&DestructorCallback); /* Called when application terminates */

   PktMgr->TlmPipeStatus = CFE_SB_CreatePipe(&(PktMgr->TlmPipe),
                                             INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_PIPE_DEPTH),
                                             INITBL_GetStrConfig(IniTbl, CFG_PKTMGR_PIPE_NAME));

   if (PktMgr->TlmPipeStatus == CFE_SUCCESS)
   {
      PKTTBL_Constructor(&PktMgr->PktTbl, LoadPktTbl);
      TBLMGR_RegisterTblWithDef(TblMgr, PKTTBL_NAME,
                                PKTTBL_LoadCmd, PKTTBL_DumpCmd,
                                INITBL_GetStrConfig(IniTbl, CFG_PKTTBL_LOAD_FILE));
   }
   else
   {
      // Don't isolate the error. Pipe depth error is most likely cause. 
      CFE_EVS_SendEvent(PKTMGR_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR,
                        "Error creating telemetry pipe, status=%d. Verify pipe depth %d is within cFE configuration limit.",
                        PktMgr->TlmPipeStatus, INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_PIPE_DEPTH));
   }
   
} /* End PKTMGR_Constructor() */


/******************************************************************************
** Function: PKTMGR_AddPktCmd
**
** Notes:
**   1. Command rejected if table has existing entry for commanded msg ID
**   2. Only update the table if the software bus subscription successful.  
** 
*/
bool PKTMGR_AddPktCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_AddPkt_CmdPayload_t *AddPkt = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_AddPkt_t);
   bool          RetStatus = true;
   PKTTBL_Pkt_t  NewPkt;
   int32         Status;
   uint16        AppId;

   
   AppId = AddPkt->MsgId & PKTTBL_APP_ID_MASK;
   
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId == PKTTBL_UNUSED_MSG_ID)
   {
      
      NewPkt.MsgId          = AddPkt->MsgId;
      NewPkt.Qos            = AddPkt->Qos;
      NewPkt.BufLim         = AddPkt->BufLim;
      NewPkt.Filter.Type    = AddPkt->FilterType;
      NewPkt.Filter.Param.N = AddPkt->FilterParam.N;
      NewPkt.Filter.Param.X = AddPkt->FilterParam.X;
      NewPkt.Filter.Param.O = AddPkt->FilterParam.O;
   
      Status = SubscribeNewPkt(&NewPkt);
   
      if (Status == CFE_SUCCESS)
      {

         PktMgr->PktTbl.Data.Pkt[AppId] = NewPkt;
      
         CFE_EVS_SendEvent(PKTMGR_ADD_PKT_EID, CFE_EVS_EventType_INFORMATION,
                           "Added message ID 0x%04X, QoS (%d,%d), BufLim %d",
                           NewPkt.MsgId, NewPkt.Qos.Priority, NewPkt.Qos.Reliability, NewPkt.BufLim);
      }
      else
      {
   
         CFE_EVS_SendEvent(PKTMGR_ADD_PKT_EID, CFE_EVS_EventType_ERROR,
                           "Error adding message ID 0x%04X. Software Bus subscription failed with return status 0x%8x",
                           AddPkt->MsgId, Status);
      }
   
   } /* End if packet entry unused */
   else
   {
   
      CFE_EVS_SendEvent(PKTMGR_ADD_PKT_EID, CFE_EVS_EventType_ERROR,
                        "Error adding message ID 0x%04X. Packet already exists in the packet table",
                        AddPkt->MsgId);
   }
   
   return RetStatus;

} /* End of PKTMGR_AddPktCmd() */


/******************************************************************************
** Function: PKTMGR_EnableOutputCmd
**
*/
bool PKTMGR_EnableOutputCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_EnableOutput_CmdPayload_t *EnableOutput = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_EnableOutput_t);
   bool  RetStatus = false;
   int32 OsStatus;
   

   if (PktMgr->TlmPipeStatus == CFE_SUCCESS)
   {
      
      /*
      ** Always update the socket address regardless of the downlink state.
      */

      strncpy(PktMgr->TlmDestIp, EnableOutput->DestIp, PKTMGR_IP_STR_LEN);
      OS_SocketAddrInit(&PktMgr->TlmSocketAddr, OS_SocketDomain_INET);
      OS_SocketAddrFromString(&PktMgr->TlmSocketAddr, PktMgr->TlmDestIp);
      OS_SocketAddrSetPort(&PktMgr->TlmSocketAddr, PktMgr->TlmUdpPort);

      OsStatus = OS_SUCCESS;
      if(PktMgr->DownlinkOn == false)
      {
         OsStatus = OS_SocketOpen(&PktMgr->TlmSockId, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
      }
      
      if (OsStatus == OS_SUCCESS)
      {
         RetStatus = true;
         PKTMGR_InitStats(INITBL_GetIntConfig(PktMgr->IniTbl, CFG_APP_RUN_LOOP_DELAY),
                          INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_STATS_CONFIG_DELAY));
         PktMgr->DownlinkOn   = true;
         PktMgr->SuppressSend = false;
         CFE_EVS_SendEvent(PKTMGR_TLM_ENA_OUTPUT_EID, CFE_EVS_EventType_INFORMATION,
                           "Telemetry output enabled for IP %s", PktMgr->TlmDestIp);         
      }
      else
      {
         CFE_EVS_SendEvent(PKTMGR_TLM_ENA_OUTPUT_EID, CFE_EVS_EventType_ERROR,
                           "Telemetry output socket open error. Status = %d", OsStatus);
      }
   
   } /* End if pipe created */
   else
   {
      CFE_EVS_SendEvent(PKTMGR_TLM_ENA_OUTPUT_EID, CFE_EVS_EventType_ERROR,
                        "Enable telemetry output command rejected. Telemetry pipe not created"); 
   }
   
   return RetStatus;

} /* End PKTMGR_EnableOutputCmd() */


/******************************************************************************
** Function:  PKTMGR_InitStats
**
** If OutputTlmInterval==0 then retain current stats
** ComputeStats() logic assumes at least 1 init cycle
**
*/
void PKTMGR_InitStats(uint16 OutputTlmInterval, uint16 InitDelay)
{
   
   if (OutputTlmInterval != 0) PktMgr->Stats.OutputTlmInterval = (double)OutputTlmInterval;
   
   PktMgr->Stats.State = PKTMGR_STATS_INIT_CYCLE;
   PktMgr->Stats.InitCycles = (PktMgr->Stats.OutputTlmInterval >= InitDelay) ? 1 : (double)InitDelay/PktMgr->Stats.OutputTlmInterval;
            
   PktMgr->Stats.IntervalMilliSecs = 0.0;
   PktMgr->Stats.IntervalPkts = 0;
   PktMgr->Stats.IntervalBytes = 0;
      
   PktMgr->Stats.PrevIntervalAvgPkts  = 0.0;
   PktMgr->Stats.PrevIntervalAvgBytes = 0.0;
   
   PktMgr->Stats.AvgPktsPerSec  = 0.0;
   PktMgr->Stats.AvgBytesPerSec = 0.0;

} /* End PKTMGR_InitStats() */


/******************************************************************************
** Function: PKTMGR_OutputTelemetry
**
*/
uint16 PKTMGR_OutputTelemetry(void)
{

   int     SocketStatus;
   int32   SbStatus;
   bool    SendMsg;
   uint16  NumPktsOutput  = 0;
   uint32  NumBytesOutput = 0;
   size_t  EdsDataSize;
   
   CFE_SB_MsgId_t   MsgId;
   CFE_MSG_ApId_t   MsgAppId;
   CFE_MSG_Size_t   MsgLen;
   CFE_SB_Buffer_t  *SbBufPtr;
   PKTTBL_Pkt_t     *PktTblEntry;
   const CFE_MSG_Message_t *MsgPtr;

   if (PktMgr->TlmPipeStatus != CFE_SUCCESS) return 0; // If tlm pipe not created
    
   /*
   ** CFE_SB_RcvMsg returns CFE_SUCCESS when it gets a packet, otherwise
   ** no packet was received
   */
   do
   {

      SbStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->TlmPipe, CFE_SB_POLL);
 
      if ( (SbStatus == CFE_SUCCESS) && (PktMgr->SuppressSend == false) )
      {
          
         CFE_MSG_GetSize(&SbBufPtr->Msg, &MsgLen);
         CFE_MSG_GetApId(&SbBufPtr->Msg, &MsgAppId);
         MsgAppId = MsgAppId & PKTTBL_APP_ID_MASK;
         PktTblEntry = &(PktMgr->PktTbl.Data.Pkt[MsgAppId]);

         if (!PktUtil_IsPacketFiltered(&SbBufPtr->Msg, &PktTblEntry->Filter))
         {
            if (PktTblEntry->Forward)
            {
               // TODO - Can message length be tailored to wrapped message size?
               // TODO - Could make message information with a filter
               CFE_EVS_SendEvent(PKTMGR_FORWARD_EID, CFE_EVS_EventType_DEBUG,
                                 "Forwarding app ID %d, length %d", MsgAppId, (int16)MsgLen);
               
               memcpy(&(PktMgr->PubWrappedTlm.Payload), &SbBufPtr->Msg, MsgLen);
               CFE_SB_TimeStampMsg(CFE_MSG_PTR(PktMgr->PubWrappedTlm.TelemetryHeader));
               CFE_SB_TransmitMsg(CFE_MSG_PTR(PktMgr->PubWrappedTlm.TelemetryHeader), true);
               PktMgr->PktForwardCnt++;
            } 
            
            if(PktMgr->DownlinkOn)
            {
              
               SendMsg = false; 
               SbStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
               if (SbStatus == CFE_SUCCESS)
               {
                  if (PktMgr->TlmSource == KIT_TO_TlmSource_LOCAL)
                  {
                     
                     // If it's not a wrapped message then sent it
                     if (!CFE_SB_MsgId_Equal(MsgId, PktMgr->SubWrappedTlmMid))
                     {
                        SendMsg = true;
                        MsgPtr  = &(SbBufPtr->Msg); 
                     }
                  }
                  else
                  {
                     // Only wrapped messages
                     if (CFE_SB_MsgId_Equal(MsgId, PktMgr->SubWrappedTlmMid))
                     {
                        // TODO - Verify unwrapped message
                        // TODO - Could make message information with a filter
                        CFE_EVS_SendEvent(PKTMGR_UNWRAP_EID, CFE_EVS_EventType_INFORMATION,
                                          "Unwrapping msg ID 0x%04X(%d)", CFE_SB_MsgIdToValue(MsgId),CFE_SB_MsgIdToValue(MsgId));
                        const KIT_TO_WrappedSbMsgTlm_Payload_t *SbMsgPayload = CMDMGR_PAYLOAD_PTR(&(SbBufPtr->Msg), KIT_TO_WrappedSbMsgTlm_t);
                        SendMsg = true;
                        MsgPtr  = (CFE_MSG_Message_t *)SbMsgPayload;
                     } 
                  }

                  if (SendMsg)
                  {
                     SbStatus = PackEdsOutputMessage(SocketBuffer, MsgPtr, SocketBufferLen, &EdsDataSize);     
                     if (SbStatus == CFE_SUCCESS)
                     {
                        SocketStatus = OS_SocketSendTo(PktMgr->TlmSockId, SocketBuffer, EdsDataSize, &PktMgr->TlmSocketAddr);
                        ++NumPktsOutput;
                        NumBytesOutput += MsgLen;
                     }
                     else
                     {
                        CFE_EVS_SendEvent(PKTMGR_EDS_PACK_MSG_ERR_EID,CFE_EVS_EventType_ERROR,
                              "Error packing EDS output message %d, len %ld",
                              CFE_SB_MsgIdToValue(MsgId),MsgLen);
                     }
                  } /* End if send msg */
               } /* End if got msg id */
            } /* End if downlink enabled */
         } /* End if packet is not filtered */
         else
         {
            SocketStatus = 0;
         } 
         
         if (SocketStatus < 0)
         {
             
            CFE_EVS_SendEvent(PKTMGR_SOCKET_SEND_ERR_EID,CFE_EVS_EventType_ERROR,
                              "Error sending packet on socket %s, port %d, status %d. Tlm output suppressed\n",
                              PktMgr->TlmDestIp, PktMgr->TlmUdpPort, SocketStatus);
            PktMgr->SuppressSend = true;
         }

      } /* End if SB received msg and output enabled */

   } while(SbStatus == CFE_SUCCESS);

   ComputeStats(NumPktsOutput, NumBytesOutput);

   return NumPktsOutput;
   
} /* End of PKTMGR_OutputTelemetry() */


/******************************************************************************
** Function: PKTMGR_RemoveAllPktsCmd
**
** Notes:
**   1. The cFE to_lab code unsubscribes the command and send HK MIDs. I'm not
**      sure why this is done and I'm not sure how the command is used. This 
**      command is intended to help manage TO telemetry packets.
*/
bool PKTMGR_RemoveAllPktsCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   uint16   AppId;
   uint16   PktCnt = 0;
   uint16   FailedUnsubscribe = 0;
   int32    Status;
   bool     RetStatus = true;

   for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++)
   {
      
      if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID)
      {
          
         ++PktCnt;

         Status = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(PktMgr->PktTbl.Data.Pkt[AppId].MsgId), PktMgr->TlmPipe);
         if(Status != CFE_SUCCESS)
         {
             
            FailedUnsubscribe++;
            CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_EID, CFE_EVS_EventType_ERROR,
                              "Error removing message ID 0x%04X at table packet index %d. Unsubscribe status 0x%8X",
                              PktMgr->PktTbl.Data.Pkt[AppId].MsgId, AppId, Status);
         }

         PKTTBL_SetPacketToUnused(&(PktMgr->PktTbl.Data.Pkt[AppId]));

      } /* End if packet in use */

   } /* End AppId loop */

   CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, 
                     "PKTMGR_RemoveAllPktsCmd() - About to flush pipe\n");
   FlushTlmPipe();
   CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, 
                     "PKTMGR_RemoveAllPktsCmd() - Completed pipe flush\n");

   if (FailedUnsubscribe == 0)
   {
      
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_EID, CFE_EVS_EventType_INFORMATION,
                        "Removed %d table packet entries", PktCnt);
   }
   else
   {
      
      RetStatus = false;
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_EID, CFE_EVS_EventType_INFORMATION,
                        "Attempted to remove %d packet entries. Failed %d unsubscribes",
                        PktCnt, FailedUnsubscribe);
   }

   return RetStatus;

} /* End of PKTMGR_RemoveAllPktsCmd() */


/*******************************************************************
** Function: PKTMGR_RemovePktCmd
**
*/
bool PKTMGR_RemovePktCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_RemovePkt_CmdPayload_t *RemovePkt = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_RemovePkt_t);
   bool    RetStatus = true;
   uint16  AppId;
   int32   Status;
  
   
   AppId = RemovePkt->MsgId & PKTTBL_APP_ID_MASK;
  
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID)
   {

      PKTTBL_SetPacketToUnused(&(PktMgr->PktTbl.Data.Pkt[AppId]));
      
      Status = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(RemovePkt->MsgId), PktMgr->TlmPipe);
      if(Status == CFE_SUCCESS)
      {
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_EID, CFE_EVS_EventType_INFORMATION,
                           "Succesfully removed message ID 0x%04X from the packet table",
                           RemovePkt->MsgId);
      }
      else
      {
         RetStatus = false;
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_EID, CFE_EVS_EventType_ERROR,
                           "Removed message ID 0x%04X from packet table, but SB unsubscribe failed with return status 0x%8x",
                           RemovePkt->MsgId, Status);
      }

   } /* End if found message ID in table */
   else
   {

      CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_EID, CFE_EVS_EventType_ERROR,
                        "Error removing message ID 0x%04X. Packet not defined in packet table.",
                        RemovePkt->MsgId);

   } /* End if didn't find message ID in table */

   return RetStatus;

} /* End of PKTMGR_RemovePktCmd() */


/******************************************************************************
** Function:  PKTMGR_ResetStatus
**
*/
void PKTMGR_ResetStatus(void)
{

   PKTMGR_InitStats(0,INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_STATS_CONFIG_DELAY));
   PktMgr->PktForwardCnt = 0;

} /* End PKTMGR_ResetStatus() */


/*******************************************************************
** Function: PKTMGR_SendPktTblTlmCmd
**
*/
bool PKTMGR_SendPktTblTlmCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_SendPktTblTlm_CmdPayload_t *SendPktTblTlm = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_SendPktTblTlm_t);
   uint16        AppId;
   PKTTBL_Pkt_t* PktPtr;
   int32         Status;
   KIT_TO_PktTblTlm_Payload_t *Payload = &PktMgr->PktTblTlm.Payload;
  
  
   AppId  = SendPktTblTlm->MsgId & PKTTBL_APP_ID_MASK;
   PktPtr = &(PktMgr->PktTbl.Data.Pkt[AppId]);
   
   Payload->MsgId  = PktPtr->MsgId;
   Payload->Qos    = PktPtr->Qos;
   Payload->BufLim = PktPtr->BufLim;
   
   Payload->FilterType    = PktPtr->Filter.Type;
   Payload->FilterParam.N = PktPtr->Filter.Param.N;
   Payload->FilterParam.X = PktPtr->Filter.Param.X;
   Payload->FilterParam.O = PktPtr->Filter.Param.O;

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(PktMgr->PktTblTlm));
   Status = CFE_SB_TransmitMsg(CFE_MSG_PTR(PktMgr->PktTblTlm), true);
    
   return (Status == CFE_SUCCESS);

} /* End of PKTMGR_SendPktTblTlmCmd() */


/*******************************************************************
** Function: PKTMGR_SetTlmSourceCmd
**
*/
bool PKTMGR_SetTlmSourceCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_SetTlmSource_CmdPayload_t *SetTlmSource = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_SetTlmSource_t);
   bool RetStatus = false;


   if ((SetTlmSource->Source >= KIT_TO_TlmSource_Enum_t_MIN) &&
       (SetTlmSource->Source <= KIT_TO_TlmSource_Enum_t_MAX))
   {
      PktMgr->TlmSource = SetTlmSource->Source;
      CFE_EVS_SendEvent(PKTMGR_SET_TLM_SOURCE_CMD_EID, CFE_EVS_EventType_INFORMATION,
                        "Telemetry source set to %d", SetTlmSource->Source);
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(PKTMGR_SET_TLM_SOURCE_CMD_EID, CFE_EVS_EventType_ERROR,
                        "Error setting tlm source, invalid source ID %d",
                        SetTlmSource->Source);    
   }   
   
   return RetStatus;

} /* End of PKTMGR_SetTlmSourceCmd() */


/******************************************************************************
** Function: PKTMGR_UpdatePktFilterCmd
**
** Notes:
**   1. Command rejected if AppId packet entry has not been loaded 
**   2. The filter type is verified but the filter parameter values are not 
** 
*/
bool PKTMGR_UpdatePktFilterCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const  KIT_TO_UpdatePktFilter_CmdPayload_t *UpdatePktFilter = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_UpdatePktFilter_t);
   bool   RetStatus = false;
   uint16 AppId;

   
   AppId = UpdatePktFilter->MsgId & PKTTBL_APP_ID_MASK;
   
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID)
   {
      
      if (PktUtil_IsFilterTypeValid(UpdatePktFilter->FilterType))
      {
        
         PktUtil_Filter_t *TblFilter = &(PktMgr->PktTbl.Data.Pkt[AppId].Filter);
         
         CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_EID, CFE_EVS_EventType_INFORMATION,
                           "Successfully changed message ID 0x%04X's filter (Type,N,X,O) from (%d,%d,%d,%d) to (%d,%d,%d,%d)",
                           UpdatePktFilter->MsgId,
                           TblFilter->Type, TblFilter->Param.N, TblFilter->Param.X, TblFilter->Param.O,
                           UpdatePktFilter->FilterType,    UpdatePktFilter->FilterParam.N,
                           UpdatePktFilter->FilterParam.X, UpdatePktFilter->FilterParam.O);
                           
         TblFilter->Type    = UpdatePktFilter->FilterType;
         TblFilter->Param.N = UpdatePktFilter->FilterParam.N;      
         TblFilter->Param.X = UpdatePktFilter->FilterParam.X;      
         TblFilter->Param.O = UpdatePktFilter->FilterParam.O;      
        
         RetStatus = true;
      
      } /* End if valid packet filter type */
      else
      {
   
         CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_EID, CFE_EVS_EventType_ERROR,
                           "Error updating filter for message ID 0x%04X. Invalid filter type %d",
                           UpdatePktFilter->MsgId, UpdatePktFilter->FilterType);
      }
   
   } /* End if packet entry unused */
   else
   {
   
      CFE_EVS_SendEvent(PKTMGR_UPDATE_FILTER_CMD_EID, CFE_EVS_EventType_ERROR,
                        "Error updating filter for message ID 0x%04X. Packet not in use",
                        UpdatePktFilter->MsgId);
   }
   
   return RetStatus;

} /* End of PKTMGR_UpdatePktFilterCmd() */


/******************************************************************************
** Function:  ComputeStats
**
** Called each output telemetry cycle
*/
static void ComputeStats(uint16 PktsSent, uint32 BytesSent)
{

   uint32 DeltaTimeMicroSec;   
   CFE_TIME_SysTime_t CurrTime = CFE_TIME_GetTime();
   CFE_TIME_SysTime_t DeltaTime;
   
   if (PktMgr->Stats.InitCycles > 0)
   {
   
      --PktMgr->Stats.InitCycles;
      PktMgr->Stats.PrevTime = CFE_TIME_GetTime();
      PktMgr->Stats.State = PKTMGR_STATS_INIT_CYCLE;

   }
   else
   {
      
      DeltaTime = CFE_TIME_Subtract(CurrTime, PktMgr->Stats.PrevTime);
      DeltaTimeMicroSec = CFE_TIME_Sub2MicroSecs(DeltaTime.Subseconds); 
      
      PktMgr->Stats.IntervalMilliSecs += (double)DeltaTime.Seconds*1000.0 + (double)DeltaTimeMicroSec/1000.0;
      PktMgr->Stats.IntervalPkts      += PktsSent;
      PktMgr->Stats.IntervalBytes     += BytesSent;

      if (PktMgr->Stats.IntervalMilliSecs >= PktMgr->Stats.OutputTlmInterval)
      {
      
         double Seconds = PktMgr->Stats.IntervalMilliSecs/1000;
         
         CFE_EVS_SendEvent(PKTMGR_DEBUG_EID, CFE_EVS_EventType_DEBUG,
                           "IntervalSecs=%f, IntervalPkts=%d, IntervalBytes=%d\n",
                           Seconds,PktMgr->Stats.IntervalPkts,PktMgr->Stats.IntervalBytes);
        
         PktMgr->Stats.AvgPktsPerSec  = (double)PktMgr->Stats.IntervalPkts/Seconds;
         PktMgr->Stats.AvgBytesPerSec = (double)PktMgr->Stats.IntervalBytes/Seconds;
         
         /* Good enough running average that avoids overflow */
         if (PktMgr->Stats.State == PKTMGR_STATS_INIT_CYCLE) {
     
            PktMgr->Stats.State = PKTMGR_STATS_INIT_INTERVAL;
       
         }
         else
         {
            
            PktMgr->Stats.State = PKTMGR_STATS_VALID;
            PktMgr->Stats.AvgPktsPerSec  = (PktMgr->Stats.AvgPktsPerSec  + PktMgr->Stats.PrevIntervalAvgPkts) / 2.0; 
            PktMgr->Stats.AvgBytesPerSec = (PktMgr->Stats.AvgBytesPerSec + PktMgr->Stats.PrevIntervalAvgBytes) / 2.0; 
  
         }
         
         PktMgr->Stats.PrevIntervalAvgPkts  = PktMgr->Stats.AvgPktsPerSec;
         PktMgr->Stats.PrevIntervalAvgBytes = PktMgr->Stats.AvgBytesPerSec;
         
         PktMgr->Stats.IntervalMilliSecs = 0.0;
         PktMgr->Stats.IntervalPkts      = 0;
         PktMgr->Stats.IntervalBytes     = 0;
      
      } /* End if report cycle */
      
      PktMgr->Stats.PrevTime = CFE_TIME_GetTime();
      
   } /* End if not init cycle */
   

} /* End ComputeStats() */


/******************************************************************************
** Function: DestructorCallback
**
** This function is called when the app is killed. This should
** never occur but if it does this will close the network socket.
*/
static void DestructorCallback(void)
{

   CFE_EVS_SendEvent(PKTMGR_DESTRUCTOR_INFO_EID, CFE_EVS_EventType_INFORMATION, 
                     "Destructor callback -- Closing TO Network socket. Downlink on = %d\n",
                     PktMgr->DownlinkOn);
   
   if (PktMgr->DownlinkOn)
   {
      
      OS_close(PktMgr->TlmSockId);
   
   }

} /* End DestructorCallback() */


/******************************************************************************
** Function: FlushTlmPipe
**
** Remove all of the packets from the input pipe.
**
*/
static void FlushTlmPipe(void)
{

   int32 SbStatus;
   CFE_SB_Buffer_t  *SbBufPtr;

   do
   {
      SbStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->TlmPipe, CFE_SB_POLL);

   } while(SbStatus == CFE_SUCCESS);

} /* End FlushTlmPipe() */
   

/******************************************************************************
** Function: LoadPktTbl
**
** Notes:
**   1. Function signature must match the PKTTBL_LoadNewTbl_t definition
**   2. After the previous table's subscriptions are removed the new table is
**      copied into the working table data structure. However there could still
**      be subscription errors because of invalid table data so in a sense  
*/
static bool LoadPktTbl(PKTTBL_Data_t *NewTbl)
{

   uint16  AppId;
   uint16  PktCnt = 0;
   uint16  FailedSubscription = 0;
   int32   Status;
   bool    RetStatus = true;

   CFE_MSG_Message_t *MsgPtr = NULL;

   PKTMGR_RemoveAllPktsCmd(NULL, MsgPtr);  /* Both parameters are unused so OK to be NULL */

   CFE_PSP_MemCpy(&(PktMgr->PktTbl), NewTbl, sizeof(PKTTBL_Data_t));

   for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++)
   {

      if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTTBL_UNUSED_MSG_ID)
      {
         
         ++PktCnt;
         Status = SubscribeNewPkt(&(PktMgr->PktTbl.Data.Pkt[AppId])); 

         if(Status != CFE_SUCCESS)
         {
            
            ++FailedSubscription;
            CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_EID,CFE_EVS_EventType_ERROR,
                              "Error subscribing to message ID 0x%04X, BufLim %d, Status %i",
                              PktMgr->PktTbl.Data.Pkt[AppId].MsgId, 
                              PktMgr->PktTbl.Data.Pkt[AppId].BufLim, Status);
         }
      }

   } /* End pkt loop */

   if (FailedSubscription == 0)
   {
      
      PKTMGR_InitStats(INITBL_GetIntConfig(PktMgr->IniTbl, CFG_APP_RUN_LOOP_DELAY),
                       INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_STATS_INIT_DELAY));
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_EID, CFE_EVS_EventType_INFORMATION,
                        "Successfully loaded new table with %d packets", PktCnt);
   }
   else
   {
      
      RetStatus = false;
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_EID, CFE_EVS_EventType_INFORMATION,
                        "Attempted to load new table with %d packets. Failed %d subscriptions",
                        PktCnt, FailedSubscription);
   }

   return RetStatus;

} /* End LoadPktTbl() */


/******************************************************************************
** Function: PackEdsOutputMessage
**
** Notes:
**   1. Adopted from NASA"S cfE-eds-framework TO_LAB app
**
*/
static int32 PackEdsOutputMessage(void *DestBuffer, const CFE_MSG_Message_t *SrcBuffer, 
                                  size_t SrcBufferSize, size_t *EdsDataSize)
{
    EdsLib_Id_t                           EdsId;
    EdsLib_DataTypeDB_TypeInfo_t          TypeInfo;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Publisher_Component_t          PublisherParams;
    uint16                                TopicId;
    int32                                 Status;
    size_t                                SrcMsgSize;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    CFE_MSG_GetSize(SrcBuffer, &SrcMsgSize);

    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &SrcBuffer->BaseMsg);
    CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);
    TopicId = PublisherParams.Telemetry.TopicId;

    Status = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telemetry_Interface_ID, TopicId, 1, 1,
                                            &EdsId);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_STATUS_UNKNOWN_MSG_ID;
    }

    Status = EdsLib_DataTypeDB_PackCompleteObject(EDS_DB, &EdsId, DestBuffer, SrcBuffer, 8 * SrcBufferSize,
                                                  SrcMsgSize);
    if (Status != EDSLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    Status = EdsLib_DataTypeDB_GetTypeInfo(EDS_DB, EdsId, &TypeInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    *EdsDataSize = (TypeInfo.Size.Bits + 7) / 8;
    
    return CFE_SUCCESS;
}


/******************************************************************************
** Function: SubscribeNewPkt
**
*/
static int32 SubscribeNewPkt(PKTTBL_Pkt_t *NewPkt)
{

   int32 Status = CFE_SUCCESS;
   CFE_SB_MsgId_t MsgId = CFE_SB_ValueToMsgId(NewPkt->MsgId);

   // Don't subscribe to messages that get wrapped and forwarded because they'd immediately get forwarded to KIT_TO
   if (!CFE_SB_MsgId_Equal(MsgId, PktMgr->PubWrappedTlmMid))
   {
      Status = CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(NewPkt->MsgId), PktMgr->TlmPipe, NewPkt->Qos, NewPkt->BufLim);
      CFE_EVS_SendEvent(PKTMGR_SUBSCRIBE_EID, CFE_EVS_EventType_DEBUG,
                        "Subscribed to message 0x%04X(%d)",NewPkt->MsgId, NewPkt->MsgId); 
   }
   else
   {
      CFE_EVS_SendEvent(PKTMGR_SUBSCRIBE_EID, CFE_EVS_EventType_INFORMATION,
                        "Skip subscribing to tunnel message 0x%04X(%d)",NewPkt->MsgId, NewPkt->MsgId);    
   }

   return Status;

} /* End SubscribeNewPkt(() */


