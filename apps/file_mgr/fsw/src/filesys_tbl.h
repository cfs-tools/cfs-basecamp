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
**    Manage FILE_MGR's File System table
**
**  Notes:
**    1. Use the Singleton design pattern. A pointer to the table object
**       is passed to the constructor and saved for all other operations.
**       This is a table-specific file so it doesn't need to be re-entrant.
**    2. FILE_MGR's EDS defines the FileSys table structure and content
**       options. The EDS and JSON must manually be kept in synch.
*/

#ifndef _filesys_tbl_
#define _filesys_tbl_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define FILESYS_TBL_DUMP_EID      (FILESYS_TBL_BASE_EID + 0)
#define FILESYS_TBL_LOAD_EID      (FILESYS_TBL_BASE_EID + 1)
#define FILESYS_TBL_VALIDATE_EID  (FILESYS_TBL_BASE_EID + 2)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Table - Local table copy used for table loads
** 
*/

typedef struct
{

   EdsDataType_FILE_MGR_FileSysTblEntries_t  Entry;
   
} FILESYS_TBL_Data_t;


/*
** Table load callback function
*/
typedef bool (*FILESYS_TBL_LoadFunc_t)(FILESYS_TBL_Data_t *);


/******************************************************************************
** Class
*/

// Create CJSON_FILESYS_TBL_t
CJSON_STATE_STRUCT(FILESYS_TBL, FILESYS_TBL_JSON_FILE_MAX_CHAR);

typedef struct
{

   /*
   ** Table Data
   */
   
   FILESYS_TBL_Data_t     Data;
   FILESYS_TBL_LoadFunc_t LoadFunc; 
   
   CJSON_FILESYS_TBL_t    Json;
   
} FILESYS_TBL_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILESYS_TBL_Constructor
**
** Initialize the File System table object.
**
** Notes:
**   1. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void FILESYS_TBL_Constructor(FILESYS_TBL_Class_t *TblObj, 
                             FILESYS_TBL_LoadFunc_t LoadFunc);


/******************************************************************************
** Function: FILESYS_TBL_DumpCmd
**
** Command to write the table data from memory to a JSON file.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**
*/
bool FILESYS_TBL_DumpCmd(osal_id_t FileHandle);


/******************************************************************************
** Function: FILESYS_TBL_LoadCmd
**
** Command to copy the table data from a JSON file to memory.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has validated the file exists.
**
*/
bool FILESYS_TBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);


/******************************************************************************
** Function: FILESYS_TBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void FILESYS_TBL_ResetStatus(void);


#endif /* _filesys_tbl_ */

