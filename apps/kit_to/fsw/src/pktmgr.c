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
**   1. Packet output is supressed during initialization. The enable
**      output command needs to be sent to start output to a command
**      specified IP address and port. 
**   2. This has some of the features of a flight app such as packet
**      filtering but it would need design/code reviews to transition
**      it to a flight mission. For starters it uses UDP sockets and 
**      it doesn't regulate output bit rates.
**   3. CCSDS virtual channels are not actually used but two telemetry
**      pipes are setup as if they are virtual channels to illustrate
**      how they can be used. 
**      - VC1: File Transfer (FILE_XFER) data segment packets
**      - VC2: All other telemetry packets
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
#include "edslib_global.h"
#include "edslib_datatypedb.h"
#include "edslib_intfdb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include "app_cfg.h"
#include "pktmgr.h"


/******************************/
/** File Function Prototypes **/
/******************************/

static void ComputeStats(uint16 PktsSent, uint32 BytesSent);
static CFE_Status_t CreateEdsOutputMessage(void *DstBuffer, const CFE_MSG_Message_t *SrcBuffer, 
                                           size_t SrcBufferSize, size_t *EdsDataSize);
static void DestructorCallback(void);
static CFE_Status_t EncodeEdsOutputPayload(void *DstBuffer, const CFE_MSG_Message_t *SrcBuffer, EdsLib_Id_t ParentIntfId,
                                           const EdsLib_SizeInfo_t *MaxSize, EdsLib_SizeInfo_t *ProcessedSize);
static void FlushTlmPipes(void);
static bool LoadPktTbl(PKTTBL_Data_t *NewTbl);
static uint16 OutputTelemetry(const CFE_SB_Buffer_t *SbBufPtr);
static int32 SubscribeNewPkt(PKTTBL_Pkt_t *NewPkt);

/**********************/
/** Global File Data **/
/**********************/

static PKTMGR_Class_t *PktMgr = NULL;
static EdsPackedBuffer_CFE_HDR_TelemetryHeader_t SocketBuffer;
static uint16 SocketBufferLen = sizeof(SocketBuffer);

