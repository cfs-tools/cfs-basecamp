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
**    Implement the OSK C Demo application
**
**  Notes:
**    1. See header notes
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Includes
*/

#include <string.h>
#include "osk_c_demo_app.h"
#include "osk_c_demo_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ    (&(OskCDemo.IniTbl))
#define  CMDMGR_OBJ    (&(OskCDemo.CmdMgr))
#define  TBLMGR_OBJ    (&(OskCDemo.TblMgr))
#define  CHILDMGR_OBJ  (&(OskCDemo.ChildMgr))

#define  DEVICE_OBJ        (&(OskCDemo.Device))
#define  HISTOGRAM_OBJ     (&(OskCDemo.Histogram))
#define  HISTOGRAM_LOG_OBJ (&(OskCDemo.Histogram.Log))

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

OSK_C_DEMO_Class_t  OskCDemo;


/******************************************************************************
** Function: OSK_C_DEMO_AppMain
**
*/
void OSK_C_DEMO_AppMain(void)
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

   CFE_ES_WriteToSysLog("OSK_C_DEMO App terminating, run status = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(OSK_C_DEMO_EXIT_EID, CFE_EVS_EventType_CRITICAL, "OSK_C_DEMO App terminating, run status = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of OSK_C_DEMO_AppMain() */


/******************************************************************************
** Function: OSK_C_DEMO_NoOpCmd
**
*/
bool OSK_C_DEMO_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (OSK_C_DEMO_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for OSK_C_DEMO App version %d.%d.%d",
                      OSK_C_DEMO_MAJOR_VER, OSK_C_DEMO_MINOR_VER, OSK_C_DEMO_PLATFORM_REV);

   return true;


} /* End OSK_C_DEMO_NoOpCmd() */


