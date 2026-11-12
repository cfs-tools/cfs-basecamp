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
**    Define KIT_SCH's User Message Table that defines user app messages
**    that are sent by the scheduler
**
**  Notes:
**    1. Use the Singleton design pattern. A pointer to the table object
**       is passed to the constructor and saved for all other operations.
**       This is a table-specific file so it doesn't need to be re-entrant.
**    2. User messages are periodic messages sent to a user app so they can
**       perform an action like send a housekeeping packet. The most common
**       use case is when NASA apps are downloaded/installed via the app
**       proxy feature. 
**
*/
#ifndef _usrmsgtbl_
#define _usrmsgtbl_

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

#define USRMSGTBL_LOAD_EID      (USRMSGTBL_BASE_EID + 0)
#define USRMSGTBL_LOAD_ERR_EID  (USRMSGTBL_BASE_EID + 1)
#define USRMSGTBL_DUMP_EID      (USRMSGTBL_BASE_EID + 2)
#define USRMSGTBL_DUMP_ERR_EID  (USRMSGTBL_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** User Message Table - Local table copy used for table loads
** 
*/

typedef struct
{
   
   uint16                                TopicId;
   EdsDataType_KIT_SCH_UsrMsgPeriodId_t  PeriodId;
    
} USRMSGTBL_Entry_t;


typedef struct
{

   USRMSGTBL_Entry_t Entry[USRMSGTBL_MAX_ENTRIES];

} USRMSGTBL_Data_t;


// Put header in a struct in case payload support is added
typedef struct
{
    CFE_MSG_CommandHeader_t Header;

} USRMSGTBL_CmdMsg_t;


typedef struct
{

   USRMSGTBL_CmdMsg_t Msg[USRMSGTBL_MAX_ENTRIES];

} USRMSGTBL_Commands_t;

// Create CJSON_MSGTBL_t
CJSON_STATE_STRUCT(USRMSGTBL, USRMSGTBL_JSON_FILE_MAX_CHAR);  

typedef struct
{

   /*
   ** Table parameter data
   */
   
   USRMSGTBL_Data_t Data;

   /*
   ** Command messages
   */
   
   USRMSGTBL_Commands_t Cmd;
   
   /*
   ** Event messages
   */

   uint32  InitDebugEvtType;
   uint32  LowFreqDebugEvtType;

   /*
   ** JSON Table Management
   */
   
   CJSON_USRMSGTBL_t  Json;
   
} USRMSGTBL_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: USRMSGTBL_Constructor
**
** Initialize the User Message Table object.
**
** Notes:
**   1. This must be called prior to any other function.
**   2. The local table data is not populated. This is done when the table is 
**      registered with the app framework table manager.
*/
void USRMSGTBL_Constructor(USRMSGTBL_Class_t *ObjPtr, const INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: USRMSGTBL_DumpCmd
**
** Command to dump the table.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**
*/
bool USRMSGTBL_DumpCmd(osal_id_t FileHandle);


/******************************************************************************
** Function: USRMSGTBL_LoadCmd
**
** Command to load the table.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file exists.
**
*/
bool USRMSGTBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);


/******************************************************************************
** Function: USRMSGTBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
** Notes:
**   1. See the USRMSGTBL_Class_t definition for the affected data.
**
*/
void USRMSGTBL_ResetStatus(void);


#endif /* _usrmsgtbl_ */
