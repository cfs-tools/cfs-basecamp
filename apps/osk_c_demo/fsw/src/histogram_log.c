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
**    Implement the HISTOGRAM_LOG_Class methods
**
**  Notes:
**    1. The log/playback functionality is for demonstration purposes and 
**       the logic is kept simple so users can focus on learning developing 
**       apps using the OSK C Framework.
**    2. Logging and playback can't be enabled at the same time. If a command
**       to start a playback is received when logging is in progress, the
**       logging will be stopped and a playback will be started. The same
**       occurs in reverse when a playback is in progress and a command to 
**       start a message log is received. Neither case is considered an error.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "histogram_log.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static void CreateFilename(uint16 BinNum);
static void LogDataSample(uint16 DataSample);
static void PlaybkSamples(void);
static void StopLog(void);
static void StopPlaybk(void);


/**********************/
/** Global File Data **/
/**********************/

static HISTOGRAM_LOG_Class_t  *HistogramLog = NULL;



/******************************************************************************
** Function: HISTOGRAM_LOG_Constructor
**
*/
void HISTOGRAM_LOG_Constructor(HISTOGRAM_LOG_Class_t *HistogramLogPtr, 
                               INITBL_Class_t *IniTbl)
{
 
   HistogramLog = HistogramLogPtr;

   CFE_PSP_MemSet((void*)HistogramLog, 0, sizeof(HISTOGRAM_LOG_Class_t));
 
   HistogramLog->FilePrefix    = INITBL_GetStrConfig(IniTbl, CFG_HIST_LOG_FILE_PREFIX);
   HistogramLog->FileExtension = INITBL_GetStrConfig(IniTbl, CFG_HIST_LOG_FILE_EXTENSION);
   
   CFE_MSG_Init(CFE_MSG_PTR(HistogramLog->BinPlaybkTlm.TelemetryHeader), 
                   CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_OSK_C_DEMO_BIN_PLAYBK_TLM_TOPICID)),
                   sizeof(OSK_C_DEMO_BinPlaybkTlm_t));

} /* End HISTOGRAM_LOG_Constructor */


/******************************************************************************
** Function: HISTOGRAM_LOG_RunChildTaskCmd
**
** Notes:
**   None
**
*/
bool HISTOGRAM_LOG_RunChildTaskCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const OSK_C_DEMO_RunHistogramLogChildTask_Payload_t *RunChildTask = CMDMGR_PAYLOAD_PTR(MsgPtr, OSK_C_DEMO_RunHistogramLogChildTask_t);


   CFE_EVS_SendEvent (HISTOGRAM_LOG_RUN_EID, CFE_EVS_EventType_DEBUG,
                      "HISTOGRAM_LOG_RunChildTaskCmd: Ena %d, BinNum %d, Commanded: BinNum %d, DataSample %d",
                      HistogramLog->Ena,  HistogramLog->BinNum,  
                      RunChildTask->BinNum, RunChildTask->DataSample);
   
   if (HistogramLog->Ena && RunChildTask->BinNum == HistogramLog->BinNum)
   {
   
      LogDataSample(RunChildTask->DataSample);
   
   } /* End if log in progress */
   else
   {
      if (HistogramLog->PlaybkEna)
      {
         PlaybkSamples();
      }
   }
   
   return true;

} /* End HISTOGRAM_LOG_RunChildTaskCmd() */


/******************************************************************************
** Function:  HISTOGRAM_LOG_ResetStatus
**
*/
void HISTOGRAM_LOG_ResetStatus()
{
 
   if (!HistogramLog->Ena)
   {
      HistogramLog->BinNum = 0;
      HistogramLog->Cnt    = 0;
   }
   
   if (!HistogramLog->PlaybkEna)
   {
      HistogramLog->PlaybkCnt   = 0;
   }
   
} /* End HISTOGRAM_LOG_ResetStatus() */


