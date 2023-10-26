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


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static bool AcceptNewTable(const EXOBJTBL_Data_t *TblData);
static const char *CounterModeStr(@TEMPLATE@_CounterMode_Enum_t CounterMode);
static EXOBJTBL_Limit_t GetCounterModeLimits(void);

/**********************/
/** Global File Data **/
/**********************/

static EXOBJ_Class_t *ExObj = NULL;


/******************************************************************************
** Function: EXOBJ_Constructor
**
*/
void EXOBJ_Constructor(EXOBJ_Class_t *ExObjPtr,
                       const INITBL_Class_t *IniTbl,
                       TBLMGR_Class_t *TblMgr)
{
 
   ExObj = ExObjPtr;

   CFE_PSP_MemSet((void*)ExObj, 0, sizeof(EXOBJ_Class_t));
    
   ExObj->CounterMode  = @TEMPLATE@_CounterMode_Increment;
   ExObj->CounterValue = ExObj->Tbl.Data.IncrLimit.Low;

   EXOBJTBL_Constructor(&ExObj->Tbl, IniTbl, AcceptNewTable);

   TBLMGR_RegisterTblWithDef(TblMgr, EXOBJTBL_LoadCmd, EXOBJTBL_DumpCmd, 
                             INITBL_GetStrConfig(IniTbl, CFG_TBL_LOAD_FILE));
   
} /* End EXOBJ_Constructor */


/******************************************************************************
** Function:  EXOBJ_ResetStatus
**
*/
void EXOBJ_ResetStatus()
{
 
   EXOBJTBL_ResetStatus();
   
} /* End EXOBJ_ResetStatus() */


/******************************************************************************
** Function: EXOBJ_SetModeCmd
**
** Notes:
**   1. See file prologue for logging/playback logic.
*/
bool EXOBJ_SetModeCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool  RetStatus = false;
   EXOBJTBL_Limit_t CounterLimit;

   const @TEMPLATE@_CounterMode_CmdPayload_t *Cmd = CMDMGR_PAYLOAD_PTR(MsgPtr, @TEMPLATE@_SetCounterMode_t);
   @TEMPLATE@_CounterMode_Enum_t PrevMode = ExObj->CounterMode;
   
   if ((Cmd->Mode == @TEMPLATE@_CounterMode_Increment) ||
       (Cmd->Mode == @TEMPLATE@_CounterMode_Decrement))
   {
   
      RetStatus = true; 
      ExObj->CounterMode = Cmd->Mode;
      CounterLimit = GetCounterModeLimits();
      
      CFE_EVS_SendEvent (EXOBJ_SET_MODE_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Counter mode changed from %s to %s. Limits: Low=%d, High=%d",
                         CounterModeStr(PrevMode), CounterModeStr(ExObj->CounterMode),
                         CounterLimit.Low, CounterLimit.High);

   }
   else
   {
      CFE_EVS_SendEvent (EXOBJ_SET_MODE_CMD_EID, CFE_EVS_EventType_ERROR,
                         "Set counter mode rejected. Invalid mode %d",
                         Cmd->Mode);
   }
   
   return RetStatus;

} /* End EXOBJ_SetModeCmd() */


/******************************************************************************
** Function: EXOBJ_Execute
**
*/
void EXOBJ_Execute(void)
{
   
   EXOBJTBL_Limit_t CounterLimit;
      
   if (ExObj->CounterMode == @TEMPLATE@_CounterMode_Increment)
   {
      if (ExObj->CounterValue < ExObj->Tbl.Data.IncrLimit.High)
      {
         ExObj->CounterValue++;
      }
      else
      {
         ExObj->CounterValue = ExObj->Tbl.Data.IncrLimit.Low;
      }
   } /* End if increment */
   else
   {
      if (ExObj->CounterValue > ExObj->Tbl.Data.DecrLimit.Low)
      {
         ExObj->CounterValue--;
      }
      else
      {
         ExObj->CounterValue = ExObj->Tbl.Data.DecrLimit.High;
      }
   } /* End if decrement */
   
   CounterLimit = GetCounterModeLimits();
   CFE_EVS_SendEvent (EXOBJ_EXECUTE_EID, CFE_EVS_EventType_DEBUG,
                      "%s counter mode: Value %d. Limits: Low=%d, High=%d", 
                      CounterModeStr(ExObj->CounterMode), ExObj->CounterValue,
                      CounterLimit.Low, CounterLimit.High);
                         
} /* End EXOBJ_Execute() */


/******************************************************************************
** Function: AcceptNewTable
**
** Validate new table load
**
*/
static bool AcceptNewTable(const EXOBJTBL_Data_t *TblData)
{

   bool RetStatus = false;
   
   if (TblData->IncrLimit.Low >= TblData->IncrLimit.High)
   {
      CFE_EVS_SendEvent(EXOBJ_ACCEPT_TBL_EID, CFE_EVS_EventType_ERROR, 
                        "Table rejected. Increment low limit %d must be less than high limit %d",
                        TblData->IncrLimit.Low, TblData->IncrLimit.High);
   }
   else if (TblData->DecrLimit.Low >= TblData->DecrLimit.High)
   {
      CFE_EVS_SendEvent(EXOBJ_ACCEPT_TBL_EID, CFE_EVS_EventType_ERROR, 
                        "Table rejected. Decrement low limit %d must be less than high limit %d",
                        TblData->DecrLimit.Low, TblData->DecrLimit.High);
   }
   else
   {
      //EX1
      if ((TblData->IncrLimit.High - TblData->IncrLimit.Low) > TblData->LimitRangeMax ||
          (TblData->DecrLimit.High - TblData->DecrLimit.Low) > TblData->LimitRangeMax )
      {
         CFE_EVS_SendEvent(EXOBJ_ACCEPT_TBL_EID, CFE_EVS_EventType_ERROR, 
                           "Table rejected. Maximum range %d exceeded. Increment: Low %d, High %d, Decrement: Low %d, High %d",
                           TblData->LimitRangeMax,
                           TblData->IncrLimit.Low, TblData->IncrLimit.High, 
                           TblData->DecrLimit.Low, TblData->DecrLimit.High);
      }
      else
      {         
         RetStatus = true;
      }
      //EX1
      
   } /* End if valid limits */


   return RetStatus;
   
} /* End AcceptNewTable() */


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
** Function: GetCounterModeLimits
**
*/
static EXOBJTBL_Limit_t GetCounterModeLimits(void)
{
   
   EXOBJTBL_Limit_t CounterLimit;

   if (ExObj->CounterMode == @TEMPLATE@_CounterMode_Increment)
   {
      CounterLimit.Low  = ExObj->Tbl.Data.IncrLimit.Low;
      CounterLimit.High = ExObj->Tbl.Data.IncrLimit.High;

   } /* End if increment */
   else
   {
      CounterLimit.Low  = ExObj->Tbl.Data.DecrLimit.Low;
      CounterLimit.High = ExObj->Tbl.Data.DecrLimit.High;
      
   } /* End if decrement */
   
   return CounterLimit;
   
} /* End GetCounterModeLimits() */

