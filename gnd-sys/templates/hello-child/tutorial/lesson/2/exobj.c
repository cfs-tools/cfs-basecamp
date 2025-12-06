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
**    Implement the EXOBJ_Class methods 
**
**  Notes:
**    None
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "exobj.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define MIN_CHILD_DELAY_MS 10  // Minimum child task loop delay to prevent CPU hogging  

/*******************************/
/** Local Function Prototypes **/
/*******************************/

static const char *CounterModeStr(@TEMPLATE@_CounterMode_Enum_t CounterMode);
static bool StackPush(uint16 CounterValue, const char *TimeStr);
static void ManageCounter(void);


/**********************/
/** Global File Data **/
/**********************/

static EXOBJ_Class_t *ExObj = NULL;

//EX1,6,14
/******************************************************************************
** Function: EXOBJ_Constructor
**
*/
void EXOBJ_Constructor(EXOBJ_Class_t *ExObjPtr,
                       const INITBL_Class_t *IniTbl,
                       uint32 ChildExecSemaphore)
{
 
   ExObj = ExObjPtr;

   CFE_PSP_MemSet((void*)ExObj, 0, sizeof(EXOBJ_Class_t));
    
   ExObj->CounterMode  = @TEMPLATE@_CounterMode_Increment;
   ExObj->CounterLoLim = INITBL_GetIntConfig(IniTbl, CFG_EXOBJ_COUNTER_LO_LIM);
   ExObj->CounterHiLim = INITBL_GetIntConfig(IniTbl, CFG_EXOBJ_COUNTER_HI_LIM);
   ExObj->CounterValue = ExObj->CounterLoLim;
   
   ExObj->ChildExecSemaphore = ChildExecSemaphore;
   OS_MutSemCreate(&ExObj->ChildDataSemaphore, "HELLO_CHILD_DATA", 0);
   ExObj->ChildTaskDelay = INITBL_GetIntConfig(IniTbl, CFG_CHILD_DELAY);
   
   
} /* End EXOBJ_Constructor */
//EX1

//EX2,13,1,
/******************************************************************************
** Function: EXOBJ_ChildTask
**
** Notes:
**   1. This is designed to be used as CFE_ES_CreateChildTask()'s function
**
*/
void EXOBJ_ChildTask(void)
{

   while (true)
   {
      OS_CountSemTake(ExObj->ChildExecSemaphore); // Pend until parent app gives semaphore 
      ManageCounter();
   }
   
} /* End EXOBJ_ChildTask() */
//EX2


/******************************************************************************
** Function: EXOBJ_Execute_ChildTask
**
** Notes:
**   1. This is designed to be used with app_c_fw's ChildMgr service.
**   2. Returning false causes the child task to terminate.
**
*/
bool EXOBJ_Execute_ChildTask(CHILDMGR_Class_t *ChildMgr)
{

   OS_TaskDelay(ExObj->ChildTaskDelay);
   ManageCounter();
   
   return true;
   
} /* End EXOBJ_Execute_ChildTask() */


/******************************************************************************
** Function:  EXOBJ_ResetStatus
**
*/
void EXOBJ_ResetStatus()
{
 
   return;
   
} /* End EXOBJ_ResetStatus() */


/******************************************************************************
** Function: EXOBJ_SetChildDelayCmd
**
*/
bool EXOBJ_SetChildDelayCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool    RetStatus = false;
   uint32  PrevDelay = ExObj->ChildTaskDelay;

   const @TEMPLATE@_SetChildDelay_CmdPayload_t *Cmd = CMDMGR_PAYLOAD_PTR(MsgPtr, @TEMPLATE@_SetChildDelay_t);
   
   // Too low of delay would hog system
   if (Cmd->Delay >= MIN_CHILD_DELAY_MS)
   {
      RetStatus = true;
      ExObj->ChildTaskDelay = Cmd->Delay;
      CFE_EVS_SendEvent (EXOBJ_SET_CHILD_DELAY_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Child task loop delay changed from %d to %d",
                         PrevDelay, ExObj->ChildTaskDelay);
   }
   else
   {
      CFE_EVS_SendEvent (EXOBJ_SET_CHILD_DELAY_CMD_EID, CFE_EVS_EventType_ERROR,
                         "Set child task loop delay command rejected. Commanded delay %d is less than minimum delay %d",
                         Cmd->Delay, MIN_CHILD_DELAY_MS);
   }
   
   return RetStatus;

} /* End EXOBJ_SetChildDelayCmd() */


