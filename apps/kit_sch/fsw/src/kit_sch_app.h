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
**    Define the Scheduler application
**
**  Notes:
**    1. The scheduler object owns the message and scheduler tables
**       so it provides the table load/dump command functions.
**
*/

#ifndef _kit_sch_app_
#define _kit_sch_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "scheduler.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define KIT_SCH_APP_NOOP_EID    (KIT_SCH_APP_BASE_EID + 0)
#define KIT_SCH_APP_INIT_EID    (KIT_SCH_APP_BASE_EID + 1)
#define KIT_SCH_APP_EXIT_EID    (KIT_SCH_APP_BASE_EID + 2)
#define KIT_SCH_APP_MID_ERR_EID (KIT_SCH_APP_BASE_EID + 3)
#define KIT_SCH_APP_DEBUG_EID   (KIT_SCH_APP_BASE_EID + 4)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
** - See EDS command definitions in kit_sch.xml
*/


/******************************************************************************
** Telmetery Packets
** - See EDS command definitions in kit_sch.xml
*/


/******************************************************************************
** KIT_SCH_Class
*/
typedef struct
{
   
   /* 
   ** App Framework
   */
   
   INITBL_Class_t   IniTbl; 
   CFE_SB_PipeId_t  CmdPipe;
   CMDMGR_Class_t   CmdMgr;
   TBLMGR_Class_t   TblMgr;

   
   /*
   ** Telemetry Packets
   */
   
   KIT_SCH_HkTlm_t  HkTlm;


   /*
   ** KIT_SCH State & Contained Objects
   */
   
   uint32   StartupSyncTimeout;
   CFE_SB_MsgId_t   CmdMid;
   CFE_SB_MsgId_t   SendHkMid;
   
   SCHEDULER_Class_t  Scheduler;
  
} KIT_SCH_Class;


/*******************/
/** Exported Data **/
/*******************/

extern KIT_SCH_Class  KitSch;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: KIT_SCH_Main
**
*/
void KIT_SCH_Main(void);


/******************************************************************************
** Function: KIT_SCH_NoOpCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_SCH_NoOpCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_SCH_ResetAppCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_SCH_ResetAppCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _kit_sch_app_ */

