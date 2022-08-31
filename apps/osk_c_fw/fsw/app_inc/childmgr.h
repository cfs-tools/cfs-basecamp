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
**    Manage an application's child task
**
**  Notes:
**    1. Provides a common structure for managing a child task. This design
**       attempts to strike a balance between adding value by supporting app
**       design patterns that alleviate duplicate "plumbing" code and provide
**       tested solutions for developers versus over complicating the situation
**       with obscure interfaces and forcing developers to use solutions that
**       don't optimize the solution to their particular problem.
**    2. Design rationale:
**       - Main apps own all app objects and should manage calls to them
**       - App objects may be aware of their execution environment so the they
**         may need to suspend execution but they shouldn't be tightly coupled
**         to the childmgr.
**       - Provide a few childmgr callback functions that can be used but don't
**         try to solve every conceivable scenario. These callbacks can serve
**         as starting points for developers to create their own functions.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _childmgr_
#define _childmgr_

/*
** Includes
*/

#include "osk_c_fw_cfg.h"
#include "cmdmgr.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define CHILDMGR_RUNTIME_ERR  0xC0000001  /* See cfe_error.h for fields */

#define CHILDMGR_SEM_INVALID  0xFFFFFFFF
#define CHILDMGR_MUTEX_NAME   "CHILDMGR_MUTEX_"   /* A number will be appended for each child task*/
#define CHILDMGR_CNTSEM_NAME  "CHILDMGR_CNTSEM_"  /* A number will be appended for each child task*/

/*
** Event Message IDs
*/

#define CHILDMGR_INIT_ERR_EID                    (CHILDMGR_BASE_EID +  0)
#define CHILDMGR_INIT_COMPLETE_EID               (CHILDMGR_BASE_EID +  1)
#define CHILDMGR_REG_INVALID_FUNC_CODE_EID       (CHILDMGR_BASE_EID +  2)
#define CHILDMGR_EMPTY_TASK_Q_EID                (CHILDMGR_BASE_EID +  3)
#define CHILDMGR_INVALID_Q_READ_IDX_EID          (CHILDMGR_BASE_EID +  4)
#define CHILDMGR_TAKE_SEM_FAILED_EID             (CHILDMGR_BASE_EID +  5)
#define CHILDMGR_INVOKE_CHILD_ERR_EID            (CHILDMGR_BASE_EID +  6)
#define CHILDMGR_Get_CHILD_ERR_EID               (CHILDMGR_BASE_EID +  7)
#define CHILDMGR_DISPATCH_UNUSED_FUNC_CODE_EID   (CHILDMGR_BASE_EID +  8)
#define CHILDMGR_RUNTIME_ERR_EID                 (CHILDMGR_BASE_EID +  9)
#define CHILDMGR_DEBUG_EID                       (CHILDMGR_BASE_EID + 10)



/**********************/
/** Type Definitions **/
/**********************/

/*
** Initialization Structure
*/

typedef struct
{
   
   const char *TaskName;
   uint32 StackSize;
   uint32 Priority;
   uint32 PerfId;
   
} CHILDMGR_TaskInit_t;

/*
** Command Queue
*/
 
typedef struct
{

   CFE_MSG_CommandHeader_t  CmdHeader;
   char  Payload[CHILDMGR_CMD_PAYLOAD_LEN];
   
} CHILDMGR_CmdQEntry_t;

typedef struct
{

   uint32  Mutex;
   uint8   WriteIndex;
   uint8   ReadIndex;
   uint8   Count;
   uint8   Spare;
   
   CHILDMGR_CmdQEntry_t  Entry[CHILDMGR_CMD_Q_ENTRIES];

} CHILDMGR_CmdQ_t;


typedef bool (*CHILDMGR_CmdFuncPtr_t) (void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);

/*
** Objects register their command functions so each command structure
** contains a pointer to the object's data and to the command function.
** The parent app's CMDMGR takes care of validating commands so command 
** attributes like data length are not needed here.
*/

