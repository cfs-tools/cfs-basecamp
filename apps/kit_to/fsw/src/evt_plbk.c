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
**    Provide a service to playback cFE_EVS event log file in telemetry.
**
**  Notes:
**    Nones
**
*/

/*
** Include Files:
*/

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "cfe_evs_eds_cc.h"
#include "cfe_evs_extern_typedefs.h"
#include "kit_to_eds_defines.h"

#include "evt_plbk.h"


/******************************/
/** File Function Prototypes **/
/******************************/

static void SendEventTlmMsg(void);
static bool LoadLogFile(void);


/**********************/
/** Global File Data **/
/**********************/

static EVT_PLBK_Class_t *EvtPlbk = NULL;

static CFE_EVS_WriteLogDataFileCmd_t WriteEvsLogFileCmd;


/******************************************************************************
** Function: EVT_PLBK_Constructor
**
*/
void EVT_PLBK_Constructor(EVT_PLBK_Class_t *EvtPlbkPtr, INITBL_Class_t *IniTbl)
{

   EvtPlbk = EvtPlbkPtr;

   memset ((void*)EvtPlbk, 0, sizeof(EVT_PLBK_Class_t));   /* Enabled set to FALSE */
   
   EvtPlbk->HkCyclePeriod = INITBL_GetIntConfig(IniTbl, CFG_EVT_PLBK_HK_PERIOD);
   strncpy(EvtPlbk->EventLogFile, INITBL_GetStrConfig(IniTbl, CFG_EVT_PLBK_LOG_FILE), CFE_MISSION_MAX_PATH_LEN);
   
   CFE_MSG_Init(CFE_MSG_PTR(EvtPlbk->Tlm.TelemetryHeader),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_EVENT_PLBK_TLM_TOPICID)),
                sizeof(KIT_TO_PlbkEventTlm_t));
   
   /* 
   ** Initialize the static fields in the 'Write Log to File' command. The filename
   ** and checksum are set prior to sending the command
   */
   CFE_MSG_Init(CFE_MSG_PTR(WriteEvsLogFileCmd.CommandBase),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_EVS_CMD_TOPICID)),
                sizeof(CFE_EVS_WriteLogDataFileCmd_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(WriteEvsLogFileCmd.CommandBase), CFE_EVS_WRITE_LOG_DATA_FILE_CC);
                                                              
} /* End EVT_PLBK_Constructor() */


/******************************************************************************
** Function: EVT_PLBK_ResetStatus
**
*/
void EVT_PLBK_ResetStatus(void)
{

   /* Nothing to do */

} /* End EVT_PLBK_ResetStatus() */


/******************************************************************************
** Function: EVT_PLBK_Execute
**
*/
void EVT_PLBK_Execute(void)
{

   CFE_TIME_SysTime_t  AttemptTime;
 
   if (EvtPlbk->Enabled)
   {

      if (EvtPlbk->LogFileCopied)
      {

         EvtPlbk->HkCycleCount++;
         
         if (EvtPlbk->HkCycleCount >= EvtPlbk->HkCyclePeriod)
         {
            
            SendEventTlmMsg();
            EvtPlbk->HkCycleCount = 0;
         
         }         
      } /* End if LogFileCopied */
      else
      {

         if (LoadLogFile())
         {
         
            EvtPlbk->LogFileCopied = true;
         
         }
         else
         {
         
            EvtPlbk->EvsLogFileOpenAttempts++;
            
            if (EvtPlbk->EvsLogFileOpenAttempts > 2)
            {
            
               AttemptTime = CFE_TIME_Subtract(CFE_TIME_GetTime(), EvtPlbk->StartTime);
               
               CFE_EVS_SendEvent(EVT_PLBK_LOG_READ_ERR_EID, CFE_EVS_EventType_ERROR, 
                                 "Failed to read event log file %s after %d attempts over %d seconds",
                                 WriteEvsLogFileCmd.Payload.LogFilename, (EvtPlbk->EvsLogFileOpenAttempts-1), AttemptTime.Seconds);
                                 
               EvtPlbk->Enabled = false;
       
            }
            
         } /* End if !LoadLogFile() */ 
         
      } /* End if !LogFileCopied */
   
   } /* End if enabled */
     
} /* EVT_PLBK_Execute() */


