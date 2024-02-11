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
**    Manage the Histogram defintion table
**
**  Notes:
**    1. Use the Singleton design pattern. A pointer to the table object
**       is passed to the constructor and saved for all other operations.
**       This is a table-specific file so it doesn't need to be re-entrant.
**
*/

#ifndef _histogram_tbl_
#define _histogram_tbl_

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

#define HISTOGRAM_TBL_DUMP_EID   (HISTOGRAM_TBL_BASE_EID + 0)
#define HISTOGRAM_TBL_LOAD_EID   (HISTOGRAM_TBL_BASE_EID + 1)
#define HISTOGRAM_TBL_VALID_EID  (HISTOGRAM_TBL_BASE_EID + 2)

/**********************/
/** Type Definitions **/
/**********************/

/*
** Table load callback function
*/
typedef void (*HISTOGRAM_TBL_LoadFunc_t)(void);


/******************************************************************************
** Table - Local table copy used for table loads
** 
*/

typedef struct
{

   uint16  LoLim;
   uint16  HiLim;

} HISTOGRAM_TBL_Bin_t;


typedef struct
{

   uint16               BinCnt;
   HISTOGRAM_TBL_Bin_t  Bin[HISTOGRAM_MAX_BINS];
   
} HISTOGRAM_TBL_Data_t;


/******************************************************************************
** Class
*/

typedef struct
{

   /*
   ** Table Data
   */
   
   HISTOGRAM_TBL_Data_t     Data;
   HISTOGRAM_TBL_LoadFunc_t LoadFunc; 
   
   /*
   ** Standard CJSON table data
   */
   
   bool         Loaded;   /* Has entire table been loaded? */
   uint16       LastLoadCnt;
   
   size_t       JsonObjCnt;
   char         JsonBuf[HISTOGRAM_TBL_JSON_FILE_MAX_CHAR];   
   size_t       JsonFileLen;
   
} HISTOGRAM_TBL_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: HISTOGRAM_TBL_Constructor
**
** Initialize the Histogram table object.
**
** Notes:
**   1. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void HISTOGRAM_TBL_Constructor(HISTOGRAM_TBL_Class_t *TblObj, 
                               HISTOGRAM_TBL_LoadFunc_t LoadFunc);


/******************************************************************************
** Function: HISTOGRAM_TBL_DumpCmd
**
** Command to write the table data from memory to a JSON file.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**
*/
bool HISTOGRAM_TBL_DumpCmd(osal_id_t FileHandle);


/******************************************************************************
** Function: HISTOGRAM_TBL_LoadCmd
**
** Command to copy the table data from a JSON file to memory.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has validated the file exists.
**
*/
bool HISTOGRAM_TBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);


/******************************************************************************
** Function: HISTOGRAM_TBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void HISTOGRAM_TBL_ResetStatus(void);


#endif /* _histogram_tbl_ */

