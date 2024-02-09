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
**    Manage tables for an application
**
**  Notes:
**    1. This utility does not dictate a specific table format. It 
**       only specifies an API for managing an application's table.
**    2. This code must be reentrant and no global data can be used. 
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "cfe.h"
#include "fileutil.h"
#include "tblmgr.h"
#include "cmdmgr.h"


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static bool LoadTblStub(TBLMGR_Tbl_t* Tbl, uint8 LoadType, const char* Filename);
static bool DumpTblStub(TBLMGR_Tbl_t* Tbl, uint8 DumpType, const char* Filename);


/******************************************************************************
** Function: TBLMGR_Constructor
**
** Notes:
**    1. This function must be called prior to any other functions being
**       called using the same tblmgr instance.
**
*/
void TBLMGR_Constructor(TBLMGR_Class_t *TblMgr)
{

   int i;

   CFE_PSP_MemSet(TblMgr, 0, sizeof(TBLMGR_Class_t));
   for (i=0; i < TBLMGR_MAX_TBL_PER_APP; i++)
   {
      TblMgr->Tbl[i].LoadFuncPtr = LoadTblStub;
      TblMgr->Tbl[i].DumpFuncPtr = DumpTblStub;
   }

} /* End TBLMGR_Constructor() */


/******************************************************************************
** Function: TBLMGR_DumpTblCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
**  2. It calls the TBLMGR_DumpTblFuncPtr function that the user provided
**     during registration 
** 
*/
bool TBLMGR_DumpTblCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = false;
   TBLMGR_Tbl_t *Tbl;
   TBLMGR_Class_t *TblMgr = (TBLMGR_Class_t *) ObjDataPtr;
   const  TBLMGR_TblCmdMsg_Payload_t *DumpTblCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, TBLMGR_DumpTblCmdMsg_t);
      
   if (DumpTblCmd->Id < TblMgr->NextAvailableId)
   {
      TblMgr->Tbl[DumpTblCmd->Id].LastAction       = TBLMGR_ACTION_DUMP;
      TblMgr->Tbl[DumpTblCmd->Id].LastActionStatus = TBLMGR_STATUS_INVALID;
      TblMgr->LastActionTblId = DumpTblCmd->Id;
      if (FileUtil_VerifyDirForWrite(DumpTblCmd->Filename))
      {
         Tbl = &TblMgr->Tbl[DumpTblCmd->Id];
         RetStatus = (Tbl->DumpFuncPtr) (Tbl, DumpTblCmd->Type, DumpTblCmd->Filename);
         if (RetStatus)
         {
            TblMgr->Tbl[DumpTblCmd->Id].LastActionStatus = TBLMGR_STATUS_VALID;
            CFE_EVS_SendEvent(TBLMGR_DUMP_SUCCESS_EID, CFE_EVS_EventType_INFORMATION, 
                              "Successfully dumped table %d to file %s",
                              DumpTblCmd->Id, DumpTblCmd->Filename);
         }
      }
   }
   else
   {
      
      CFE_EVS_SendEvent(TBLMGR_DUMP_ID_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid table dump ID %d. Greater than last registered ID %d.",
                        DumpTblCmd->Id, (TblMgr->NextAvailableId-1));     
   }

   return RetStatus;
  
} /* End TBLMGR_DumpTbl() */


/******************************************************************************
** Function: TBLMGR_GetLastTblStatus
**
** Returns a pointer to the table status structure for the table that the
** last action was performed upon.
*/
const TBLMGR_Tbl_t *TBLMGR_GetLastTblStatus(TBLMGR_Class_t *TblMgr)
{

   if (TblMgr->LastActionTblId < TBLMGR_MAX_TBL_PER_APP)
   {
      return &(TblMgr->Tbl[TblMgr->LastActionTblId]);
   }
   else
   {
      return NULL;
   }
   
} /* End TBLMGR_GetLastTblStatus() */