/******************************************************************************
** Function: EVT_PLBK_ConfigCmd
**
** - Configure the behavior of playbacks. See command parameters definitions
**   for details. 
** - Only verify filename is valid. CFE_EVS will perform checks regarding 
**   whether the log file can be created.
** - No limit check performed on HkCyclesPerPkt because no harmful affects it
**   unreasonable value sent. 
**
*/
bool EVT_PLBK_ConfigCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const KIT_TO_CfgEvtLogPlbk_CmdPayload_t *CfgEvtLogPlbk = CMDMGR_PAYLOAD_PTR(MsgPtr, KIT_TO_CfgEvtLogPlbk_t);
   bool  RetStatus = false;

   EvtPlbk->HkCyclePeriod = CfgEvtLogPlbk->HkCyclesPerPkt;

   if (FileUtil_VerifyFilenameStr(CfgEvtLogPlbk->EventLogFile))
   {
      
      strncpy(EvtPlbk->EventLogFile, CfgEvtLogPlbk->EventLogFile, CFE_MISSION_MAX_PATH_LEN);
   
      CFE_EVS_SendEvent(EVT_PLBK_CFG_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                        "Config playback command accepted with log file %s and HK period %d",
                        EvtPlbk->EventLogFile, CfgEvtLogPlbk->HkCyclesPerPkt);

      RetStatus = true;
      
   }
   else
   {
      
      CFE_EVS_SendEvent(EVT_PLBK_CFG_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Config playback command rejected, invalid filename %s",
                        WriteEvsLogFileCmd.Payload.LogFilename);

   
   }
   
   return RetStatus;

} /* End of EVT_PLBK_ConfigCmd() */


/******************************************************************************
** Function: EVT_PLBK_StartCmd
**
** Remove log file if it exists because the playback logic checks to see if the
** log exists and don't want an old playback file confusing the logic. 
*/
bool EVT_PLBK_StartCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   FileUtil_FileInfo_t FileInfo;

   FileInfo = FileUtil_GetFileInfo(EvtPlbk->EventLogFile, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == APP_C_FW_FileState_FILE_CLOSED)
   {
      OS_remove(EvtPlbk->EventLogFile);
   }
   
   strncpy(WriteEvsLogFileCmd.Payload.LogFilename, EvtPlbk->EventLogFile, CFE_MISSION_MAX_PATH_LEN);
   
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(WriteEvsLogFileCmd));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(WriteEvsLogFileCmd), true);

   EvtPlbk->StartTime = CFE_TIME_GetTime();   

   EvtPlbk->Enabled = true;
   EvtPlbk->HkCycleCount = 0;

   EvtPlbk->LogFileCopied = false;
   EvtPlbk->EvsLogFileOpenAttempts = 0;
   
   CFE_EVS_SendEvent(EVT_PLBK_SENT_WRITE_LOG_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                     "Commanded CFE_EVS to write event log to %s. Event tlm HK period = %d",
                     WriteEvsLogFileCmd.Payload.LogFilename, EvtPlbk->HkCyclePeriod);
   
   return true;
   
} /* End EVT_PLBK_StartCmd() */


/******************************************************************************
** Function: EVT_PLBK_StopCmd
**
*/
bool EVT_PLBK_StopCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   EvtPlbk->Enabled = false;
   EvtPlbk->LogFileCopied = false;
   EvtPlbk->HkCycleCount = 0;
   
   CFE_EVS_SendEvent(EVT_PLBK_STOP_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                     "Event playback stopped");
   
   return true;
   
} /* End EVT_PLBK_StopCmd() */