/******************************************************************************
** Function: HISTOGRAM_LOG_StartLogCmd
**
*/
bool HISTOGRAM_LOG_StartLogCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool   RetStatus = false;
   int32  SysStatus;
   os_err_name_t OsErrStr;
   const OSK_C_DEMO_StartHistogramLog_Payload_t *StartLog = CMDMGR_PAYLOAD_PTR(MsgPtr,OSK_C_DEMO_StartHistogramLog_t);
      
   if (StartLog->BinNum < HISTOGRAM_MAX_BINS)
   {
      
      if (HistogramLog->Ena)
      {
         StopLog();
      }
   
      if (HistogramLog->PlaybkEna)
      {
         StopPlaybk();
      }

      HistogramLog->Cnt        = 0;
      HistogramLog->BinNum     = StartLog->BinNum;
      HistogramLog->MaxEntries = StartLog->MaxEntries;
   
      CreateFilename(HistogramLog->BinNum);

      SysStatus = OS_OpenCreate(&HistogramLog->FileHandle, HistogramLog->Filename, OS_FILE_FLAG_CREATE, OS_READ_WRITE);

      if (SysStatus == OS_SUCCESS)
      {
         RetStatus = true;
         HistogramLog->Ena = true;
 
         CFE_EVS_SendEvent (HISTOGRAM_LOG_START_LOG_CMD_EID,
                            CFE_EVS_EventType_INFORMATION, 
                            "Created new log file %s with a maximum of %d entries",
                            HistogramLog->Filename, HistogramLog->MaxEntries);
      }
      else
      {
         OS_GetErrorName(SysStatus, &OsErrStr);
         CFE_EVS_SendEvent (HISTOGRAM_LOG_START_LOG_CMD_EID, CFE_EVS_EventType_ERROR, 
                            "Start histogram log rejected. Error creating new log file %s. Status = %s",
                            HistogramLog->Filename, OsErrStr);         
      }
   } /* End if valid BinNum */ 
   else
   {
      CFE_EVS_SendEvent (HISTOGRAM_LOG_START_LOG_CMD_EID, CFE_EVS_EventType_ERROR, 
                         "Start histogram log rejected. Commanded bin num %d exceeeds maximum bin num %d",
                         StartLog->BinNum, (HISTOGRAM_MAX_BINS-1));
   }
     
   return RetStatus;

} /* End HISTOGRAM_LOG_StartLogCmd() */


/******************************************************************************
** Function: HISTOGRAM_LOG_StartPlaybkCmd
**
*/
bool HISTOGRAM_LOG_StartPlaybkCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool   RetStatus = false;
   int32  SysStatus;

   os_err_name_t OsErrStr;
   FileUtil_FileInfo_t FileInfo;
   

   if (HistogramLog->Ena)
   {
      StopLog();
   }

   if (!HistogramLog->PlaybkEna)
   {
      
      if (HistogramLog->Cnt > 0)
      {

         FileInfo = FileUtil_GetFileInfo(HistogramLog->Filename, OS_MAX_PATH_LEN, false);

         if (FILEUTIL_FILE_EXISTS(FileInfo.State))
         {

            SysStatus = OS_OpenCreate(&HistogramLog->FileHandle, HistogramLog->Filename, OS_FILE_FLAG_NONE, OS_READ_ONLY);

            if (SysStatus == OS_SUCCESS)
            {
               RetStatus                 = true;
               HistogramLog->PlaybkEna   = true;
               HistogramLog->PlaybkCnt   = 0;
            
               CFE_EVS_SendEvent (HISTOGRAM_LOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_INFORMATION,
                                  "Started histogram log playback of file %s started", HistogramLog->Filename);
            }
            else
            {
               OS_GetErrorName(SysStatus, &OsErrStr);
               CFE_EVS_SendEvent (HISTOGRAM_LOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                                  "Start playback failed. Error opening file %s. Status = %s",
                                  HistogramLog->Filename, OsErrStr);
            }
         
         } /* End if file exists */
         else
         {
            CFE_EVS_SendEvent (HISTOGRAM_LOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                              "Start playback failed. Sample data log file does not exist");
         }
      
      } /* HistogramLog->Cnt > 0 */
      else
      {
         
         CFE_EVS_SendEvent (HISTOGRAM_LOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                            "Start playback failed. Sample data log count is zero");
      }
   } /* End if playback not in progress */ 
   else
   {

      CFE_EVS_SendEvent (HISTOGRAM_LOG_START_PLAYBK_CMD_EID, CFE_EVS_EventType_ERROR,
                         "Start playback ignored. Playback already in progress");

   }
   
   return RetStatus;
   
} /* End HISTOGRAM_LOG_StartPlaybkCmd() */


