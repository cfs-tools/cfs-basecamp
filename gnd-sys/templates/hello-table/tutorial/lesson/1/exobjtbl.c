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
**    Implement the Example Object table
**
**  Notes:
**   1. The static "TblData" serves as a table load buffer. Table dump data is
**      read directly from table owner's table storage.
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "exobjtbl.h"


/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/


/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool LoadJsonData(size_t JsonFileLen);


/**********************/
/** Global File Data **/
/**********************/

static EXOBJTBL_Class_t* ExObjTbl = NULL;

static EXOBJTBL_Data_t TblData; /* Working buffer for loads */

//EX1,9,3,
static CJSON_Obj_t JsonTblObjs[] = {

   /* Table Data Address       Table Data Length                Updated, Data Type,  float,    JSON string,  core-json query length excludes '\0' */
   
   { &TblData.IncrLimit.Low,   sizeof(TblData.IncrLimit.Low),   false,   JSONNumber, false,  { "increment.low-limit",  (sizeof("increment.low-limit")-1)}  },
   { &TblData.IncrLimit.High,  sizeof(TblData.IncrLimit.High),  false,   JSONNumber, false,  { "increment.high-limit", (sizeof("increment.high-limit")-1)} },
   
   { &TblData.DecrLimit.Low,   sizeof(TblData.DecrLimit.Low),   false,   JSONNumber, false,  { "decrement.low-limit",  (sizeof("decrement.low-limit")-1)}  },
   { &TblData.DecrLimit.High,  sizeof(TblData.DecrLimit.High),  false,   JSONNumber, false,  { "decrement.high-limit", (sizeof("decrement.high-limit")-1)} },

   { &TblData.LimitRangeMax,   sizeof(TblData.LimitRangeMax),   false,   JSONNumber, false,  { "limit-range-max",      (sizeof("limit-range-max")-1)}      }

};
//EX1 


/******************************************************************************
** Function: EXOBJTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void EXOBJTBL_Constructor(EXOBJTBL_Class_t *ExObjTblPtr, const INITBL_Class_t *IniTbl,
                          EXOBJTBL_AcceptLoadFunc_t AcceptLoadFunc)
{

   ExObjTbl = ExObjTblPtr;

   CFE_PSP_MemSet(ExObjTbl, 0, sizeof(EXOBJTBL_Class_t));

   ExObjTbl->AcceptLoadFunc = AcceptLoadFunc;
   ExObjTbl->AppName        = INITBL_GetStrConfig(IniTbl, CFG_APP_CFE_NAME);
   ExObjTbl->JsonObjCnt     = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
         
} /* End EXOBJTBL_Constructor() */


/******************************************************************************
** Function: EXOBJTBL_ResetStatus
**
*/
void EXOBJTBL_ResetStatus(void)
{

   ExObjTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;
   ExObjTbl->LastLoadCnt = 0;
 
} /* End EXOBJTBL_ResetStatus() */


/******************************************************************************
** Function: EXOBJTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. This could migrate into table manager but I think I'll keep it here so
**     user's can add table processing code if needed.
*/
bool EXOBJTBL_LoadCmd(TBLMGR_Tbl_t *Tbl, uint8 LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, ExObjTbl->JsonBuf, EXOBJTBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      
      ExObjTbl->Loaded = true;
      ExObjTbl->LastLoadStatus = TBLMGR_STATUS_VALID;
      RetStatus = true;
   
   }
   else
   {

      ExObjTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;

   }

   return RetStatus;
   
} /* End EXOBJTBL_LoadCmd() */


/******************************************************************************
** Function: EXOBJTBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. File is formatted so it can be used as a load file.
*/
//EX2,10,6,
bool EXOBJTBL_DumpCmd(TBLMGR_Tbl_t *Tbl, uint8 DumpType, const char *Filename)
{

   char    DumpRecord[256];

   sprintf(DumpRecord,"   \"increment\":\n   {\n      \"low-limit\": %d,\n      \"high-limit\": %d\n   },\n",
           ExObjTbl->Data.IncrLimit.Low, ExObjTbl->Data.IncrLimit.High);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

   sprintf(DumpRecord,"   \"decrement\":\n   {\n      \"low-limit\": %d,\n      \"high-limit\": %d\n   },\n",
           ExObjTbl->Data.DecrLimit.Low, ExObjTbl->Data.DecrLimit.High);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

   sprintf(DumpRecord,"   \"limit-range-max\": %d", ExObjTbl->Data.LimitRangeMax);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

   return true;
   
} /* End of EXOBJTBL_DumpCmd() */
//EX2


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. See file prologue for full/partial table load scenarios
*/
static bool LoadJsonData(size_t JsonFileLen)
{

   bool      RetStatus = false;
   size_t    ObjLoadCnt;


   ExObjTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &ExObjTbl->Data, sizeof(EXOBJTBL_Data_t));
   
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, ExObjTbl->JsonObjCnt, ExObjTbl->JsonBuf, ExObjTbl->JsonFileLen);

   if (!ExObjTbl->Loaded && (ObjLoadCnt != ExObjTbl->JsonObjCnt))
   {

      CFE_EVS_SendEvent(EXOBJTBL_LOAD_EID, CFE_EVS_EventType_ERROR, 
                        "Table has never been loaded and new table only contains %d of %d data objects",
                        (int)ObjLoadCnt, (int)ExObjTbl->JsonObjCnt);
   
   }
   else
   {
      bool CopyTblData = true;
      
      if (ExObjTbl->AcceptLoadFunc != NULL)
      {
         CopyTblData = (ExObjTbl->AcceptLoadFunc)(&TblData);
      }
      
      if (CopyTblData)
      {
         memcpy(&ExObjTbl->Data, &TblData, sizeof(EXOBJTBL_Data_t));
         ExObjTbl->LastLoadCnt = ObjLoadCnt;
         RetStatus = true;
      }         

   } /* End if valid JSON object count */
   
   return RetStatus;
   
} /* End LoadJsonData() */