/******************************************************************************
** Function: PKTMGR_Constructor
**
*/
void PKTMGR_Constructor(PKTMGR_Class_t *PktMgrPtr,
                        INITBL_Class_t *IniTbl,
                        TBLMGR_Class_t *TblMgr)
{
   int32 OsStatus;
   
   PktMgr = PktMgrPtr;

   memset(PktMgr,0,sizeof(PKTMGR_Class_t));
   PktMgr->IniTbl          = IniTbl;
   PktMgr->DownlinkOn      = false;
   PktMgr->SuppressSend    = true;
   PktMgr->EdsEncodeErrLim = INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_EDS_ENCODE_ERR_LIM);
   PktMgr->TlmSource       = KIT_TO_TlmSource_LOCAL;
   PktMgr->TlmSockId       = 0;
   PktMgr->TlmUdpPort      = INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_UDP_TLM_PORT);
   strncpy(PktMgr->TlmDestIp, "000.000.000.000", PKTMGR_IP_STR_LEN);
   PktMgr->SubWrappedTlmMid  = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_SUB_WRAPPED_TLM_TOPICID));
   PktMgr->PubWrappedTlmMid  = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_PUB_WRAPPED_TLM_TOPICID));
   
   PktMgr->Vc1FileDataTlmVal = INITBL_GetIntConfig(IniTbl, CFG_FILE_XFER_FOTP_DATA_SEGMENT_TLM_TOPICID);
   PktMgr->Vc1FileDataTlmMid = CFE_SB_ValueToMsgId(PktMgr->Vc1FileDataTlmVal);

   PKTMGR_InitStats(INITBL_GetIntConfig(IniTbl, CFG_APP_RUN_LOOP_DELAY),
                    INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_STATS_INIT_DELAY));

   PKTTBL_SetTblToUnused(&(PktMgr->PktTbl.Data));

   CFE_MSG_Init(CFE_MSG_PTR(PktMgr->PktTblTlm), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_PKT_TBL_TLM_TOPICID)), 
                sizeof(KIT_TO_PktTblTlm_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(PktMgr->PubWrappedTlm), PktMgr->PubWrappedTlmMid, sizeof(KIT_TO_WrappedSbMsgTlm_t));
   
   OS_TaskInstallDeleteHandler(&DestructorCallback); /* Called when application terminates */


   /*
   ** Create pipes sequentially to allow more specific error reporting
   ** - Could run a system without the file downlink VC pipe but not
   **   worth the complexity for an educational system
   ** 
   */
   PktMgr->Vc1PipeStatus = CFE_SB_CreatePipe(&(PktMgr->Vc1Pipe),
                                             INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_VC1_PIPE_DEPTH),
                                             INITBL_GetStrConfig(IniTbl, CFG_PKTMGR_VC1_PIPE_NAME));
   if (PktMgr->Vc1PipeStatus == CFE_SUCCESS)
   {
      
      PktMgr->Vc2PipeStatus = CFE_SB_CreatePipe(&(PktMgr->Vc2Pipe),
                                                INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_VC2_PIPE_DEPTH),
                                                INITBL_GetStrConfig(IniTbl, CFG_PKTMGR_VC2_PIPE_NAME));

      if (PktMgr->Vc2PipeStatus == CFE_SUCCESS)
      {
         PKTTBL_Constructor(&PktMgr->PktTbl, LoadPktTbl);
         TBLMGR_RegisterTblWithDef(TblMgr, PKTTBL_NAME,
                                   PKTTBL_LoadCmd, PKTTBL_DumpCmd,
                                   INITBL_GetStrConfig(IniTbl, CFG_PKTTBL_LOAD_FILE));

         OsStatus = OS_CountSemCreate(&PktMgr->Vc1SemId, INITBL_GetStrConfig(IniTbl, CFG_PKTMGR_VC1_SEMAPHORE_NAME),
                                      INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_VC1_PIPE_DEPTH), 0);
         if (OsStatus != CFE_SUCCESS)
         {
            CFE_EVS_SendEvent(PKTMGR_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR,
                              "Error creating virtual channel 1 semaphore %s, status=%d",
                              INITBL_GetStrConfig(IniTbl, CFG_PKTMGR_VC1_SEMAPHORE_NAME), OsStatus);            
         }        
      }
      else
      {
         // Don't isolate the error. Pipe depth error is most likely cause. 
         CFE_EVS_SendEvent(PKTMGR_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR,
                           "Error creating virtual channel 2 telemetry pipe, status=%d. Verify pipe depth %d is within cFE configuration limit.",
                           PktMgr->Vc2PipeStatus, INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_VC2_PIPE_DEPTH));
      }
   }
   else
   {
      // Don't isolate the error. Pipe depth error is most likely cause. 
      CFE_EVS_SendEvent(PKTMGR_CONSTRUCTOR_EID, CFE_EVS_EventType_ERROR,
                        "Error creating virtual channel 1 telemetry pipe, status=%d. Verify pipe depth %d is within cFE configuration limit.",
                        PktMgr->Vc1PipeStatus, INITBL_GetIntConfig(IniTbl, CFG_PKTMGR_VC1_PIPE_DEPTH));
   }
   

} /* End PKTMGR_Constructor() */


/******************************************************************************
** Function: PKTMGR_AddPktCmd
**
** Notes:
**   1. Command rejected if table has existing entry for commanded msg ID
**   2. Only update the table if the software bus subscription successful 
**   3. This command only applies to virtual channel 2
*/
bool PKTMGR_AddPktCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_AddPkt_CmdPayload_t *AddPkt = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_AddPkt_t);
   bool          RetStatus = true;
   PKTTBL_Pkt_t  NewPkt;
   int32         Status;
   uint16        AppId;

   
   AppId = AddPkt->MsgId & PKTTBL_APP_ID_MASK;
   
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId == PKTUTIL_UNUSED_MSG_ID)
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

} /* End PKTMGR_AddPktCmd() */