/******************************************************************************
** Function: HISTOGRAM_LOG_StopLogCmd
**
*/
bool HISTOGRAM_LOG_StopLogCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   if (HistogramLog->Ena)
   {
      StopLog();
   }
   else
   {
      CFE_EVS_SendEvent (HISTOGRAM_LOG_STOP_LOG_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Stop log command received with no log in progress");
   }
   
   return true;
   
} /* End HISTOGRAM_LOG_StopLogCmd() */


/******************************************************************************
** Function: HISTOGRAM_LOG_StopPlaybkCmd
**
*/
bool HISTOGRAM_LOG_StopPlaybkCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   if (HistogramLog->PlaybkEna)
   {
      StopPlaybk();
   }
   else
   {
      CFE_EVS_SendEvent (HISTOGRAM_LOG_STOP_LOG_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Stop playback command received with no playback in progress");
   }
   
   return true;
   
} /* End MSGLOG_StopPlaybkCmd() */


/******************************************************************************
** Function: CreateFilename
**
** Create a filename using the table-defined prefix & extension and the bin
** number function parameter.
**
** Notes:
**   1. No string buffer error checking performed
*/
static void CreateFilename(uint16 BinNum)
{
   
   int  i;
   char BinStr[16];

   CFE_EVS_SendEvent (HISTOGRAM_LOG_START_LOG_CMD_EID, CFE_EVS_EventType_DEBUG,
                      "CreateFilename: File prefix %s, Flle ext %s, Bin %d",
                      HistogramLog->FilePrefix, HistogramLog->FileExtension, BinNum); 

   sprintf(BinStr,"%02d",BinNum);

   strcpy (HistogramLog->Filename, HistogramLog->FilePrefix);

   i = strlen(HistogramLog->Filename);  /* Starting position for message ID */
   strcat (&(HistogramLog->Filename[i]), BinStr);
   
   i = strlen(HistogramLog->Filename);  /* Starting position for extension */
   strcat (&(HistogramLog->Filename[i]), HistogramLog->FileExtension);
   

} /* End CreateFilename() */


/******************************************************************************
** Functions: LogDataSample
**
** Notes:
**   1. The cFE time print format is yyyy-ddd-hh:mm:ss.xxxxx. The subseconds
**      are not logged.
*/
static void LogDataSample(uint16 DataSample)
{
      
   int    i;
   int32  SysStatus;
   char   LogText[HISTOGRAM_LOG_TEXT_LEN];
   char   DataSampleStr[16];
   os_err_name_t OsErrStr;

   sprintf(DataSampleStr,":%4d\n", DataSample);
   CFE_TIME_Print(LogText, CFE_TIME_GetTime());
   i = strlen(LogText)-6;                       /* Data sample Start position. Remove subsecs */
   strcpy(&(LogText[i]), DataSampleStr);
   
   SysStatus = OS_write(HistogramLog->FileHandle, LogText, strlen(LogText));

   if (SysStatus >= 0)
   {

      CFE_EVS_SendEvent (HISTOGRAM_LOG_RUN_LOG_EID, CFE_EVS_EventType_DEBUG,
                         "Log DataSample %d: %s", HistogramLog->Cnt, LogText);      
      HistogramLog->Cnt++;
      if (HistogramLog->Cnt >= HistogramLog->MaxEntries)
      {   
         StopLog();  
      }
   }
   else
   {
      OS_GetErrorName(SysStatus, &OsErrStr);
      CFE_EVS_SendEvent (HISTOGRAM_LOG_RUN_LOG_EID, CFE_EVS_EventType_ERROR, 
                         "Error writing data sample %s. Status = %s",
                         LogText, OsErrStr);         
   }
   
} /* End LogDataSample() */


