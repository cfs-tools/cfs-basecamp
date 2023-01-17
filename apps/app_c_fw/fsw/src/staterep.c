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
**    Define a class that provides a mechanism for objects to report boolean
**    states represented by a single bit that are aggregated into an app state
**    report packet.
**
**  Notes:
**    1. This code was originally acquired from the NASA cFS External Code
**       Interface (ECI) NOSA release. Several cosmetic changes have been made
**       but it remains functionally very similar.
**    2. This code must be reentrant and no global data can be used. 
**    3. There are several (uint16) casts that are required to prevent compiler
**       warnings most are due to the compiler assuming a signed integer result
**       for integer-based math operations.
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Include Files:
*/

#include "staterep.h"


/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/

typedef struct
{

   uint16  WordIndex;
   uint16  Mask;

} StateRepBitStruct_t;


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static bool GetIdBit(STATEREP_Class_t*    StateRep,
                     const char*          CallerStr,
                     uint16               Id,
                     StateRepBitStruct_t* StateRepBit);
                         

/******************************************************************************
** Function: STATEREP_Constructor
**
** Notes:
**    None
*/
void STATEREP_Constructor(STATEREP_Class_t*  StateRep, 
                          uint16             IdCnt)
{

   uint16 RemBitCnt, i;

   /*
   ** Clear entire check structure which disables all detectors and
   ** clears their latch flags.
   */
   
   CFE_PSP_MemSet(StateRep,0,sizeof(STATEREP_Class_t));

   StateRep->BitConfig.IdLimit = IdCnt;
   StateRep->BitConfig.BitfieldWords = (uint16)(IdCnt / STATEREP_BITS_PER_WORD);
   
   RemBitCnt = (uint16)(IdCnt % STATEREP_BITS_PER_WORD);
   for (i=0; i < RemBitCnt; i++)
   {
      
      StateRep->BitConfig.BitfieldRemMask |= 1 << i;
   }

   StateRep->TlmMode = STATEREP_NEW_REPORT;
      
} /* End STATEREP_Constructor() */



/******************************************************************************
** Function: STATEREP_ClearBitCmd
**
** Notes:
**    1. Must clear both the Software Bus report packet and NewReport.
*/
bool STATEREP_ClearBitCmd(                   void *ObjDataPtr,  /* Pointer to an instance of a STATEREP_Class  */
                          const CFE_MSG_Message_t *MsgPtr)      /* Pointer to STATEREP_ClearBitCmd struct      */
{

   STATEREP_Class_t*          StateRep    = (STATEREP_Class_t*)ObjDataPtr;
   STATEREP_ClearBitCmdMsg_t* ClearBitCmd = (STATEREP_ClearBitCmdMsg_t*)MsgPtr;


   bool                 RetStatus = true;
   StateRepBitStruct_t  StateRepBit;

   if (ClearBitCmd->Id == STATEREP_SELECT_ALL)
   {

      CFE_PSP_MemSet(&(StateRep->BitConfig.Latched),0,sizeof(StateRep->BitConfig.Latched));

      for (StateRepBit.WordIndex=0; StateRepBit.WordIndex < STATEREP_BITFIELD_WORDS; StateRepBit.WordIndex++)
      {

         StateRep->CurrBits.Word[StateRepBit.WordIndex]    = 0;
         StateRep->TlmMsg.Bits.Word[StateRepBit.WordIndex] = 0;

      } /* End LatchIndex loop */

   } /* End if select all */

   else
   {

      RetStatus = GetIdBit(StateRep, "State Reporter Rejected Clear Bit Cmd: ",
                           ClearBitCmd->Id, &StateRepBit);
      
      if (RetStatus == true)
      {

         StateRepBit.Mask = (uint16)~StateRepBit.Mask;

         StateRep->BitConfig.Latched[StateRepBit.WordIndex] &= StateRepBit.Mask;

         StateRep->CurrBits.Word[StateRepBit.WordIndex]    &= StateRepBit.Mask;
         StateRep->TlmMsg.Bits.Word[StateRepBit.WordIndex] &= StateRepBit.Mask;
      
      } /* End if valid ID */

   } /* End if individual ID */


   return RetStatus;


} /* End STATEREP_ClearBitCmd() */


/******************************************************************************
** Function: STATEREP_ConfigBitCmd
**
** Notes:
**    None
*/
bool STATEREP_ConfigBitCmd(                void*  ObjDataPtr,  /* Pointer to an instance of a STATEREP_Class   */
                           const CFE_MSG_Message_t *MsgPtr)    /* Pointer to STATEREP_ConfigBitCmd struct */

