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
**    Define KIT_SCH's Message Table that provides the messages to 
**    be sent by the scheduler
**
**  Notes:
**    1. Use the Singleton design pattern. A pointer to the table object
**       is passed to the constructor and saved for all other operations.
**       This is a table-specific file so it doesn't need to be re-entrant.
**    2. The table file is a JSON text file.
**
*/
#ifndef _msgtbl_
#define _msgtbl_

/*
** Includes
*/

#include "app_cfg.h"
#include "cjson.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define MSGTBL_LOAD_EID      (MSGTBL_BASE_EID + 0)
#define MSGTBL_LOAD_ERR_EID  (MSGTBL_BASE_EID + 1)
#define MSGTBL_DUMP_EID      (MSGTBL_BASE_EID + 2)
#define MSGTBL_DUMP_ERR_EID  (MSGTBL_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Message Table -  Local table copy used for table loads
** 
*/

typedef struct
{
   
   uint16  Buffer[MSGTBL_MAX_MSG_WORDS];
   uint16  PayloadWordCnt;
    
} MSGTBL_Entry_t;


typedef struct
{

   MSGTBL_Entry_t Entry[MSGTBL_MAX_ENTRIES];

} MSGTBL_Data_t;


typedef struct
{
    CFE_MSG_CommandHeader_t Header;
    uint16                  Payload[MSGTBL_MAX_MSG_WORDS - sizeof(CFE_MSG_CommandHeader_t)/2];

} MSGTBL_CmdMsg_t;


typedef struct
{

   MSGTBL_CmdMsg_t Msg[MSGTBL_MAX_ENTRIES];

} MSGTBL_Commands_t;

typedef struct
{

   /*
   ** Table parameter data
   */
   
   MSGTBL_Data_t Data;

   /*
   ** Command messages
   */
   
   MSGTBL_Commands_t Cmd;

   /*
   ** Standard CJSON table data
   */
   
   bool         Loaded;   /* Has entire table been loaded? */
   uint16       LastLoadCnt;
   
   size_t       JsonObjCnt;
   char         JsonBuf[MSGTBL_JSON_FILE_MAX_CHAR];   
   size_t       JsonFileLen;
   
} MSGTBL_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: MSGTBL_Constructor
**
** Initialize the Message Table object.
**
** Notes:
**   1. This must be called prior to any other function.
**   2. The local table data is not populated. This is done when the table is 
**      registered with the app framework table manager.
*/
void MSGTBL_Constructor(MSGTBL_Class_t *ObjPtr);


/******************************************************************************
** Function: MSGTBL_DumpCmd
**
** Command to dump the table.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**
*/
bool MSGTBL_DumpCmd(osal_id_t FileHandle);


/******************************************************************************
** Function: MSGTBL_LoadCmd
**
** Command to load the table.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file exists.
**
*/
bool MSGTBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);


/******************************************************************************
** Function: MSGTBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
** Notes:
**   1. See the MSGTBL_Class_t definition for the affected data.
**
*/
void MSGTBL_ResetStatus(void);


#endif /* _msgtbl_ */
