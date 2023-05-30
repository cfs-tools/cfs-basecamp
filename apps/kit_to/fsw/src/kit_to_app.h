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
**     Define the OpenSatKit Telemetry Output application. This app
**     receives telemetry packets from the software bus and uses its
**     packet table to determine whether packets should be sent over
**     a UDP socket.
**
**  Notes:
**    None
**
**  References:
**    1. cFS Basecamp Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/
#ifndef _kit_to_app_
#define _kit_to_app_

/*
** Includes
*/

#include "cfe_msgids.h"
#include "app_cfg.h"
#include "pkttbl.h"
#include "pktmgr.h"
#include "evt_plbk.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define KIT_TO_APP_INIT_EID               (KIT_TO_APP_BASE_EID + 0)
#define KIT_TO_APP_INIT_ERR_EID           (KIT_TO_APP_BASE_EID + 1)
#define KIT_TO_APP_NOOP_EID               (KIT_TO_APP_BASE_EID + 2)
#define KIT_TO_APP_EXIT_EID               (KIT_TO_APP_BASE_EID + 3)
#define KIT_TO_APP_INVALID_MID_EID        (KIT_TO_APP_BASE_EID + 4)
#define KIT_TO_SET_RUN_LOOP_DELAY_EID     (KIT_TO_APP_BASE_EID + 5)
#define KIT_TO_INVALID_RUN_LOOP_DELAY_EID (KIT_TO_APP_BASE_EID + 6)
#define KIT_TO_DEMO_EID                   (KIT_TO_APP_BASE_EID + 7)
#define KIT_TO_TEST_FILTER_EID            (KIT_TO_APP_BASE_EID + 8)


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
** KIT_TO_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */
   
   INITBL_Class_t  IniTbl;
   CFE_SB_PipeId_t CmdPipe;
   CMDMGR_Class_t  CmdMgr;
   TBLMGR_Class_t  TblMgr;

   /*
   ** Telemetry Packets
   */
   
   KIT_TO_HkTlm_t         HkTlm;
   KIT_TO_DataTypesTlm_t  DataTypesTlm;


   /*
   ** KIT_CI State & Contained Objects
   */
   
   CFE_SB_MsgId_t  CmdMid;
   CFE_SB_MsgId_t  SendHkMid;
   
   uint16  RunLoopDelay;
   uint16  RunLoopDelayMin;
   uint16  RunLoopDelayMax;

   PKTTBL_Class_t    PktTbl;
   PKTMGR_Class_t    PktMgr;
   EVT_PLBK_Class_t  EvtPlbk;
   
} KIT_TO_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern KIT_TO_Class_t  KitTo;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: KIT_TO_AppMain
**
*/
void KIT_TO_AppMain(void);


/******************************************************************************
** Function: KIT_TO_NoOpCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_TO_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_TO_ResetAppCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_TO_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_TO_SendDataTypesTlmCmd
**
*/
bool KIT_TO_SendDataTypesTlmCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_TO_SetRunLoopDelayCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr_t definition
**
*/
bool KIT_TO_SetRunLoopDelayCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: KIT_TO_TestFilterCmd
**
*/
bool KIT_TO_TestPktFilterCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _kit_to_app_ */