/******************************************************************************
** Function: PKTMGR_EnableOutputCmd
**
*/
bool PKTMGR_EnableOutputCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_EnableOutput_CmdPayload_t *EnableOutput = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_EnableOutput_t);
   bool  RetStatus = false;
   int32 OsStatus;
   

   if (PktMgr->Vc2PipeStatus == CFE_SUCCESS)
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
                           "Telemetry output enabled for IP %s using OSAL object %ld",
                           PktMgr->TlmDestIp, OS_ObjectIdToInteger(PktMgr->TlmSockId));         
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
** Notes:
**   1. If OutputTlmInterval==0 then retain current stats
**   2. ComputeStats() logic assumes at least 1 init cycle
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
** Notes:
**   1. Stats are computed for downlink volume so all virtual channels are
**      included.
*/
uint16 PKTMGR_OutputTelemetry(void)
{

   uint16           NumPktsOutput=0;
   CFE_Status_t     CfeStatus;
   CFE_SB_Buffer_t  *SbBufPtr;
   
   /*
   ** CFE_SB_RcvMsg returns CFE_SUCCESS when it gets a packet, otherwise
   ** no packet was received
   */

   if (PktMgr->Vc1PipeStatus == CFE_SUCCESS)
   {
      do
      {
         CfeStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->Vc1Pipe, CFE_SB_POLL);
         if (CfeStatus == CFE_SUCCESS)
         {
            OS_CountSemGive(PktMgr->Vc1SemId);
            NumPktsOutput = OutputTelemetry(SbBufPtr);
         }
      } while(CfeStatus == CFE_SUCCESS); 
   } /* End VC1 */

   if (PktMgr->Vc2PipeStatus == CFE_SUCCESS)
   {
      do
      {
         CfeStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->Vc2Pipe, CFE_SB_POLL);
         if (CfeStatus == CFE_SUCCESS)
         {
            NumPktsOutput += OutputTelemetry(SbBufPtr);
         }

      } while(CfeStatus == CFE_SUCCESS);
   } /* End VC2 */

   return NumPktsOutput;
   
} /* End PKTMGR_OutputTelemetry() */

      
/******************************************************************************
** Function: PKTMGR_RemoveAllPktsCmd
**
** Notes:
**   1. The use case for this command is unclear but it is included because
**      the cFS TO_LAB app and KIT_TO is designed to include all of TO_LAB's
**      functionality.
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
      
      if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTUTIL_UNUSED_MSG_ID &&
          PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PktMgr->Vc1FileDataTlmVal)
      {
          
         ++PktCnt;

         Status = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(PktMgr->PktTbl.Data.Pkt[AppId].MsgId), PktMgr->Vc2Pipe);
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
   FlushTlmPipes();
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

} /* End PKTMGR_RemoveAllPktsCmd() */


/*******************************************************************
** Function: PKTMGR_RemovePktCmd
**
** Notes:
**   1. Trying to remove a non-existent entry is not considered an error
**   2. This command only applies to virtual channel 2
*/
bool PKTMGR_RemovePktCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_RemovePkt_CmdPayload_t *RemovePkt = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_RemovePkt_t);
   bool    RetStatus = true;
   uint16  AppId;
   int32   Status;
  
   
   AppId = RemovePkt->MsgId & PKTTBL_APP_ID_MASK;
  
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTUTIL_UNUSED_MSG_ID)
   {

      if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PktMgr->Vc1FileDataTlmVal)
      {
         PKTTBL_SetPacketToUnused(&(PktMgr->PktTbl.Data.Pkt[AppId]));
         
         Status = CFE_SB_Unsubscribe(CFE_SB_ValueToMsgId(RemovePkt->MsgId), PktMgr->Vc2Pipe);
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
      }
      else
      {
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_EID, CFE_EVS_EventType_ERROR,
                           "Remove message 0x%04X command rejected. Message is file data on virtual channel 1 that can't be removed",
                           RemovePkt->MsgId);         
      }
   } /* End if found message ID in table */
   else
   {

      CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_EID, CFE_EVS_EventType_ERROR,
                        "Error removing message ID 0x%04X. Packet not defined in packet table.",
                        RemovePkt->MsgId);

   } /* End if didn't find message ID in table */

   return RetStatus;

} /* End PKTMGR_RemovePktCmd() */