/******************************************************************************
** Function: TBLMGR_GetTblStatus
**
** Returns a pointer to the table status for TblId.
*/
const TBLMGR_Tbl_t *TBLMGR_GetTblStatus(TBLMGR_Class_t *TblMgr, uint8 TblId)
{

   if (TblId < TBLMGR_MAX_TBL_PER_APP)
   {
      return &(TblMgr->Tbl[TblId]);
   }
   else
   {
      return NULL;
   }
   
} /* End TBLMGR_GetTblStatus() */


/******************************************************************************
** Function: TBLMGR_LoadTblCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
**  2. It calls the TBLMGR_LoadTblFuncPtr function that the user provided
**     during registration
** 
*/
bool TBLMGR_LoadTblCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = false;
   TBLMGR_Tbl_t *Tbl;
   TBLMGR_Class_t *TblMgr = (TBLMGR_Class_t *) ObjDataPtr;
   const  TBLMGR_TblCmdMsg_Payload_t* LoadTblCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, TBLMGR_LoadTblCmdMsg_t);

   if (DBG_TBLMGR) OS_printf("TBLMGR_LoadTblCmd() Entry\n");

   if (LoadTblCmd->Id < TblMgr->NextAvailableId)
   {

      TblMgr->Tbl[LoadTblCmd->Id].LastAction       = TBLMGR_ACTION_LOAD;
      TblMgr->Tbl[LoadTblCmd->Id].LastActionStatus = TBLMGR_STATUS_INVALID;
      TblMgr->LastActionTblId = LoadTblCmd->Id;
      /* Errors reported by utility so no need for else clause */
      if (FileUtil_VerifyFileForRead(LoadTblCmd->Filename))
      {

         if (DBG_TBLMGR) OS_printf("TBLMGR_LoadTblCmd() Before Tbl->LoadFuncPtr call\n");
         Tbl = &(TblMgr->Tbl[LoadTblCmd->Id]);
         RetStatus = (Tbl->LoadFuncPtr) (Tbl, LoadTblCmd->Type, LoadTblCmd->Filename);
         if (RetStatus)
         {
            TblMgr->Tbl[LoadTblCmd->Id].LastActionStatus = TBLMGR_STATUS_VALID;
            CFE_EVS_SendEvent(TBLMGR_LOAD_SUCCESS_EID, CFE_EVS_EventType_INFORMATION, 
                              "Successfully %sd table %d using file %s",
                              TBLMGR_LoadTypeStr(LoadTblCmd->Type),
                              LoadTblCmd->Id, LoadTblCmd->Filename);
         }
      }
   }
   else {
      
      CFE_EVS_SendEvent(TBLMGR_LOAD_ID_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Invalid table load ID %d. Greater than last registered ID %d.",
                        LoadTblCmd->Id, (TblMgr->NextAvailableId-1));     
   }

   return RetStatus;
  
} /* End TBLMGR_LoadTblCmd() */


/******************************************************************************
** Function: TBLMGR_LoadTypeStr
**
*/
const char *TBLMGR_LoadTypeStr(int8 LoadType)
{

   static const char *LoadTypeStr[] =
   {
      "replace",
      "update",
      "undefined" 
   };

   uint8 i = 2;
   
   if ( LoadType == TBLMGR_LOAD_TBL_REPLACE ||
        LoadType == TBLMGR_LOAD_TBL_UPDATE)
   {
   
      i = LoadType;
   
   }
        
   return LoadTypeStr[i];

} /* End TBLMGR_LoadTypeStr() */


