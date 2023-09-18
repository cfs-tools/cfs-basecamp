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
**    Define the @Template@ application
**
**  Notes:
**   1. This file was automatically generated by cFS Basecamp's app
**      creation tool. If you edit it, your changes will be lost if
**      a new app with the same name is created. 
**
*/

#ifndef _@template@_app_
#define _@template@_app_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/

//EX1
/*
** Events
*/

#define @TEMPLATE@_INIT_APP_EID    (@TEMPLATE@_BASE_EID + 0)
#define @TEMPLATE@_NOOP_EID        (@TEMPLATE@_BASE_EID + 1)
#define @TEMPLATE@_EXIT_EID        (@TEMPLATE@_BASE_EID + 2)
#define @TEMPLATE@_INVALID_MID_EID (@TEMPLATE@_BASE_EID + 3)
#define @TEMPLATE@_SET_PARAM_EID   (@TEMPLATE@_BASE_EID + 4)
//EX1

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
** - See EDS command definitions in @template@.xml
*/


/******************************************************************************
** Telmetery Packets
** - See EDS command definitions in @template@.xml
*/


/******************************************************************************
** @TEMPLATE@_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */ 
    
   INITBL_Class_t  IniTbl; 
   CMDMGR_Class_t  CmdMgr;
   
   /*
   ** Command Packets
   */

 
   /*
   ** Telemetry Packets
   */
   
   @TEMPLATE@_HkTlm_t  HkTlm;
   
   /*
   ** @TEMPLATE@ State & Contained Objects
   */ 
           
   uint32           PerfId;
   CFE_SB_PipeId_t  CmdPipe;
   CFE_SB_MsgId_t   CmdMid;
   CFE_SB_MsgId_t   SendHkMid;
   
} @TEMPLATE@_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern @TEMPLATE@_Class_t  @Template@;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: @TEMPLATE@_AppMain
**
*/
void @TEMPLATE@_AppMain(void);


/******************************************************************************
** Function: @TEMPLATE@_NoOpCmd
**
*/
bool @TEMPLATE@_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);

//EX2
/******************************************************************************
** Function: @TEMPLATE@_ResetAppCmd
**
*/
bool @TEMPLATE@_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);



/******************************************************************************
** Function: @TEMPLATE@_SetParam
**
*/
bool @TEMPLATE@_SetParamCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);
//EX2

#endif /* _@template@_ */