/******************************************************************************
** Function: PKTMGR_ResetStatus
**
*/
void PKTMGR_ResetStatus(void)
{

   PKTMGR_InitStats(0,INITBL_GetIntConfig(PktMgr->IniTbl, CFG_PKTMGR_STATS_CONFIG_DELAY));
   PktMgr->PktForwardCnt = 0;
   PktMgr->EdsEncodeErrCnt = 0;

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

} /* End oPKTMGR_SendPktTblTlmCmd() */


/*******************************************************************
** Function: PKTMGR_SetTlmSourceCmd
**
*/
bool PKTMGR_SetTlmSourceCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_SetTlmSource_CmdPayload_t *SetTlmSource = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_SetTlmSource_t);
   bool RetStatus = false;


   if ((SetTlmSource->Source >= EdsDataType_EdsEnum_KIT_TO_TlmSource_t_MIN) &&
       (SetTlmSource->Source <= EdsDataType_EdsEnum_KIT_TO_TlmSource_t_MAX))
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

} /* End PKTMGR_SetTlmSourceCmd() */


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
   
   if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTUTIL_UNUSED_MSG_ID)
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

} /* End PKTMGR_UpdatePktFilterCmd() */


/******************************************************************************
** Function: ComputeStats
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
         
         CFE_EVS_SendEvent(PKTMGR_COMPUTE_STATS_EID, CFE_EVS_EventType_DEBUG,
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
** Function: CreateEdsOutputMessage
**
** Notes:
**   1. Adopted from NASA'S cfE-eds-framework TO_LAB app's
**      TO_LAB_EncodeOutputMessage()
**
*/
static CFE_Status_t CreateEdsOutputMessage(void *DstBuffer, const CFE_MSG_Message_t *SrcBuffer, 
                                           size_t SrcBufferSize, size_t *EdsDataSize)
{

   CFE_MissionLib_TopicInfo_t            TopicInfo;
   CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
   CFE_SB_Publisher_Component_t          PublisherParams;
   int32                                 EdsStatus;
   CFE_Status_t                          CfeStatus;
   CFE_MSG_Size_t                        SrcMsgSize;
   EdsLib_SizeInfo_t                     MaxSize;
   EdsLib_SizeInfo_t                     ProcessedSize;
    
   memset(&MaxSize, 0, sizeof(MaxSize));
   memset(&ProcessedSize, 0, sizeof(ProcessedSize));
    
   CfeStatus = CFE_MSG_GetSize(SrcBuffer, &SrcMsgSize);
   if (CfeStatus == CFE_SUCCESS)
   {
      MaxSize.Bits  = EdsLib_OCTETS_TO_BITS(sizeof(SocketBuffer));
      MaxSize.Bytes = SrcMsgSize;
   }
    
   CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &SrcBuffer->BaseMsg);
   CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);
   
   EdsStatus = CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, PublisherParams.Telemetry.TopicId, &TopicInfo);
   if (EdsStatus != CFE_MISSIONLIB_SUCCESS)
   {
      CFE_EVS_SendEvent(PKTMGR_EDS_ENCODE_MSG_ERR_EID,CFE_EVS_EventType_ERROR,
            "Error getting EDS topic ID, CFE_MissionLib_GetTopicInfo() return status %d",
            (int)EdsStatus);
      CfeStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
   }
   else if (EdsLib_Is_Valid(TopicInfo.ParentIntfId))
   {
      CfeStatus = EncodeEdsOutputPayload(SocketBuffer, SrcBuffer, TopicInfo.ParentIntfId,
                                           &MaxSize, &ProcessedSize);
   }
   else
   {
      CFE_EVS_SendEvent(PKTMGR_EDS_ENCODE_MSG_ERR_EID,CFE_EVS_EventType_ERROR,
                        "No EDS definition for TopicId %u",
                        (unsigned int)PublisherParams.Telemetry.TopicId);
      CfeStatus = CFE_STATUS_VALIDATION_FAILURE;
   }
    
   *EdsDataSize   = EdsLib_BITS_TO_OCTETS(ProcessedSize.Bits);

   return CfeStatus;
    
} /* End CreateEdsOutputMessage() */ 


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
** Function: EncodeEdsOutputPayload
**
** Notes:
**   1. Adopted from NASA'S cfE-eds-framework TO_LAB app's
**      TO_LAB_EncodeOutputPayload()
**
*/
static CFE_Status_t EncodeEdsOutputPayload(void *DstBuffer, const CFE_MSG_Message_t *SrcBuffer, EdsLib_Id_t ParentIntfId,
                                           const EdsLib_SizeInfo_t *MaxSize, EdsLib_SizeInfo_t *ProcessedSize)
{
   int32        EdsStatus;
   CFE_Status_t CfeStatus;
   EdsLib_Id_t  EdsId;

   static const EdsLib_Id_t CFE_SB_TELEMETRY_ID =
        EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsCommand_CFE_SB_Telemetry_indication_DECLARATION);

   const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

   CfeStatus = CFE_SUCCESS;

   /* Note on the encode side, this does not need to process the header separately from the payload. */
   EdsStatus = EdsLib_IntfDB_FindAllArgumentTypes(EDS_DB, CFE_SB_TELEMETRY_ID, ParentIntfId, &EdsId, 1);
   if (EdsStatus != EDSLIB_SUCCESS)
   {
   
      CFE_EVS_SendEvent(PKTMGR_EDS_ENCODE_PYLD_ERR_EID,CFE_EVS_EventType_ERROR,
                        "Error getting EDS arguments types, EdsLib_IntfDB_FindAllArgumentTypes() return status %d",
                        (int)EdsStatus);
      CfeStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    
   }
   else
   {   
      EdsStatus = EdsLib_DataTypeDB_PackPartialObjectVarSize(EDS_DB, &EdsId, DstBuffer, SrcBuffer,
                                                             MaxSize, ProcessedSize);
      if (EdsStatus != EDSLIB_SUCCESS)
      {
      
         CFE_EVS_SendEvent(PKTMGR_EDS_ENCODE_PYLD_ERR_EID,CFE_EVS_EventType_ERROR,
                        "Error packing EDS object, EdsLib_DataTypeDB_PackPartialObjectVarSize() return status %d",
                        (int)EdsStatus);
         CfeStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
         
      }
   }

   if (CfeStatus == CFE_SUCCESS)
   {
      EdsStatus =
            EdsLib_DataTypeDB_FinalizePackedObjectVarSize(EDS_DB, EdsId, SrcBuffer, DstBuffer, ProcessedSize);
      
      if (EdsStatus != EDSLIB_SUCCESS)
      {
         CFE_EVS_SendEvent(PKTMGR_EDS_ENCODE_PYLD_ERR_EID,CFE_EVS_EventType_ERROR,
                        "Error finalizing EDS object, EdsLib_DataTypeDB_FinalizePackedObjectVarSize() return status %d",
                        (int)EdsStatus);
         CfeStatus = CFE_STATUS_VALIDATION_FAILURE;
      }
   }

   return CfeStatus;
    
} /* End EncodeEdsOutputPayload() */


