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
**    Define the App C Demo application
**
**  Notes:
**    1. Demonstrates an application using the App C Framework. It also serves
**       as the final app that is developed during the Code-As-You-Go(CAYG)
**       app development tutorial.
**
*/

#ifndef _app_c_demo_app_
#define _app_c_demo_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "device.h"
#include "histogram.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define APP_C_DEMO_INIT_APP_EID    (APP_C_DEMO_BASE_EID + 0)
#define APP_C_DEMO_NOOP_EID        (APP_C_DEMO_BASE_EID + 1)
#define APP_C_DEMO_EXIT_EID        (APP_C_DEMO_BASE_EID + 2)
#define APP_C_DEMO_INVALID_MID_EID (APP_C_DEMO_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
** - See EDS command definitions in app_c_demo.xml
*/


/******************************************************************************
** Telmetery Packets
** - See EDS command definitions in app_c_demo.xml
*/


/******************************************************************************
** APP_C_DEMO_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */ 
    
   INITBL_Class_t    IniTbl; 
   CFE_SB_PipeId_t   CmdPipe;
   CMDMGR_Class_t    CmdMgr;
   TBLMGR_Class_t    TblMgr;
   CHILDMGR_Class_t  ChildMgr;
    
   /*
   ** Command Packets
   */

   APP_C_DEMO_RunHistogramLogChildTask_t  RunHistogramLogChildTask;

   /*
   ** Telemetry Packets
   */
   
   APP_C_DEMO_StatusTlm_t  StatusTlm;

   /*
   ** APP_C_DEMO State & Contained Objects
   */ 
           
   uint32            PerfId;
   CFE_SB_MsgId_t    CmdMid;
   CFE_SB_MsgId_t    ExecuteMid;
   
   DEVICE_Class_t    Device;
   HISTOGRAM_Class_t Histogram;
   
} APP_C_DEMO_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern APP_C_DEMO_Class_t  AppCDemo;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: APP_C_DEMO_AppMain
**
*/
void APP_C_DEMO_AppMain(void);


/******************************************************************************
** Function: APP_C_DEMO_NoOpCmd
**
*/
bool APP_C_DEMO_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: APP_C_DEMO_ResetAppCmd
**
*/
bool APP_C_DEMO_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _app_c_demo_ */