/******************************************************************************
** Functions: PlaybkSamples
**
** Copy one time-stamped data sample into playback telemetry packet and send
** the telemetry packet. 
**
** Notes:
**   1. Uses log counter to determine when to wrap around to start of file
**      and not the end of file.
**   2. The Startlog command ensures at least one sample is in the log file.
**
*/
static void PlaybkSamples(void)
{
   
   if (HistogramLog->PlaybkCnt < HistogramLog->Cnt)
   {
   
      if (FileUtil_ReadLine(HistogramLog->FileHandle, HistogramLog->BinPlaybkTlm.Payload.DataSampleTxt, HISTOGRAM_LOG_TEXT_LEN))
      {

         CFE_EVS_SendEvent(HISTOGRAM_LOG_RUN_PLAYBK_EID, CFE_EVS_EventType_DEBUG,
                           "PlaybkSample %d: %s", 
                           HistogramLog->PlaybkCnt, HistogramLog->BinPlaybkTlm.Payload.DataSampleTxt);      
         
         HistogramLog->BinPlaybkTlm.Payload.LogFileEntry = HistogramLog->PlaybkCnt;
        
         CFE_SB_TimeStampMsg(CFE_MSG_PTR(HistogramLog->BinPlaybkTlm.TelemetryHeader));
         CFE_SB_TransmitMsg(CFE_MSG_PTR(HistogramLog->BinPlaybkTlm.TelemetryHeader), true);

         HistogramLog->PlaybkCnt++;
         if (HistogramLog->PlaybkCnt >= HistogramLog->Cnt)
         {
         
            HistogramLog->PlaybkCnt = 0;
            OS_lseek(HistogramLog->FileHandle, 0, OS_SEEK_SET);
         
         }
      }
      else
      {
      
         CFE_EVS_SendEvent (HISTOGRAM_LOG_RUN_PLAYBK_EID, CFE_EVS_EventType_ERROR,
                            "Error reading log file %s at playbk count %d", 
                            HistogramLog->Filename, HistogramLog->PlaybkCnt);
         StopPlaybk();
      }
      
   }
   else
   {
      CFE_EVS_SendEvent (HISTOGRAM_LOG_RUN_PLAYBK_EID, CFE_EVS_EventType_ERROR,
                        "Error reading log file %s at playbk count %d", 
                        HistogramLog->Filename, HistogramLog->PlaybkCnt);
      StopPlaybk();
   }

} /* End PlaybkSamples() */


/******************************************************************************
** Function: StopLog
**
** Notes:
**   1. Assumes caller checked if log was in progress
*/
static void StopLog(void)
{
   
   OS_close(HistogramLog->FileHandle);
   HistogramLog->Ena = false;
   
   CFE_EVS_SendEvent (HISTOGRAM_LOG_STOP_LOG_CMD_EID, CFE_EVS_EventType_INFORMATION,
                      "Closed log file %s with %d entries", 
                      HistogramLog->Filename, HistogramLog->Cnt);

}/* End StopLog() */


/******************************************************************************
** Function: StopPlaybk
**
** Notes:
**   1. Assumes caller checked if playback was in progress. 
**   2. Clears playback state data
*/
static void StopPlaybk(void)
{
   
   OS_close(HistogramLog->FileHandle);
   
   HistogramLog->PlaybkEna = false;
   HistogramLog->BinPlaybkTlm.Payload.LogFileEntry = 0;
   memset(HistogramLog->BinPlaybkTlm.Payload.DataSampleTxt, '\0', HISTOGRAM_LOG_TEXT_LEN);

   CFE_EVS_SendEvent (HISTOGRAM_LOG_STOP_PLAYBK_CMD_EID, CFE_EVS_EventType_INFORMATION,
                      "Playback stopped. Closed log file %s", HistogramLog->Filename);

} /* End StopPlaybk() */
