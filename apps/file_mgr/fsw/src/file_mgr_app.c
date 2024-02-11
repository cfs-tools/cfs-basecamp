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
**    Implement the File Manager application
**
**  Notes:
**    1. See header notes
**
*/

/*
** Includes
*/

#include <string.h>
#include "file_mgr_app.h"
#include "file_mgr_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ   (&(FileMgr.IniTbl))
#define  CMDMGR_OBJ   (&(FileMgr.CmdMgr))
#define  TBLMGR_OBJ   (&(FileMgr.TblMgr))
#define  CHILDMGR_OBJ (&(FileMgr.ChildMgr))
#define  DIR_OBJ      (&(FileMgr.Dir))
#define  FILE_OBJ     (&(FileMgr.File))
#define  FILESYS_OBJ  (&(FileMgr.FileSys))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendHousekeepingPkt(void);

/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum_t IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)  


/*****************/
/** Global Data **/
/*****************/

FILE_MGR_Class_t  FileMgr;


/******************************************************************************
** Function: FILE_MGR_AppMain
**
*/
void FILE_MGR_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;


   CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

   if (InitApp() == CFE_SUCCESS) /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN;  
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {

      RunStatus = ProcessCommands(); /* Pends indefinitely & manages CFE_ES_PerfLogEntry() calls */

   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("FILE_MGR App terminating, err = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(FILE_MGR_EXIT_EID, CFE_EVS_EventType_CRITICAL, "FILE_MGR App terminating, err = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of FILE_MGR_AppMain() */


/******************************************************************************
** Function: FILE_MGR_NoOpCmd
**
*/

bool FILE_MGR_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (FILE_MGR_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for FILE_MGR App version %d.%d.%d",
                      FILE_MGR_MAJOR_VER, FILE_MGR_MINOR_VER, FILE_MGR_PLATFORM_REV);

   return true;


} /* End FILE_MGR_NoOpCmd() */


/******************************************************************************
** Function: FILE_MGR_ResetAppCmd
**
*/

bool FILE_MGR_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   CHILDMGR_ResetStatus(CHILDMGR_OBJ);
   
   DIR_ResetStatus();
   FILE_ResetStatus();
   FILESYS_ResetStatus();
	  
   return true;

} /* End FILE_MGR_ResetAppCmd() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = APP_C_FW_CFS_ERROR;
   
   CHILDMGR_TaskInit_t ChildTaskInit;
   
   /*
   ** Initialize objects 
   */

   if (INITBL_Constructor(&FileMgr.IniTbl, FILE_MGR_INI_FILENAME, &IniCfgEnum))
   {
   
      FileMgr.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_MAIN_PERF_ID);
      FileMgr.CmdMid    = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_FILE_MGR_CMD_TOPICID));
      FileMgr.SendHkMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_FILE_MGR_SEND_HK_TOPICID));
      CFE_ES_PerfLogEntry(FileMgr.PerfId);

      /* Constructor sends error events */    
      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CHILD_TASK_PERF_ID);
      Status = CHILDMGR_Constructor(CHILDMGR_OBJ, 
                                    ChildMgr_TaskMainCmdDispatch,
                                    NULL, 
                                    &ChildTaskInit); 
  
   } /* End if INITBL Constructed */
  
   if (Status == CFE_SUCCESS)
   {

      DIR_Constructor(DIR_OBJ, &FileMgr.IniTbl);
      FILE_Constructor(FILE_OBJ, &FileMgr.IniTbl);
      FILESYS_Constructor(FILESYS_OBJ, &FileMgr.IniTbl);


      /*
      ** Initialize app level interfaces
      */
      
      CFE_SB_CreatePipe(&FileMgr.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(FileMgr.CmdMid,    FileMgr.CmdPipe);
      CFE_SB_Subscribe(FileMgr.SendHkMid, FileMgr.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,   NULL, FILE_MGR_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC,  NULL, FILE_MGR_ResetAppCmd, 0);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_CREATE_DIR_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_CreateDir_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_DELETE_DIR_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_DeleteDir_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_DELETE_ALL_DIR_CC,      CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_DeleteAllDir_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_SEND_DIR_LIST_TLM_CC,   CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_SendDirListTlm_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_SEND_DIR_TLM_CC,        CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_SendDirTlm_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_WRITE_DIR_LIST_FILE_CC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_WriteDirListFile_CmdPayload_t));
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_CREATE_DIR_CC,          DIR_OBJ, DIR_CreateCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_DELETE_DIR_CC,          DIR_OBJ, DIR_DeleteCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_DELETE_ALL_DIR_CC,      DIR_OBJ, DIR_DeleteAllCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_SEND_DIR_LIST_TLM_CC,   DIR_OBJ, DIR_SendDirListTlmCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_SEND_DIR_TLM_CC,        DIR_OBJ, DIR_SendDirTlmCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_WRITE_DIR_LIST_FILE_CC, DIR_OBJ, DIR_WriteListFileCmd);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_CONCATENATE_FILE_CC,     CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_ConcatenateFile_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_COPY_FILE_CC,            CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_CopyFile_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_DECOMPRESS_FILE_CC,      CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_DecompressFile_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_DELETE_FILE_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_DeleteFile_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_MOVE_FILE_CC,            CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_MoveFile_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_RENAME_FILE_CC,          CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_RenameFile_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_SEND_FILE_INFO_TLM_CC,   CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_SendFileInfoTlm_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_SET_FILE_PERMISSIONS_CC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_SetFilePermissions_CmdPayload_t));
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_CONCATENATE_FILE_CC,     FILE_OBJ, FILE_ConcatenateCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_COPY_FILE_CC,            FILE_OBJ, FILE_CopyCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_DECOMPRESS_FILE_CC,      FILE_OBJ, FILE_DecompressCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_DELETE_FILE_CC,          FILE_OBJ, FILE_DeleteCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_MOVE_FILE_CC,            FILE_OBJ, FILE_MoveCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_RENAME_FILE_CC,          FILE_OBJ, FILE_RenameCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_SEND_FILE_INFO_TLM_CC,   FILE_OBJ, FILE_SendInfoTlmCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_SET_FILE_PERMISSIONS_CC, FILE_OBJ, FILE_SetPermissionsCmd);

      /* 
      ** Alternative commands don't increment the main command counters. They do increment the child command counters which mimics
      ** the original FM app behavior, but I'm not sure that's desirable since the child counters are also used by ground ops.
      */
      CMDMGR_RegisterFuncAltCnt(CMDMGR_OBJ, FILE_MGR_DELETE_FILE_ALT_CC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(FILE_MGR_DeleteFile_CmdPayload_t));
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, FILE_MGR_DELETE_FILE_ALT_CC, FILE_OBJ, FILE_DeleteCmd);
 
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_SEND_OPEN_FILE_TLM_CC,     FILESYS_OBJ, FILESYS_SendOpenFileTlmCmd, CMDMGR_NO_PARAM_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_SEND_FILE_SYS_TBL_TLM_CC,  FILESYS_OBJ, FILESYS_SendTblTlmCmd,      CMDMGR_NO_PARAM_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, FILE_MGR_SET_FILE_SYS_TBL_STATE_CC, FILESYS_OBJ, FILESYS_SetTblStateCmd,     sizeof(FILE_MGR_SetFileSysTblState_CmdPayload_t));

      CFE_MSG_Init(CFE_MSG_PTR(FileMgr.HkPkt.TelemetryHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_FILE_MGR_HK_TLM_TOPICID)), sizeof(FILE_MGR_HkTlm_t));

      TBLMGR_Constructor(TBLMGR_OBJ, INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CFE_NAME));
   
      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(FILE_MGR_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "FILE_MGR App Initialized. Version %d.%d.%d",
                        FILE_MGR_MAJOR_VER, FILE_MGR_MINOR_VER, FILE_MGR_PLATFORM_REV);
                     
     
   } /* End if CHILDMGR constructed */
   
   return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