/******************************************************************************
** Function: LoadLogFile()
**
*/
static bool LoadLogFile(void)
{

   bool      RetStatus = false;
   bool      ReadingFile;
   uint16    i;
   osal_id_t FileHandle;
   int32     FileStatus;
   int32     ReadLength;
   os_err_name_t           OsErrStr;
   KIT_TO_PlbkEvent_t      *PlbkTlmEvent;
   FileUtil_FileInfo_t     FileInfo;
   CFE_FS_Header_t         CfeHeader;
   CFE_EVS_LongEventTlm_t  EvsLogEventTlm;
   const CFE_EVS_LongEventTlm_Payload_t *EvsLogEvent;


   FileInfo = FileUtil_GetFileInfo(EvtPlbk->EventLogFile, OS_MAX_PATH_LEN, false);
    
   if (FILEUTIL_FILE_EXISTS(FileInfo.State))
   {

      FileStatus = OS_OpenCreate(&FileHandle, EvtPlbk->EventLogFile, OS_FILE_FLAG_NONE, OS_READ_ONLY);

      if (FileStatus == OS_SUCCESS)
      {
      
         FileStatus = CFE_FS_ReadHeader(&CfeHeader, FileHandle);
         
         if (FileStatus == sizeof(CFE_FS_Header_t))
         {
            
            if (CfeHeader.SubType == CFE_FS_SubType_EVS_EVENTLOG)
            {
                        
               /* 
               ** Event log file:
               ** - Contains full event message with CCSDS header
               ** - Only contains actual events, i.e. no null entries to pad to max entries
               */
               ReadingFile = true;
               for (i=0; ((i < CFE_PLATFORM_EVS_LOG_MAX) && ReadingFile); i++)
               {
               
                  ReadLength = OS_read(FileHandle, &EvsLogEventTlm, sizeof(CFE_EVS_LongEventTlm_t));
                  if (ReadLength == sizeof(CFE_EVS_LongEventTlm_t))
                  {
                     
                     PlbkTlmEvent = &EvtPlbk->EventLog.Entry[i].Event;
                     EvsLogEvent  = &EvsLogEventTlm.Payload;

                     CFE_MSG_GetMsgTime(CFE_MSG_PTR(EvsLogEventTlm),&PlbkTlmEvent->Time);
                     PlbkTlmEvent->PacketID.EventID   = EvsLogEvent->PacketID.EventID;
                     PlbkTlmEvent->PacketID.EventType = EvsLogEvent->PacketID.EventType;
                     strncpy(PlbkTlmEvent->PacketID.AppName, EvsLogEvent->PacketID.AppName, CFE_MISSION_MAX_API_LEN);
                     strncpy(PlbkTlmEvent->Message, EvsLogEvent->Message, CFE_MISSION_EVS_MAX_MESSAGE_LENGTH);
                     EvtPlbk->EventLog.Entry[i].Loaded = true;
                  
                  }
                  else
                  {
                     ReadingFile = false;               
                  }

               } /* End file read loop */

            } /* End if valid file header subtype */
            else
            {
                  
               CFE_EVS_SendEvent(EVT_PLBK_LOG_HDR_TYPE_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "Invalid file header subtype %d for event log file %s", 
                                 CfeHeader.SubType, EvtPlbk->EventLogFile);
                                    
            } /* End if invalid file header subtype */
       
         } /* End if read file header */
         else
         {
            
            CFE_EVS_SendEvent(EVT_PLBK_LOG_HDR_READ_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Error reading event log %s file header. Return status = 0x%08X",
                              EvtPlbk->EventLogFile, FileStatus);
         
         } /* End if file header read error */

         FileStatus = OS_close(FileHandle);
         
         if (!ReadingFile)
         {
            
            i--;
            EvtPlbk->EventLog.EventCnt = i;
            
            while (i < CFE_PLATFORM_EVS_LOG_MAX)
            {
               
               EvtPlbk->EventLog.Entry[i].Loaded = false;
               PlbkTlmEvent = &EvtPlbk->EventLog.Entry[i].Event;
               PlbkTlmEvent->Time.Seconds    = 0;
               PlbkTlmEvent->Time.Subseconds = 0;
               PlbkTlmEvent->PacketID.EventID   = 0;
               PlbkTlmEvent->PacketID.EventType = 0;
               strncpy(PlbkTlmEvent->PacketID.AppName, "UNDEF", CFE_MISSION_MAX_API_LEN);
               strncpy(PlbkTlmEvent->Message, "UNDEF", CFE_MISSION_EVS_MAX_MESSAGE_LENGTH);

               i++;
               
            } /* End filler loop */ 
         } /* End if !ReadingFile */ 
         else
         {
            EvtPlbk->EventLog.EventCnt = i;
         }
        
         EvtPlbk->EventLog.PlbkIdx = 0;  
         RetStatus = true;

         /* Load telemetry that is fixed for each playback session */         
         strncpy(EvtPlbk->Tlm.Payload.EventLogFile, EvtPlbk->EventLogFile, CFE_MISSION_MAX_PATH_LEN);
         EvtPlbk->Tlm.Payload.EventCnt = EvtPlbk->EventLog.EventCnt;

         CFE_EVS_SendEvent(EVT_PLBK_READ_LOG_SUCCESS_EID, CFE_EVS_EventType_INFORMATION,
                           "Successfully loaded %d event messages from %s",
                           EvtPlbk->EventLog.EventCnt, EvtPlbk->EventLogFile);
         
      } /* End if open file */
      else
      {
         
         OS_GetErrorName(FileStatus, &OsErrStr);
         CFE_EVS_SendEvent(EVT_PLBK_LOG_OPEN_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Error opening event log file %s. Status = %s",
                           EvtPlbk->EventLogFile, OsErrStr);
         
      } /* End if open failed */
   } /* End if file exists */
   else
   {
   
      CFE_EVS_SendEvent(EVT_PLBK_LOG_NONEXISTENT_EID, CFE_EVS_EventType_ERROR, 
                        "Event log file %s doesn't exist", EvtPlbk->EventLogFile);
   
   } /* End if log file non-existent */
   
   return RetStatus;
   
} /* End LoadLogFile() */