/******************************************************************************
** Function: OSK_C_DEMO_ResetAppCmd
**
** Notes:
**   1. Framework objects require an object reference since they are
**      reentrant. Applications use the singleton pattern and store a
**      reference pointer to the object data during construction.
*/
bool OSK_C_DEMO_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   CHILDMGR_ResetStatus(CHILDMGR_OBJ);
   
   DEVICE_ResetStatus();
   HISTOGRAM_ResetStatus();
	  
   return true;

} /* End OSK_C_DEMO_ResetAppCmd() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 RetStatus = OSK_C_FW_CFS_ERROR;
   
   CHILDMGR_TaskInit_t ChildTaskInit;
   
   /*
   ** Read JSON INI Table & Initialize Child Manager  
   */
   
   if (INITBL_Constructor(INITBL_OBJ, OSK_C_DEMO_INI_FILENAME, &IniCfgEnum))
   {
   
      OskCDemo.PerfId  = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_PERF_ID);
      CFE_ES_PerfLogEntry(OskCDemo.PerfId);

      OskCDemo.CmdMid     = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_OSK_C_DEMO_CMD_TOPICID));
      OskCDemo.ExecuteMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_OSK_C_DEMO_EXE_TOPICID));

      /* Child Manager constructor sends error events */
      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_CHILD_NAME);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_PRIORITY);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CHILD_PERF_ID);

      RetStatus = CHILDMGR_Constructor(CHILDMGR_OBJ, 
                                       ChildMgr_TaskMainCmdDispatch,
                                       NULL, 
                                       &ChildTaskInit); 
  
   } /* End if INITBL Constructed */
  
   if (RetStatus == CFE_SUCCESS)
   {

      /* Must constructor table manager prior to any app objects that contain tables */
      TBLMGR_Constructor(TBLMGR_OBJ);

      /*
      ** Constuct app's contained objects
      */
           
      DEVICE_Constructor(DEVICE_OBJ, INITBL_GetIntConfig(INITBL_OBJ, CFG_DEVICE_DATA_MODULO));
      HISTOGRAM_Constructor(HISTOGRAM_OBJ, INITBL_OBJ, TBLMGR_OBJ);
      
      /*
      ** Initialize app level interfaces
      */
      
      CFE_SB_CreatePipe(&OskCDemo.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(OskCDemo.CmdMid,     OskCDemo.CmdPipe);
      CFE_SB_Subscribe(OskCDemo.ExecuteMid, OskCDemo.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_NOOP_CC,  NULL, OSK_C_DEMO_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_RESET_CC, NULL, OSK_C_DEMO_ResetAppCmd, 0);
      
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_LOAD_TBL_CC, TBLMGR_OBJ, TBLMGR_LoadTblCmd, TBLMGR_LOAD_TBL_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_DUMP_TBL_CC, TBLMGR_OBJ, TBLMGR_DumpTblCmd, TBLMGR_DUMP_TBL_CMD_DATA_LEN);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_START_HISTOGRAM_CC, HISTOGRAM_OBJ, HISTOGRAM_StartCmd, 0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_STOP_HISTOGRAM_CC,  HISTOGRAM_OBJ, HISTOGRAM_StopCmd,  0);

      /*
      ** The following commands are executed within the context of a child task. See the OSK App Dev Guide for details.
      */
      
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_START_HISTOGRAM_LOG_CC,        CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, sizeof(OSK_C_DEMO_StartHistogramLog_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_STOP_HISTOGRAM_LOG_CC,         CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, 0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_START_HISTOGRAM_LOG_PLAYBK_CC, CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, 0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, OSK_C_DEMO_STOP_HISTOGRAM_LOG_PLAYBK_CC,  CHILDMGR_OBJ, CHILDMGR_InvokeChildCmd, 0);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, OSK_C_DEMO_START_HISTOGRAM_LOG_CC,        HISTOGRAM_LOG_OBJ, HISTOGRAM_LOG_StartLogCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, OSK_C_DEMO_STOP_HISTOGRAM_LOG_CC,         HISTOGRAM_LOG_OBJ, HISTOGRAM_LOG_StopLogCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, OSK_C_DEMO_START_HISTOGRAM_LOG_PLAYBK_CC, HISTOGRAM_LOG_OBJ, HISTOGRAM_LOG_StartPlaybkCmd);
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ, OSK_C_DEMO_STOP_HISTOGRAM_LOG_PLAYBK_CC,  HISTOGRAM_LOG_OBJ, HISTOGRAM_LOG_StopPlaybkCmd);

      /* 
      ** Alternative commands don't increment the main command counters, but they do increment the child command counters.
      ** This "command" is used by the app's main loop to perform periodic processing 
      */
      CMDMGR_RegisterFuncAltCnt(CMDMGR_OBJ, OSK_C_DEMO_RUN_HISTOGRAM_LOG_CHILD_TASK_CC, CHILDMGR_OBJ,      CHILDMGR_InvokeChildCmd, sizeof(OSK_C_DEMO_RunHistogramLogChildTask_Payload_t));
      CHILDMGR_RegisterFunc(CHILDMGR_OBJ,   OSK_C_DEMO_RUN_HISTOGRAM_LOG_CHILD_TASK_CC, HISTOGRAM_LOG_OBJ, HISTOGRAM_LOG_RunChildTaskCmd);


      /*
      ** Initialize app messages 
      */


      CFE_MSG_Init(CFE_MSG_PTR(OskCDemo.RunHistogramLogChildTask.CommandBase), OskCDemo.CmdMid, sizeof(OSK_C_DEMO_RunHistogramLogChildTask_t));
      CFE_MSG_SetFcnCode(CFE_MSG_PTR(OskCDemo.RunHistogramLogChildTask.CommandBase), (CFE_MSG_FcnCode_t)OSK_C_DEMO_RUN_HISTOGRAM_LOG_CHILD_TASK_CC);

      CFE_MSG_Init(CFE_MSG_PTR(OskCDemo.StatusTlm.TelemetryHeader), 
                   CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_OSK_C_DEMO_STATUS_TLM_TOPICID)),
                   sizeof(OSK_C_DEMO_StatusTlm_t));

      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(OSK_C_DEMO_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "OSK_C_DEMO App Initialized. Version %d.%d.%d",
                        OSK_C_DEMO_MAJOR_VER, OSK_C_DEMO_MINOR_VER, OSK_C_DEMO_PLATFORM_REV);

   } /* End if CHILDMGR constructed */
   
   return RetStatus;

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
   uint16 BinNum;
   uint16 DataSample;
   CFE_SB_Buffer_t* SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;


   CFE_ES_PerfLogExit(OskCDemo.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, OskCDemo.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(OskCDemo.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);

      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, OskCDemo.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, OskCDemo.ExecuteMid))
         {

            DataSample = DEVICE_ReadData();
       

            if (HISTOGRAM_AddDataSample(DataSample, &BinNum))
            {

               OskCDemo.RunHistogramLogChildTask.Payload.BinNum     = BinNum;
               OskCDemo.RunHistogramLogChildTask.Payload.DataSample = DataSample;
               CFE_MSG_GenerateChecksum(CFE_MSG_PTR(OskCDemo.RunHistogramLogChildTask.CommandBase));
               CMDMGR_DispatchFunc(CMDMGR_OBJ, CFE_MSG_PTR(OskCDemo.RunHistogramLogChildTask.CommandBase));
            }
            SendStatusTlm();
         
         }
         else
         {   
            CFE_EVS_SendEvent(OSK_C_DEMO_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
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

   /* Good design practice in case app expands to more than one table */
   const TBLMGR_Tbl_t* LastTbl = TBLMGR_GetLastTblStatus(TBLMGR_OBJ);

   OSK_C_DEMO_StatusTlm_Payload_t *Payload = &OskCDemo.StatusTlm.Payload;
   
   /*
   ** Framework Data
   */
   
   Payload->ValidCmdCnt   = OskCDemo.CmdMgr.ValidCmdCnt;
   Payload->InvalidCmdCnt = OskCDemo.CmdMgr.InvalidCmdCnt;
   
   Payload->ChildValidCmdCnt   = OskCDemo.ChildMgr.ValidCmdCnt;
   Payload->ChildInvalidCmdCnt = OskCDemo.ChildMgr.InvalidCmdCnt;
   
   /*
   ** Table Data 
   ** - Loaded with status from the last table action 
   */

   Payload->LastTblAction       = LastTbl->LastAction;
   Payload->LastTblActionStatus = LastTbl->LastActionStatus;
          
   /*
   ** Device Data
   */

   Payload->DeviceData       = OskCDemo.Device.Data;
   Payload->DeviceDataModulo = OskCDemo.Device.DataMod;
   
   /*
   ** Histogram Data
   */

   Payload->HistEna       = OskCDemo.Histogram.Ena;
   Payload->HistMaxValue  = OskCDemo.Histogram.DataSampleMaxValue;
   Payload->HistSampleCnt = OskCDemo.Histogram.SampleCnt;
   strncpy(Payload->HistBinCntStr, OskCDemo.Histogram.BinCntStr, OS_MAX_PATH_LEN);
   
   /*
   ** Histogram Log Data
   */
   
   Payload->HistLogEna         = OskCDemo.Histogram.Log.Ena;
   Payload->HistLogBinNum      = OskCDemo.Histogram.Log.BinNum;
   Payload->HistLogCnt         = OskCDemo.Histogram.Log.Cnt;
   Payload->HistLogMaxEntries  = OskCDemo.Histogram.Log.MaxEntries;
   Payload->HistLogPlaybkEna   = OskCDemo.Histogram.Log.PlaybkEna;
   Payload->HistLogPlaybkCnt   = OskCDemo.Histogram.Log.PlaybkCnt;
   strncpy(Payload->HistLogFilename, OskCDemo.Histogram.Log.Filename, OS_MAX_PATH_LEN);

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(OskCDemo.StatusTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(OskCDemo.StatusTlm.TelemetryHeader), true);

} /* End SendStatusTlm() */