*/
static int32 ProcessCommands(void)
{

   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;

   CFE_SB_Buffer_t  *SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;
   

   CFE_ES_PerfLogExit(FileMgr.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, FileMgr.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(FileMgr.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
   
      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, FileMgr.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, FileMgr.SendHkMid))
         {  
            FILESYS_ManageTbl();
            SendHousekeepingPkt();
         }
         else
         {
            CFE_EVS_SendEvent(FILE_MGR_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%04X", 
                              CFE_SB_MsgIdToValue(MsgId));
         }

      }
      else
      {
         
         CFE_EVS_SendEvent(FILE_MGR_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                           "Failed to retrieve message ID from the SB message, Status = %d", SysStatus);
      }
      
   } /* Valid SB receive */ 
   else
   {
   
         CFE_ES_WriteToSysLog("FILE_MGR software bus error. Status = 0x%08X\n", SysStatus);   /* Use SysLog, events may not be working */
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
   }  
      
   return RetStatus;
   
} /* ProcessCommands() */


/******************************************************************************
** Function: SendHousekeepingPkt
**
*/
static void SendHousekeepingPkt(void)
{
   
   FILE_MGR_HkTlm_Payload_t *HkTlmPayload = &FileMgr.HkPkt.Payload;
   
   HkTlmPayload->ValidCmdCnt   = FileMgr.CmdMgr.ValidCmdCnt;
   HkTlmPayload->InvalidCmdCnt = FileMgr.CmdMgr.InvalidCmdCnt;

   HkTlmPayload->NumOpenFiles  = FileUtil_GetOpenFileCount();

   HkTlmPayload->ChildValidCmdCnt   = FileMgr.ChildMgr.ValidCmdCnt;
   HkTlmPayload->ChildInvalidCmdCnt = FileMgr.ChildMgr.InvalidCmdCnt;
   HkTlmPayload->ChildWarningCmdCnt = FileMgr.File.CmdWarningCnt + FileMgr.Dir.CmdWarningCnt;
 
   HkTlmPayload->ChildQueueCnt   = FileMgr.ChildMgr.CmdQ.Count;
   HkTlmPayload->ChildCurrentCC  = FileMgr.ChildMgr.CurrCmdCode;
   HkTlmPayload->ChildPreviousCC = FileMgr.ChildMgr.PrevCmdCode;

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(FileMgr.HkPkt.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(FileMgr.HkPkt.TelemetryHeader), true);
   
} /* End SendHousekeepingPkt() */

