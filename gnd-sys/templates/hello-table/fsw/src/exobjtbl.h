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
**    Define Example Object Table
**
**  Notes:
**   1. Use the Singleton design pattern. A pointer to the table object
**      is passed to the constructor and saved for all other operations.
**      This is a table-specific file so it doesn't need to be re-entrant.
**   2. The first JSON table must define all parameters. After a complete
**      table has been loaded then partial tables can be loaded.
**
*/
#ifndef _exobjtbl_
#define _exobjtbl_

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

#define EXOBJTBL_DUMP_EID   (EXOBJTBL_BASE_EID + 0)
#define EXOBJTBL_LOAD_EID   (EXOBJTBL_BASE_EID + 1)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Table - Local table copy used for table loads
** 
*/

typedef struct
{

   uint16   Low;
   uint16   High;

} EXOBJTBL_Limit_t;

typedef struct
{
   EXOBJTBL_Limit_t IncrLimit;
   EXOBJTBL_Limit_t DecrLimit;

} EXOBJTBL_Data_t;


/*
** Accept Table Load callback function
*/
typedef bool (*EXOBJTBL_AcceptLoadFunc_t)(const EXOBJTBL_Data_t *TblData);


typedef struct
{

   /*
   ** Table parameter data
   */
   
   EXOBJTBL_Data_t           Data;
   EXOBJTBL_AcceptLoadFunc_t AcceptLoadFunc; 
      
   /*
   ** Standard CJSON table data
   */
   
   bool         Loaded;       /* Has entire table been loaded? */
   uint16       LastLoadCnt;
   
   size_t       JsonObjCnt;
   char         JsonBuf[EXOBJTBL_JSON_FILE_MAX_CHAR];   
   size_t       JsonFileLen;
   
} EXOBJTBL_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: EXOBJTBL_Constructor
**
** Initialize the example table object.
**
** Notes:
**   1. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void EXOBJTBL_Constructor(EXOBJTBL_Class_t *ExObjTblPtr, const INITBL_Class_t *IniTbl,
                          EXOBJTBL_AcceptLoadFunc_t AcceptLoadFunc);


/******************************************************************************
** Function: EXOBJTBL_DumpCmd
**
** Command to dump the table.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**
*/
bool EXOBJTBL_DumpCmd(osal_id_t FileHandle);


/******************************************************************************
** Function: EXOBJTBL_LoadCmd
**
** Command to load the table.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
bool EXOBJTBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);


/******************************************************************************
** Function: EXOBJTBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void EXOBJTBL_ResetStatus(void);


#endif /* _exobjtbl_ */

