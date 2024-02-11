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
**
*/

#ifndef _tblmgr_
#define _tblmgr_

/*
** Includes
*/

#include "app_c_fw_cfg.h"
#include "cmdmgr.h"

/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define TBLMGR_REG_EXCEEDED_MAX_EID  (TBLMGR_BASE_EID + 0)
#define TBLMGR_LOAD_ID_ERR_EID       (TBLMGR_BASE_EID + 1)
#define TBLMGR_DUMP_ID_ERR_EID       (TBLMGR_BASE_EID + 2)
#define TBLMGR_LOAD_STUB_ERR_EID     (TBLMGR_BASE_EID + 3)
#define TBLMGR_DUMP_STUB_ERR_EID     (TBLMGR_BASE_EID + 4)
#define TBLMGR_LOAD_SUCCESS_EID      (TBLMGR_BASE_EID + 5)
#define TBLMGR_DUMP_SUCCESS_EID      (TBLMGR_BASE_EID + 6)


#define TBLMGR_UNDEF_STR  "Undefined"
 
/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Command Packets
** - See EDS command definitions in app_c_fw.xml
*/


/* 
** Table Class
** - ID is assigned by TBLMGR
** - Application's table processing object supply the load/dump functions
*/

typedef struct TBLMGR_Tbl TBLMGR_Tbl_t;

typedef bool (*TBLMGR_LoadTblFuncPtr_t) (APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);
typedef bool (*TBLMGR_DumpTblFuncPtr_t) (osal_id_t FileHandle);

struct TBLMGR_Tbl
{
  
   uint8   Id; 
   char    Name[OS_MAX_API_NAME];
   bool    Loaded;
   uint8   LastAction;
   APP_C_FW_TblActionStatus_Enum_t   LastActionStatus;
   char    Filename[OS_MAX_PATH_LEN];
      
   TBLMGR_LoadTblFuncPtr_t  LoadFuncPtr;
   TBLMGR_DumpTblFuncPtr_t  DumpFuncPtr;

};

/* 
** Table Manager Class
*/

typedef struct
{
   const char    *AppName;
   uint8         NextAvailableId;
   uint8         LastActionTblId;
   TBLMGR_Tbl_t  Tbl[TBLMGR_MAX_TBL_PER_APP];

} TBLMGR_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: TBLMGR_Constructor
**
** Notes:
**    1. This function must be called prior to any other functions being
**       called using the same tblmgr instance.
*/
void TBLMGR_Constructor(TBLMGR_Class_t *TblMgr, const char *AppName);


/******************************************************************************
** Function: TBLMGR_DumpTblCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
**  2. Creates a new dump file, overwriting anything that may have existed
**     previously
**  3. It calls the TBLMGR_DumpTblFuncPtr function that the user provided
**     during registration to load table-specific JSON objects.
** 
*/
bool TBLMGR_DumpTblCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: TBLMGR_GetLastTblStatus
**
** Returns a pointer to the table status structure for the table that the
** last action was performed upon. Returns NULL if table manager's last action
** is in an invalid state.
*/
const TBLMGR_Tbl_t *TBLMGR_GetLastTblStatus(TBLMGR_Class_t *TblMgr);


/******************************************************************************
** Function: TBLMGR_GetTblStatus
**
** Returns a pointer to the table status for TblId. Returns NULL if TblId 
** is invalid.
*/
const TBLMGR_Tbl_t *TBLMGR_GetTblStatus(TBLMGR_Class_t *TblMgr, uint8 TblId);


/******************************************************************************
** Function: TBLMGR_LoadTblCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
**  2. It calls the TBLMGR_LoadTblFuncPtr function that the user provided
**     during registration 
** 
*/
bool TBLMGR_LoadTblCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: TBLMGR_LoadTypeStr
**
** Note:
**  1. Returns a pointer to a string that describes the load type 
** 
*/
const char *TBLMGR_LoadTypeStr(int8 LoadType);


/******************************************************************************
** Function: TBLMGR_RegisterTbl
**
** Register a table without loading a default table.
** Returns table ID assigned to new table or TBLMGR_MAX_TBL_PER_APP if no IDs left.
*/
uint8 TBLMGR_RegisterTbl(TBLMGR_Class_t *TblMgr, const char *TblName,
                         TBLMGR_LoadTblFuncPtr_t LoadFuncPtr, 
                         TBLMGR_DumpTblFuncPtr_t DumpFuncPtr); 


/******************************************************************************
** Function: TBLMGR_RegisterTblWithDef
**
** Register a table and load a default table
** Returns table ID assigned to new table or TBLMGR_MAX_TBL_PER_APP if no IDs left.
*/
uint8 TBLMGR_RegisterTblWithDef(TBLMGR_Class_t *TblMgr, const char *TblName,
                                TBLMGR_LoadTblFuncPtr_t LoadFuncPtr, 
                                TBLMGR_DumpTblFuncPtr_t DumpFuncPtr,
                                const char *TblFilename); 


/******************************************************************************
** Function: TBLMGR_ResetStatus
**
*/
void TBLMGR_ResetStatus(TBLMGR_Class_t *TblMgr);


#endif /* _tblmgr_ */
