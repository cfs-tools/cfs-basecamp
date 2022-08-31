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
**    Manage command dispatching for an application
**
**  Notes:
**    1. 'Command' does not necessarily mean a ground command. 
**    2. This code must be reentrant so no global data is used.  
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Include Files:
*/

#include "cmdmgr.h"


/***********************/
/** Macro Definitions **/
/***********************/



/**********************/
/** Global File Data **/
/**********************/

static const char* BoolStr[] = {
   "FALSE",
   "TRUE",
   "UNDEF"
};


/******************************/
/** File Function Prototypes **/
/******************************/

static void LogMsgBytes(uint8* MsgPtr, size_t PayloadLen, CFE_MSG_FcnCode_t FuncCode);
static bool UnusedFuncCode(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: CMDMGR_Constructor
**
** Notes:
**    1. This function must be called prior to any other functions being
**       called using the same cmdmgr instance.
**
*/
void CMDMGR_Constructor(CMDMGR_Class_t* CmdMgr)
{

   int i;

   CFE_PSP_MemSet(CmdMgr, 0, sizeof(CMDMGR_Class_t));
   for (i=0; i < CMDMGR_CMD_FUNC_TOTAL; i++)
   {
      
      CmdMgr->Cmd[i].FuncPtr = UnusedFuncCode;
   
   }

} /* End CMDMGR_Constructor() */


/******************************************************************************
** Function: CMDMGR_BoolStr
**
** Purpose: Return a pointer to a string describing a boolean
**
** Notes:
**   Assumes false=0 and true=1
*/
const char* CMDMGR_BoolStr(bool BoolArg)
{
   
   uint8 i = 2;
   
   if ( BoolArg == true || BoolArg == false) {
   
      i = BoolArg;
   
   }
        
   return BoolStr[i];

} /* End CMDMGR_BoolStr() */


/******************************************************************************
** Function: CMDMGR_DispatchFunc
**
** Notes:
**   1. Considered sending an event message for alternate counter commands, but
**      decided this is the client's responsibility. CmdMgr is a dispatcher and
**      if an app wants a message response then it should publish the format. 
**
*/
bool CMDMGR_DispatchFunc(CMDMGR_Class_t* CmdMgr, const CFE_MSG_Message_t *MsgPtr)
{

   bool   ValidCmd = false;
   bool   ChecksumValid;
   size_t UserDataLen; 
   CFE_MSG_Size_t    MsgSize;
   CFE_MSG_FcnCode_t FuncCode;

   UserDataLen = CFE_SB_GetUserDataLength(MsgPtr);

   CFE_MSG_GetSize(MsgPtr, &MsgSize);
   CFE_MSG_GetFcnCode(MsgPtr, &FuncCode);
   CFE_MSG_ValidateChecksum(MsgPtr, &ChecksumValid);

   if (DBG_CMDMGR) LogMsgBytes((uint8*) MsgPtr, UserDataLen, FuncCode);

   if (FuncCode < CMDMGR_CMD_FUNC_TOTAL)
   {

      if (UserDataLen == CmdMgr->Cmd[FuncCode].UserDataLen)
      {

         if (ChecksumValid)
         {

            ValidCmd = (CmdMgr->Cmd[FuncCode].FuncPtr)(CmdMgr->Cmd[FuncCode].DataPtr, MsgPtr);

         } /* End if valid checksum */
         else
         {

            CFE_EVS_SendEvent (CMDMGR_DISPATCH_INVALID_CHECKSUM_ERR_EID, CFE_EVS_EventType_ERROR,
                               "Invalid command checksum");
         
         }
      } /* End if valid length */
      else
      {

         CFE_EVS_SendEvent (CMDMGR_DISPATCH_INVALID_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                            "Invalid command user data length %d, expected %d for function code %d",
                            (unsigned int)UserDataLen, CmdMgr->Cmd[FuncCode].UserDataLen, FuncCode);

      }

   } /* End if valid function code */
   else
   {
      
      CFE_EVS_SendEvent (CMDMGR_DISPATCH_INVALID_FUNC_CODE_ERR_EID, CFE_EVS_EventType_ERROR,
                         "Invalid command function code %d is greater than max %d",
                         FuncCode, (CMDMGR_CMD_FUNC_TOTAL-1));

   } /* End if invalid function code */

   if (CmdMgr->Cmd[FuncCode].AltCnt.Enabled)
   {
   
      ValidCmd ? CmdMgr->Cmd[FuncCode].AltCnt.Valid++ : CmdMgr->Cmd[FuncCode].AltCnt.Invalid++;
   
   } 
   else
   {
   
      ValidCmd ? CmdMgr->ValidCmdCnt++ : CmdMgr->InvalidCmdCnt++;
   
   }
   
   return ValidCmd;

} /* End CMDMGR_DispatchFunc() */


/******************************************************************************
** Function: CMDMGR_RegisterFunc
**
*/
bool CMDMGR_RegisterFunc(CMDMGR_Class_t* CmdMgr, uint16 FuncCode, void* ObjDataPtr, 
                         CMDMGR_CmdFuncPtr_t ObjFuncPtr, uint16 UserDataLen)
{

   bool    RetStatus = false;
   
   if (FuncCode < CMDMGR_CMD_FUNC_TOTAL)
   {

      if (DBG_CMDMGR) OS_printf("CMDMGR_RegisterFunc(): FuncCode %d, DataLen %d\n", FuncCode, UserDataLen);
      CmdMgr->Cmd[FuncCode].DataPtr = ObjDataPtr;
      CmdMgr->Cmd[FuncCode].FuncPtr = ObjFuncPtr;
      CmdMgr->Cmd[FuncCode].UserDataLen = UserDataLen;

      CmdMgr->Cmd[FuncCode].AltCnt.Enabled = false;
      CmdMgr->Cmd[FuncCode].AltCnt.Valid   = 0;
      CmdMgr->Cmd[FuncCode].AltCnt.Invalid = 0;
      
      RetStatus = true;

   }
   else
   {
      
      CFE_EVS_SendEvent (CMDMGR_REG_INVALID_FUNC_CODE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Attempt to register function code %d which is greater than max %d",
         FuncCode,(CMDMGR_CMD_FUNC_TOTAL-1));
   }

   return RetStatus;
   
} /* End CMDMGR_RegisterFunc() */


/******************************************************************************
** Function: CMDMGR_RegisterFuncAltCnt
**
*/
bool CMDMGR_RegisterFuncAltCnt(CMDMGR_Class_t* CmdMgr, uint16 FuncCode, void* ObjDataPtr, 
                               CMDMGR_CmdFuncPtr_t ObjFuncPtr, uint16 UserDataLen)
{

   bool    RetStatus = false;

   if (CMDMGR_RegisterFunc(CmdMgr, FuncCode, ObjDataPtr, ObjFuncPtr, UserDataLen))
   {
      
      CmdMgr->Cmd[FuncCode].AltCnt.Enabled = true;      

      RetStatus = true;

   }

   return RetStatus;
   
} /* End CMDMGR_RegisterFuncAltCnt() */


/******************************************************************************
** Function: CMDMGR_ResetStatus
**
** Keep logic simple and clear all alternate counters regardless of whether 
** they're enabled. 
**
*/
void CMDMGR_ResetStatus(CMDMGR_Class_t* CmdMgr)
{

   int i;

   CmdMgr->ValidCmdCnt   = 0;
   CmdMgr->InvalidCmdCnt = 0;

   for (i=0; i < CMDMGR_CMD_FUNC_TOTAL; i++)
   {
      
      CmdMgr->Cmd[i].AltCnt.Valid   = 0;
      CmdMgr->Cmd[i].AltCnt.Invalid = 0;
   
   }
   
} /* End CMDMGR_ResetStatus() */


/******************************************************************************
** Function: CMDMGR_ValidBoolArg
**
*/
bool CMDMGR_ValidBoolArg(uint16 BoolArg)
{
   
   return ((BoolArg == true) || (BoolArg == false));

} /* CMDMGR_ValidBoolArg() */


/******************************************************************************
** Function: LogMsgBytes
**
** Print message header on title line followed by payload data rows
*/
static void LogMsgBytes(uint8* MsgPtr, size_t PayloadLen, CFE_MSG_FcnCode_t FuncCode)
{

   int    i, row_byte;
   uint16 HeaderLen = sizeof(CFE_MSG_CommandHeader_t);
   
   
   OS_printf("Command function %d - %d byte header: ", FuncCode, (unsigned int)sizeof(CFE_MSG_CommandHeader_t));
   
   for (i=0; i < HeaderLen; i++)
   {

      if (i%2 == 0)
      {
         OS_printf("0x%02X", MsgPtr[i]);
      }
      else
      {
         OS_printf("%02X ", MsgPtr[i]);
      } 
   
   } /* End header loop */
   OS_printf("\n");
             
   for (i=HeaderLen, row_byte=0; i < (HeaderLen+PayloadLen); i++, row_byte++)
   {

      if (row_byte !=0 && row_byte%16 == 0)
      {
         OS_printf("\n");
      }

      if (i%2 == 0)
      {
         OS_printf("0x%02X", MsgPtr[i]);
      }
      else
      {
         OS_printf("%02X ", MsgPtr[i]);
      } 
   
   } /* End msg loop */
   if (i != HeaderLen)
   {
      OS_printf("\n");
   }
   
} /* End LogMsgBytes() */


/******************************************************************************
** Function: UnusedFuncCode
**
*/
static bool UnusedFuncCode(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_MSG_FcnCode_t FuncCode;
   
   CFE_MSG_GetFcnCode(MsgPtr, &FuncCode);
   CFE_EVS_SendEvent (CMDMGR_DISPATCH_UNUSED_FUNC_CODE_ERR_EID, CFE_EVS_EventType_ERROR,
                      "Unused command function code %d received",FuncCode);

   return false;

} /* End UnusedFuncCode() */