/******************************************************************************
** Function: TBLMGR_RegisterTbl
**
** Register a table without loading a default table.
** Returns table ID.
*/
uint8 TBLMGR_RegisterTbl(TBLMGR_Class_t *TblMgr, TBLMGR_LoadTblFuncPtr_t LoadFuncPtr, 
                         TBLMGR_DumpTblFuncPtr_t DumpFuncPtr)
{
  
   TBLMGR_Tbl_t*  NewTbl;
   TblMgr->LastActionTblId = TBLMGR_MAX_TBL_PER_APP;
   
   if (DBG_TBLMGR) OS_printf("TBLMGR_RegisterTbl() Entry\n");
   if (TblMgr->NextAvailableId < TBLMGR_MAX_TBL_PER_APP)
   {

      NewTbl = &(TblMgr->Tbl[TblMgr->NextAvailableId]);
      NewTbl->Id = TblMgr->NextAvailableId;
      NewTbl->LastAction = TBLMGR_ACTION_REGISTER;
      NewTbl->LastActionStatus = TBLMGR_STATUS_VALID;
      NewTbl->Loaded = false;
      strcpy(NewTbl->Filename,TBLMGR_UNDEF_STR);
       
      /* Should never have null ptr but just in case leave stub function in place */
      if (LoadFuncPtr != NULL) 
         NewTbl->LoadFuncPtr = LoadFuncPtr;
      if (DumpFuncPtr != NULL) 
         NewTbl->DumpFuncPtr = DumpFuncPtr;
       
      TblMgr->NextAvailableId++;
      TblMgr->LastActionTblId = NewTbl->Id;

   }
   else
   {
      CFE_EVS_SendEvent (TBLMGR_REG_EXCEEDED_MAX_EID, CFE_EVS_EventType_ERROR,
      "Attempt to register a table that would have exceeded the max %d tables defined for the app",
      TBLMGR_MAX_TBL_PER_APP);
   }
  
   return TblMgr->LastActionTblId;
   
} /* End TBLMGR_RegisterTbl() */


/******************************************************************************
** Function: TBLMGR_RegisterTblWithDef
**
** Register a table and load a default table
** Returns table ID.
*/
uint8 TBLMGR_RegisterTblWithDef(TBLMGR_Class_t *TblMgr, TBLMGR_LoadTblFuncPtr_t LoadFuncPtr, 
                                TBLMGR_DumpTblFuncPtr_t DumpFuncPtr, const char *TblFilename)
{

   uint8 TblId = TBLMGR_RegisterTbl(TblMgr, LoadFuncPtr, DumpFuncPtr);
   TBLMGR_LoadTblCmdMsg_t LoadTblCmd;

   if (DBG_TBLMGR) OS_printf("TBLMGR_RegisterTblWithDef() Entry\n");

   if (TblId < TBLMGR_MAX_TBL_PER_APP)
   {
      strncpy (TblMgr->Tbl[TblId].Filename,TblFilename,OS_MAX_PATH_LEN);
      TblMgr->Tbl[TblId].Filename[OS_MAX_PATH_LEN-1] = '\0';
      
      /* Use load table command function */
      LoadTblCmd.Payload.Id = TblId;
      LoadTblCmd.Payload.Type = TBLMGR_LOAD_TBL_REPLACE;
      strncpy (LoadTblCmd.Payload.Filename,TblFilename,OS_MAX_PATH_LEN);
      TBLMGR_LoadTblCmd(TblMgr, (CFE_MSG_Message_t *)&LoadTblCmd);
      
   } /* End if TblId valid */
   
   return TblId;
  
} /* End TBLMGR_RegisterTblWithDef() */  


/******************************************************************************
** Function: TBLMGR_ResetStatus
**
*/
void TBLMGR_ResetStatus(TBLMGR_Class_t *TblMgr)
{

   /* Nothing to do - Preserve status of most recent action */ 

} /* End TBLMGR_ResetStatus() */


/******************************************************************************
** Function: DumpTblStub 
**
** Notes:
**  1. Must used the TBLMGR_TblDumpFuncPtr function definition
*/
static bool DumpTblStub(TBLMGR_Tbl_t *Tbl, uint8 DumpType, const char *Filename)
{

   CFE_EVS_SendEvent (TBLMGR_DUMP_STUB_ERR_EID, CFE_EVS_EventType_ERROR,
                      "Application did not define a dump table function for table %d",
                      Tbl->Id);

   return false;

} /* End DumpTblStub() */


/******************************************************************************
** Function: LoadTblStub 
**
** Notes:
**  1. Must used the TBLMGR_TblLoadFuncPtr function definition
*/
static bool LoadTblStub(TBLMGR_Tbl_t *Tbl, uint8 LoadType, const char *Filename)
{

   CFE_EVS_SendEvent (TBLMGR_LOAD_STUB_ERR_EID, CFE_EVS_EventType_ERROR,
                      "Application did not define a load table function for table %d",
                      Tbl->Id);

   return false;

} /* End LoadTblStub() */