/******************************************************************************
** Function: FlushTlmPipes
**
** Remove all packets from all virtual channel pipes.
**
*/
static void FlushTlmPipes(void)
{

   CFE_Status_t     CfeStatus;
   CFE_SB_Buffer_t  *SbBufPtr;

   do
   {
      CfeStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->Vc1Pipe, CFE_SB_POLL);

   } while(CfeStatus == CFE_SUCCESS);

   do
   {
      CfeStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, PktMgr->Vc2Pipe, CFE_SB_POLL);

   } while(CfeStatus == CFE_SUCCESS);

} /* End FlushTlmPipes() */
   

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
      if (PktMgr->PktTbl.Data.Pkt[AppId].MsgId != PKTUTIL_UNUSED_MSG_ID)
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
** Function: OutputTelemetry
**
*/
static uint16 OutputTelemetry(const CFE_SB_Buffer_t *SbBufPtr)
{
   int     SocketStatus;
   bool    SendMsg;
   uint16  NumPktsOutput  = 0;
   uint32  NumBytesOutput = 0;
   size_t  EdsDataSize;
   CFE_Status_t CfeStatus;
   
   CFE_SB_MsgId_t   MsgId;
   CFE_MSG_ApId_t   MsgAppId = PKTUTIL_UNUSED_MSG_ID;
   CFE_MSG_Size_t   MsgLen=0;
   PKTTBL_Pkt_t     *PktTblEntry;
   const CFE_MSG_Message_t *MsgPtr;

   if (PktMgr->SuppressSend == false)
   {   
      CFE_MSG_GetSize(&SbBufPtr->Msg, &MsgLen);
      CFE_MSG_GetApId(&SbBufPtr->Msg, &MsgAppId);
      MsgAppId = MsgAppId & PKTTBL_APP_ID_MASK;
      PktTblEntry = &(PktMgr->PktTbl.Data.Pkt[MsgAppId]);

      if (!PktUtil_IsPacketFiltered(&SbBufPtr->Msg, &PktTblEntry->Filter))
      {
         if (PktTblEntry->Forward)
         {
            // TODO - Can message length be tailored to wrapped message size? Information event with filter?
            CFE_EVS_SendEvent(PKTMGR_OUTPUT_TLM_EID, CFE_EVS_EventType_DEBUG,
                              "Forwarding app ID %d, length %d", MsgAppId, (int16)MsgLen);
            
            memcpy(&(PktMgr->PubWrappedTlm.Payload), &SbBufPtr->Msg, MsgLen);
            CFE_SB_TimeStampMsg(CFE_MSG_PTR(PktMgr->PubWrappedTlm.TelemetryHeader));
            CFE_SB_TransmitMsg(CFE_MSG_PTR(PktMgr->PubWrappedTlm.TelemetryHeader), true);
            PktMgr->PktForwardCnt++;
         } 
         
         if(PktMgr->DownlinkOn)
         {
           
            SendMsg = false; 
            CfeStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
            if (CfeStatus == CFE_SUCCESS)
            {
               if (PktMgr->TlmSource == KIT_TO_TlmSource_LOCAL)
               {
                  
                  // If it's not a wrapped message then send it
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
                     // TODO: Verify unwrapped message, information event with filter?
                     CFE_EVS_SendEvent(PKTMGR_OUTPUT_TLM_EID, CFE_EVS_EventType_INFORMATION,
                                       "Unwrapping msg ID 0x%04X(%d)", CFE_SB_MsgIdToValue(MsgId),CFE_SB_MsgIdToValue(MsgId));
                     const KIT_TO_WrappedSbMsgTlm_Payload_t *SbMsgPayload = CMDMGR_PAYLOAD_PTR(&(SbBufPtr->Msg), KIT_TO_WrappedSbMsgTlm_t);
                     SendMsg = true;
                     MsgPtr  = (CFE_MSG_Message_t *)SbMsgPayload;
                  } 
               } /* End if not local */

               if (SendMsg)
               {

                  CfeStatus = CreateEdsOutputMessage(SocketBuffer, MsgPtr, SocketBufferLen, &EdsDataSize);     
                  if (CfeStatus == CFE_SUCCESS)
                  {
                     SocketStatus = OS_SocketSendTo(PktMgr->TlmSockId, SocketBuffer, EdsDataSize, &PktMgr->TlmSocketAddr);
                     ++NumPktsOutput;
                     NumBytesOutput += MsgLen;
                  }
                  else
                  {
                     
                     /*
                     ** EDS error strategy:
                     **    1. Disable individual packet output on first encoding error
                     **    2. Disable downlink after N total packet errors
                     */
                     
                     CFE_EVS_SendEvent(PKTMGR_OUTPUT_TLM_EID,CFE_EVS_EventType_ERROR,
                           "Error packing EDS output message %d, message length %ld",
                           CFE_SB_MsgIdToValue(MsgId),MsgLen);

                     PKTTBL_PKTTBL_DisablePacketOutput(&(PktMgr->PktTbl.Data.Pkt[MsgAppId]));
                     CFE_EVS_SendEvent(PKTMGR_OUTPUT_TLM_EID,CFE_EVS_EventType_INFORMATION,
                           "Disabled message %d's otuput by setting filter to 'always filtered'",MsgAppId);
                     
                     ++PktMgr->EdsEncodeErrCnt;
                     if (PktMgr->EdsEncodeErrCnt > PktMgr->EdsEncodeErrLim)
                     {
                        PktMgr->SuppressSend = true;
                        CFE_EVS_SendEvent(PKTMGR_OUTPUT_TLM_EID,CFE_EVS_EventType_ERROR,
                                          "Telemetry packet encoding errors exceeded %d, telemetry output suspended. Verify EDS library matches current code.",
                                          PktMgr->EdsEncodeErrLim);
                     }
                     
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
         
         PktMgr->SuppressSend = true;          
         CFE_EVS_SendEvent(PKTMGR_OUTPUT_TLM_EID,CFE_EVS_EventType_ERROR,
                           "Error sending MsgAppId %d (len %d) on socket %s, port %d, status %d. Tlm output suppressed",
                           MsgAppId, (int16)MsgLen, PktMgr->TlmDestIp, PktMgr->TlmUdpPort, SocketStatus);
      }
      
   } /* End if not SuppressSend */
   
   ComputeStats(NumPktsOutput, NumBytesOutput);
   
   return NumPktsOutput;
   
} /* End OutputTelemetry() */


/******************************************************************************
** Function: SubscribeNewPkt
**
*/
static int32 SubscribeNewPkt(PKTTBL_Pkt_t *NewPkt)
{

   int32 Status = CFE_SUCCESS;
   CFE_SB_MsgId_t MsgId = CFE_SB_ValueToMsgId(NewPkt->MsgId);
   int16 VirtualChannel = 2;
   
   // Don't subscribe to messages that get wrapped and forwarded because they'd immediately get forwarded to KIT_TO
   if (!CFE_SB_MsgId_Equal(MsgId, PktMgr->PubWrappedTlmMid))
   {
      if (CFE_SB_MsgId_Equal(MsgId, PktMgr->Vc1FileDataTlmMid))
      {
         VirtualChannel = 1;
         Status = CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(NewPkt->MsgId), PktMgr->Vc1Pipe, NewPkt->Qos, NewPkt->BufLim);         
      }
      else
      {
         Status = CFE_SB_SubscribeEx(CFE_SB_ValueToMsgId(NewPkt->MsgId), PktMgr->Vc2Pipe, NewPkt->Qos, NewPkt->BufLim);
      }
      CFE_EVS_SendEvent(PKTMGR_SUBSCRIBE_EID, CFE_EVS_EventType_DEBUG,
                        "Subscribed to message 0x%04X(%d) on virtual channel %d",
                        NewPkt->MsgId, NewPkt->MsgId, VirtualChannel); 
   }
   else
   {
      CFE_EVS_SendEvent(PKTMGR_SUBSCRIBE_EID, CFE_EVS_EventType_INFORMATION,
                        "Skip subscribing to tunnel message 0x%04X(%d)",NewPkt->MsgId, NewPkt->MsgId);    
   }

   return Status;

} /* End SubscribeNewPkt(() */


