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
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
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

const char* CounterModeStr(@TEMPLATE@_CounterMode_Enum_t  CounterMode);

/**********************/
/** Global File Data **/
/**********************/

static EXOBJ_Class_t*  ExObj = NULL;



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
   ExObj->CounterValue = ExObj->Tbl.Data.LowLimit;

   EXOBJTBL_Constructor(&ExObj->Tbl, IniTbl);

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
   
   bool RetStatus = false;
   const @TEMPLATE@_CounterMode_Payload_t *Cmd = CMDMGR_PAYLOAD_PTR(MsgPtr, @TEMPLATE@_SetCounterMode_t);
   @TEMPLATE@_CounterMode_Enum_t PrevMode = ExObj->CounterMode;
   
   if ((Cmd->Mode == @TEMPLATE@_CounterMode_Increment) ||
       (Cmd->Mode == @TEMPLATE@_CounterMode_Decrement))
   {
   
      RetStatus = true;
      
      ExObj->CounterMode = Cmd->Mode;
      
      CFE_EVS_SendEvent (EXOBJ_SET_MODE_CMD_EID, CFE_EVS_EventType_INFORMATION,
                         "Counter mode changed from %s to %s",
                         CounterModeStr(PrevMode), CounterModeStr(ExObj->CounterMode));

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
   
   if (ExObj->CounterMode == @TEMPLATE@_CounterMode_Increment)
   {
      if (ExObj->CounterValue < ExObj->Tbl.Data.HighLimit)
      {
         ExObj->CounterValue++;
      }
      else
      {
         ExObj->CounterValue = ExObj->Tbl.Data.LowLimit;
      }
   } /* End if increment */
   else
   {
      if (ExObj->CounterValue > ExObj->Tbl.Data.LowLimit)
      {
         ExObj->CounterValue--;
      }
      else
      {
         ExObj->CounterValue = ExObj->Tbl.Data.HighLimit;
      }
   } /* End if decrement */
   
   
   CFE_EVS_SendEvent (EXOBJ_EXECUTE_EID, CFE_EVS_EventType_DEBUG,
                      "%s counter mode: Value %d", 
                      CounterModeStr(ExObj->CounterMode), ExObj->CounterValue);

   
} /* End EXOBJ_Execute() */


/******************************************************************************
** Function: CounterModeStr
**
** Type checking should enforce valid parameter
*/
const char* CounterModeStr(@TEMPLATE@_CounterMode_Enum_t  CounterMode)
{

   static const char *CounterModeEnumStr[] =
   {
      "UNDEFINED", 
      "INCREMENT",    /* @TEMPLATE@_CounterMode_Increment */
      "DECREMENT"     /* @TEMPLATE@_CounterMode_Deccrement */
   };
        
   return CounterModeEnumStr[CounterMode];

} /* End CounterModeStr() */


