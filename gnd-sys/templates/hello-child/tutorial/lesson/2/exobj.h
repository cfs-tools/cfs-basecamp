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
**    Define the EXOBJ_Class 
**
**  Notes:
**    None
**
*/
#ifndef _exobj_
#define _exobj_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define EXOBJ_TIME_STR_LEN  64
#define EXOBJ_STACK_ENTRIES  4

/*
** Event Message IDs
*/

#define EXOBJ_SET_CHILD_DELAY_CMD_EID  (EXOBJ_BASE_EID + 0)
#define EXOBJ_SET_COUNTER_MODE_CMD_EID (EXOBJ_BASE_EID + 1)
#define EXOBJ_EXECUTE_EID              (EXOBJ_BASE_EID + 2)
#define EXOBJ_STACK_PUSH_EID           (EXOBJ_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Command Packets
** - See EDS command definitions in @template@.xml
*/


/******************************************************************************
** EXOBJ_Class
*/

typedef struct
{
   
   uint32 CounterValue;
   char   TimeStr[EXOBJ_TIME_STR_LEN];
   
} EXOBJ_CounterStackEntry_t;

//EX1,18,1,
typedef struct
{

   /*
   ** State Data
   */
   
   @TEMPLATE@_CounterMode_Enum_t CounterMode;
   uint16                        CounterValue;
   uint16                        CounterStackIndex;
   EXOBJ_CounterStackEntry_t     CounterStack[EXOBJ_STACK_ENTRIES];

   uint16  CounterLoLim;
   uint16  CounterHiLim;  
   
   uint32  ChildTaskDelay;
   uint32  ChildDataSemaphore;
   uint32  ChildExecSemaphore;
   
   /*
   ** Contained Objects
   */

   
} EXOBJ_Class_t;
//EX1


/************************/
/** Exported Functions **/
/************************/

//EX2,12,1,
/******************************************************************************
** Function: EXOBJ_Constructor
**
** Initialize the example object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void EXOBJ_Constructor(EXOBJ_Class_t *ExObjPtr,
                       const INITBL_Class_t *IniTbl,
                       uint32 ChildExecSemaphore);
//EX2

/******************************************************************************
** Function: EXOBJ_ChildTask
**
** Notes:
**   1. This is designed to be used as CFE_ES_CreateChildTask()'s function
**
*/
void EXOBJ_ChildTask(void);


/******************************************************************************
** Function: EXOBJ_Execute_ChildTask
**
** Notes:
**   1. This is designed to be used with app_c_fw's ChildMgr service.
**   2. Returning false causes the child task to terminate.
**
*/
bool EXOBJ_Execute_ChildTask(CHILDMGR_Class_t *ChildMgr);


/******************************************************************************
** Function: EXOBJ_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void EXOBJ_ResetStatus(void);


/******************************************************************************
** Function: EXOBJ_SetChildDelayCmd
**
*/
bool EXOBJ_SetChildDelayCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: EXOBJ_SetCounterModeCmd
**
*/
bool EXOBJ_SetCounterModeCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: EXOBJ_StackPop
**
*/
bool EXOBJ_StackPop(EXOBJ_CounterStackEntry_t *CounterStackEntry);


#endif /* _exobj_ */