/******************************************************************************
** Function: SendEventTlmMsg()
**
** Notes:
**   1. The log filename and event count are loaded once when the playback is
**      started.
*/
static void SendEventTlmMsg(void)
{

   uint16 i;
   const KIT_TO_PlbkEvent_t *EventLogEntry;
   KIT_TO_PlbkEvent_t       *EventTlmEntry;
      
   for (i=0; i < KIT_TO_PLBK_EVENTS_PER_TLM_MSG; i++)
   {
   
      if (EvtPlbk->EventLog.PlbkIdx >= CFE_PLATFORM_EVS_LOG_MAX) EvtPlbk->EventLog.PlbkIdx = 0;
      if (i==0) EvtPlbk->Tlm.Payload.PlbkIdx = EvtPlbk->EventLog.PlbkIdx;
      
      EventLogEntry = &EvtPlbk->EventLog.Entry[EvtPlbk->EventLog.PlbkIdx].Event;
      EventTlmEntry = &EvtPlbk->Tlm.Payload.Event[i];
      
      EventTlmEntry->Time = EventLogEntry->Time;
      EventTlmEntry->PacketID.EventID   = EventLogEntry->PacketID.EventID;
      EventTlmEntry->PacketID.EventType = EventLogEntry->PacketID.EventType;
      strncpy(EventTlmEntry->PacketID.AppName, EventLogEntry->PacketID.AppName, CFE_MISSION_MAX_API_LEN);
      strncpy(EventTlmEntry->Message, EventLogEntry->Message, CFE_MISSION_EVS_MAX_MESSAGE_LENGTH);
                  
      EvtPlbk->EventLog.PlbkIdx++;

   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(EvtPlbk->Tlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(EvtPlbk->Tlm.TelemetryHeader), true);

} /* End SendEventTlmMsg() */