{

   STATEREP_Class_t*            StateRep     = (STATEREP_Class_t*)ObjDataPtr;
   STATEREP_ConfigBitCmdMsg_t*  ConfigBitCmd = (STATEREP_ConfigBitCmdMsg_t*)MsgPtr;

   bool RetStatus = true;

   StateRepBitStruct_t  StateRepBit;

   if (ConfigBitCmd->Enable == true || ConfigBitCmd->Enable == false)
   {

      if (ConfigBitCmd->Id == STATEREP_SELECT_ALL)
      {
         
         if (ConfigBitCmd->Enable)
         {
            
            for (StateRepBit.WordIndex=0; StateRepBit.WordIndex < StateRep->BitConfig.BitfieldWords; StateRepBit.WordIndex++)
               StateRep->BitConfig.Enabled[StateRepBit.WordIndex] = 0xFFFF;

            if (StateRep->BitConfig.BitfieldWords < STATEREP_BITFIELD_WORDS)
               StateRep->BitConfig.Enabled[StateRep->BitConfig.BitfieldWords] = StateRep->BitConfig.BitfieldRemMask;

         }
         else
         {
            
            CFE_PSP_MemSet(&(StateRep->BitConfig.Enabled),0,sizeof(StateRep->BitConfig.Enabled));
         
         }         
      } /* End if select all */
      
      else
      {
         
         RetStatus = GetIdBit(StateRep, "State Reporter Reject Config Bit Cmd:",
                              ConfigBitCmd->Id, &StateRepBit);
         
         if (RetStatus == true)
         {
            
            if (ConfigBitCmd->Enable)
               StateRep->BitConfig.Enabled[StateRepBit.WordIndex] |= StateRepBit.Mask;
            
            else
               StateRep->BitConfig.Enabled[StateRepBit.WordIndex] &= ~StateRepBit.Mask;
            
         } /* End if valid ID */
         
      } /* End if individual ID */
   } /* End if valid boolean range */
   else
   {

      CFE_EVS_SendEvent (STATEREP_CONFIG_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                         "State Reporter Reject Config Bit Cmd: Invalid enable value %d",
                         ConfigBitCmd->Enable);

      RetStatus = false;

   } /* End if invalid boolean range */
  

   return RetStatus;


} /* End STATEREP_ConfigBitCmd() */


/******************************************************************************
** Function: STATEREP_GenTlmMsg
**
** Notes:
**    1. Logic assumes STATEREP_REPORT_MODE has two states.
**
*/
void STATEREP_GenTlmMsg(STATEREP_Class_t*  StateRep,
                        STATEREP_TlmMsg_t* StateRepMsg)
{

   uint16  i;

   /*
   ** Generate the state report packet
   ** - Merge or copy CurrBits into the telemetry packet
   ** - Clear CurrBits for the next execution cycle
   */

   if (StateRep->TlmMode == STATEREP_MERGE_REPORT)
   {

      for (i=0; i < STATEREP_BITFIELD_WORDS; i++)
         StateRepMsg->Bits.Word[i] |= StateRep->CurrBits.Word[i];

   } /* End if STATEREP_MERGE_REPORT */
   else
   {

      CFE_PSP_MemCpy(&(StateRepMsg->Bits),&(StateRep->CurrBits),sizeof(STATEREP_Bits_t));

   } /* End if STATEREP_NEW_REPORT */

   CFE_PSP_MemSet(&(StateRep->CurrBits),0,sizeof(STATEREP_Bits_t));


} /* End STATEREP_GenTlmMsg() */


/******************************************************************************
** Function: STATEREP_SetBit
**
** Notes:
**    1. Errors in this functions should only occur during integration. No
**       return status is provided because the caller always expects the call
**       to be successful.
*/
void STATEREP_SetBit(STATEREP_Class_t*  StateRep,
                             uint16     Id)
{

   bool                 ValidId;
   StateRepBitStruct_t  StateRepBit;

      
   ValidId = GetIdBit(StateRep, "State Reporter Rejected Set Bit Call:",
                      Id, &StateRepBit);
      
   if (ValidId == true)
   {

      if (StateRep->BitConfig.Enabled[StateRepBit.WordIndex] & StateRepBit.Mask)
      {
         
         StateRep->BitConfig.Latched[StateRepBit.WordIndex] |= StateRepBit.Mask;
            
         StateRep->CurrBits.Word[StateRepBit.WordIndex] |= StateRepBit.Mask;
            
      } /* End if enabled */
         
   } /* End if valid ID */


} /* End STATEREP_SetBit() */


/******************************************************************************
** Function: STATEREP_SetTlmMode
**
** Notes:
**    None
**
*/
void STATEREP_SetTlmMode(STATEREP_Class_t*   StateRep,
                         STATEREP_TlmMode_t  TlmMode)
{

   StateRep->TlmMode = TlmMode;

} /* End STATEREP_SetTlmMode() */


/******************************************************************************
** Function: STATEREP_TlmModeStr
**
*/
const char* STATEREP_TlmModeStr(STATEREP_TlmMode_t  TlmMode)
{

   static const char* TlmModeStr[] =
   {
      "Undefined", 
      "New Report",     /* STATEREP_NEW_REPORT   */
      "Merge Report"    /* STATEREP_MERGE_REPORT */
   };

   uint8 i = 0;
   
   if ( TlmMode == STATEREP_NEW_REPORT ||
        TlmMode == STATEREP_MERGE_REPORT)
   {
      i = TlmMode;
   }
        
   return TlmModeStr[i];

} /* End STATEREP_TlmModeStr() */


/******************************************************************************
** Function: GetIdBit
**
** Notes:
**    1. If the ID is invalid (too big) then an event message is sent.
**
*/
static bool GetIdBit(STATEREP_Class_t*    StateRep,
                     const char*          CallerStr,
                     uint16               Id,
                     StateRepBitStruct_t* StateRepBit)                        
{

   bool  RetStatus = true;


   if (Id < StateRep->BitConfig.IdLimit)
   {
   
      StateRepBit->WordIndex = (uint16)(Id/STATEREP_BITS_PER_WORD);
      StateRepBit->Mask = (uint16)(1 << (uint16)(Id % STATEREP_BITS_PER_WORD));
   
   }
   else
   {

      RetStatus = false;
      CFE_EVS_SendEvent (STATEREP_INVALID_ID_EID, CFE_EVS_EventType_ERROR,
                         "%s Invalid identifier %d (Max ID = %d)",
                         CallerStr, Id, StateRep->BitConfig.IdLimit-1);
   }

   return RetStatus;

} /* End GetIdBit() */

