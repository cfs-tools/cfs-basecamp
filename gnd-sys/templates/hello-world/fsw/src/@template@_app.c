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
**    Implement the @Template@ application
**
**  Notes:
**   1. This file was automatically generated by cFS Basecamp's app
**      creation tool. If you edit it, your changes will be lost if
**      a new app with the same name is created. 
**
*/


/*
** Includes
*/

#include <string.h>
#include "@template@_app.h"
#include "@template@_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ    (&(@Template@.IniTbl))
#define  CMDMGR_OBJ    (&(@Template@.CmdMgr))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendStatusTlm(void);


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

@TEMPLATE@_Class_t  @Template@;


/******************************************************************************
** Function: @TEMPLATE@_AppMain
**
*/
void @TEMPLATE@_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;
   
   CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

   if (InitApp() == CFE_SUCCESS)      /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN; 
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {
      
      RunStatus = ProcessCommands();  /* Pends indefinitely & manages CFE_ES_PerfLogEntry() calls */
      
   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("@TEMPLATE@ App terminating, run status = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(@TEMPLATE@_EXIT_EID, CFE_EVS_EventType_CRITICAL, "@TEMPLATE@ App terminating, run status = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of @TEMPLATE@_AppMain() */


/******************************************************************************
** Function: @TEMPLATE@_NoOpCmd
**
*/
bool @TEMPLATE@_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (@TEMPLATE@_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for @TEMPLATE@ App version %d.%d.%d",
                      @TEMPLATE@_MAJOR_VER, @TEMPLATE@_MINOR_VER, @TEMPLATE@_PLATFORM_REV);

   return true;


} /* End @TEMPLATE@_NoOpCmd() */


/******************************************************************************
** Function: @TEMPLATE@_ResetAppCmd
**
** Notes:
**   1. Framework objects require an object reference since they are
**      reentrant. Applications use the singleton pattern and store a
**      reference pointer to the object data during construction.
*/
bool @TEMPLATE@_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
     
   return true;

} /* End @TEMPLATE@_ResetAppCmd() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = APP_C_FW_CFS_ERROR;
   

   /*
   ** Initialize objects 
   */
   
   if (INITBL_Constructor(INITBL_OBJ, @TEMPLATE@_INI_FILENAME, &IniCfgEnum))
   {
   
      @Template@.PerfId  = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_PERF_ID);
      CFE_ES_PerfLogEntry(@Template@.PerfId);

      @Template@.CmdMid         = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_@TEMPLATE@_CMD_TOPICID));
      @Template@.SendStatusMid  = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_@TEMPLATE@_SEND_STATUS_TOPICID));
      
      /*
      ** Constuct app's contained objects
      */
            
      
      /*
      ** Initialize app level interfaces
      */
      
      CFE_SB_CreatePipe(&@Template@.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(@Template@.CmdMid,        @Template@.CmdPipe);
      CFE_SB_Subscribe(@Template@.SendStatusMid, @Template@.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, @TEMPLATE@_NOOP_CC,  NULL, @TEMPLATE@_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, @TEMPLATE@_RESET_CC, NULL, @TEMPLATE@_ResetAppCmd, 0);
      
      /*
      ** Initialize app messages 
      */

      CFE_MSG_Init(CFE_MSG_PTR(@Template@.StatusTlm.TelemetryHeader), 
                   CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_@TEMPLATE@_STATUS_TLM_TOPICID)),
                   sizeof(@TEMPLATE@_StatusTlm_t));

      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(@TEMPLATE@_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "@TEMPLATE@ App Initialized. Version %d.%d.%d",
                        @TEMPLATE@_MAJOR_VER, @TEMPLATE@_MINOR_VER, @TEMPLATE@_PLATFORM_REV);

      Status = CFE_SUCCESS; 

   } /* End if INITBL constructed */
   
   return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
** 
*/
static int32 ProcessCommands(void)
{
   
   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;

   CFE_SB_Buffer_t  *SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;


   CFE_ES_PerfLogExit(@Template@.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, @Template@.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(@Template@.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);

      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, @Template@.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, @Template@.SendStatusMid))
         {   
            SendStatusTlm();
         }
         else
         {   
            CFE_EVS_SendEvent(@TEMPLATE@_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%08X", 
                              CFE_SB_MsgIdToValue(MsgId));
         }

      } /* End if got message ID */
   } /* End if received buffer */
   else
   {
      RetStatus = CFE_ES_RunStatus_APP_ERROR;
   } 

   return RetStatus;
   
} /* End ProcessCommands() */


/******************************************************************************
** Function: SendStatusTlm
**
*/
static void SendStatusTlm(void)
{

   @TEMPLATE@_StatusTlm_Payload_t *Payload = &@Template@.StatusTlm.Payload;

   /*
   ** Framework Data
   */
   
   Payload->ValidCmdCnt   = @Template@.CmdMgr.ValidCmdCnt;
   Payload->InvalidCmdCnt = @Template@.CmdMgr.InvalidCmdCnt;
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(@Template@.StatusTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(@Template@.StatusTlm.TelemetryHeader), true);

} /* End SendStatusTlm() */