/*
** Manage app objects function callback signature 
*/
struct CHILDMGR_Struct;
typedef bool (*CHILDMGR_TaskCallback_t) (struct CHILDMGR_Struct* ChildMgr);


/*
** Alternate command counters allow an individual command to have its own 
** counters. The class counters are not incremented for the command. This
** is useful when commands are issued from onboard apps and incrementing 
** the class command counters may be confusing to ground operations. 
*/
typedef struct
{

   bool     Enabled;  /* Use alternate cmd counters */            
   uint16   Valid;    /* Number of valid messages received since init or reset */
   uint16   Invalid;  /* Number of invalid messages received since init or reset */

} CHILDMGR_AltCnt_t;


typedef struct
{

   void*                  DataPtr;
   CHILDMGR_CmdFuncPtr_t  FuncPtr; 
   
   CHILDMGR_AltCnt_t      AltCnt;

} CHILDMGR_Cmd_t;

typedef struct CHILDMGR_Struct
{

   int32   RunStatus;
   uint32  PerfId;
   CFE_ES_TaskId_t  TaskId;
   uint32  WakeUpSemaphore;
   
   uint16  ValidCmdCnt;
   uint16  InvalidCmdCnt;

   CFE_MSG_FcnCode_t   CurrCmdCode;
   CFE_MSG_FcnCode_t   PrevCmdCode;

   CHILDMGR_Cmd_t  Cmd[CHILDMGR_CMD_FUNC_TOTAL];

   CHILDMGR_CmdQ_t CmdQ;
   
   CHILDMGR_TaskCallback_t TaskCallback;

} CHILDMGR_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: CHILDMGR_Constructor
**
** Notes:
**    1. This function must be called prior to any other functions being
**       called using the same cmdmgr instance.
**    2. For ChildTaskMainFunc, the caller can use one of the predefined 
**       functions that start with "ChildMgr_TaskMain" or supply their
**       own function. 
**    3. The AppMainCallback function can be NULL and is only used if the
**       ChildTaskMainFunc has a callback feature.
*/
int32 CHILDMGR_Constructor(CHILDMGR_Class_t* ChildMgr,
                           CFE_ES_ChildTaskMainFuncPtr_t ChildTaskMainFunc,
                           CHILDMGR_TaskCallback_t AppMainCallback,
                           CHILDMGR_TaskInit_t* TaskInit);


/******************************************************************************
** Function: CHILDMGR_InvokeChildCmd
** 
** Notes:
**   1. This command function is registered with the app's cmdmgr with all of
**      the function codes that use the child task to process the command.
*/
bool CHILDMGR_InvokeChildCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: CHILDMGR_PauseTask
** 
** Notes:
**   1. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
**
*/
bool CHILDMGR_PauseTask(uint16* TaskBlockCnt, uint16 TaskBlockLim, 
                        uint32 TaskBlockDelayMs, uint32 PerfId);


/******************************************************************************
** Function: CHILDMGR_RegisterFunc
**
*/
bool CHILDMGR_RegisterFunc(CHILDMGR_Class_t* ChildMgr,
                           uint16 FuncCode, void* ObjDataPtr,
                           CHILDMGR_CmdFuncPtr_t ObjFuncPtr);

            
/******************************************************************************
** Function: CHILDMGR_ResetStatus
**
*/
void CHILDMGR_ResetStatus(CHILDMGR_Class_t* ChildMgr);


/******************************************************************************
** Function: ChildMgr_TaskMainCallback
**
** Notes:
**    1. Function signature must comply with CFE_ES_ChildTaskMainFuncPtr_t
**
*/
void ChildMgr_TaskMainCallback(void);


/******************************************************************************
** Function: ChildMgr_TaskMainCmdDispatch
**
** Notes:
**    1. Function signature must comply with CFE_ES_ChildTaskMainFuncPtr_t
**
*/
void ChildMgr_TaskMainCmdDispatch(void);


#endif /* _childmgr_ */