/******************************************************************************
** Function: EXOBJ_SetCounterModeCmd
**
*/
bool EXOBJ_SetCounterModeCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool  RetStatus = false;

   const @TEMPLATE@_SetCounterMode_CmdPayload_t *Cmd = CMDMGR_PAYLOAD_PTR(MsgPtr, @TEMPLATE@_SetCounterMode_t);
   @TEMPLATE@_CounterMode_Enum_t PrevMode = ExObj->CounterMode;
   
   if ((Cmd->Mode == @TEMPLATE@_CounterMode_Increment) ||
       (Cmd->Mode == @TEMPLATE@_CounterMode_Decrement))
   {
   
      RetStatus = true; 
      ExObj->CounterMode = Cmd->Mode;
      
      CFE_EVS_SendEvent (EXOBJ_SET_COUNTER_MODE_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Counter mode changed from %s to %s. Limits: Low=%d, High=%d",
                         CounterModeStr(PrevMode), CounterModeStr(ExObj->CounterMode),
                         ExObj->CounterLoLim, ExObj->CounterHiLim);

   }
   else
   {
      CFE_EVS_SendEvent (EXOBJ_SET_COUNTER_MODE_CMD_EID, CFE_EVS_EventType_ERROR,
                         "Set counter mode rejected. Invalid mode %d",
                         Cmd->Mode);
   }
   
   return RetStatus;

} /* End EXOBJ_SetCounterModeCmd() */


/******************************************************************************
** Function: EXOBJ_StackPop
**
*/
bool EXOBJ_StackPop(EXOBJ_CounterStackEntry_t *CounterStackEntry)
{
   
   bool RetStatus = false;
   
   OS_MutSemTake(ExObj->ChildDataSemaphore);
   if (ExObj->CounterStackIndex != 0)
   {
      ExObj->CounterStackIndex--;
      *CounterStackEntry = ExObj->CounterStack[ExObj->CounterStackIndex];
      RetStatus = true;
   }
   OS_MutSemGive(ExObj->ChildDataSemaphore);

   return RetStatus;
   
} /* End EXOBJ_StackPop() */


/******************************************************************************
** Function: CounterModeStr
**
** Type checking should enforce valid parameter
*/
static const char *CounterModeStr(@TEMPLATE@_CounterMode_Enum_t  CounterMode)
{

   static const char *CounterModeEnumStr[] =
   {
      "UNDEFINED", 
      "INCREMENT",    /* @TEMPLATE@_CounterMode_Increment */
      "DECREMENT"     /* @TEMPLATE@_CounterMode_Deccrement */
   };
        
   return CounterModeEnumStr[CounterMode];

} /* End CounterModeStr() */


/******************************************************************************
** Function: ManageCounter
**
** Notes:
**   1. Information events are sent because this is instructional code and the
**      events provide feedback. The events are filtered so they won't flood
**      the ground. A reset app command resets the event filter.
**
*/
static void ManageCounter(void)
{

   char TimeStr[EXOBJ_TIME_STR_LEN];

   OS_TaskDelay(ExObj->ChildTaskDelay);
   
   CFE_TIME_Print(TimeStr, CFE_TIME_GetTime());
   

   if (ExObj->CounterMode == @TEMPLATE@_CounterMode_Increment)
   {
      if (ExObj->CounterValue < ExObj->CounterHiLim)
      {
         ExObj->CounterValue++;
      }
      else
      {
         ExObj->CounterValue = ExObj->CounterLoLim;
      }
   } /* End if increment */
   else
   {
      if (ExObj->CounterValue >  ExObj->CounterLoLim)
      {
         ExObj->CounterValue--;
      }
      else
      {
         ExObj->CounterValue = ExObj->CounterHiLim;
      }
   } /* End if decrement */

   StackPush(ExObj->CounterValue, TimeStr);
   
   // Only send TimeStr seconds and subseconds
   CFE_EVS_SendEvent (EXOBJ_EXECUTE_EID, CFE_EVS_EventType_INFORMATION,
                      "EXOBJ Manage Counter[%s]: %s counter mode: Value %d. Limits: Low=%d, High=%d",
                      &TimeStr[15],
                      CounterModeStr(ExObj->CounterMode), ExObj->CounterValue,
                      ExObj->CounterLoLim, ExObj->CounterHiLim);


} /* End ManageCounter() */


/******************************************************************************
** Function: StackPush
**
*/
static bool StackPush(uint16 CounterValue, const char *TimeStr)
{
   
   bool RetStatus = false;
   
   OS_MutSemTake(ExObj->ChildDataSemaphore);
   if (ExObj->CounterStackIndex < EXOBJ_STACK_ENTRIES)
   {
      ExObj->CounterStack[ExObj->CounterStackIndex].CounterValue = ExObj->CounterValue;
      strncpy(ExObj->CounterStack[ExObj->CounterStackIndex].TimeStr, TimeStr, EXOBJ_TIME_STR_LEN);
      ExObj->CounterStackIndex++;
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent (EXOBJ_STACK_PUSH_EID, CFE_EVS_EventType_INFORMATION,
                   "EXOBJ Stack Push [%s]: %d rejected, reached stack limit %d",
                   TimeStr, CounterValue, EXOBJ_STACK_ENTRIES);
   }
   OS_MutSemGive(ExObj->ChildDataSemaphore);
   
   return RetStatus;
   
} /* End StackPush() */
