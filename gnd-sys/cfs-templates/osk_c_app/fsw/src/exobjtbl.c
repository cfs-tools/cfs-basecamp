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
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
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
 
static CJSON_Obj_t JsonTblObjs[] = {

   /* Table Data Address    Table Data Length           Updated, Data Type,  float,    JSON string,  core-json query length excludes '\0' */
   
   { &TblData.LowLimit,     sizeof(TblData.LowLimit),   false,   JSONNumber, false,  { "low-limit",  (sizeof("low-limit")-1)}      },
   { &TblData.HighLimit,    sizeof(TblData.HighLimit),  false,   JSONNumber, false,  { "high-limit", (sizeof("high-limit")-1)}     }
   
};


/******************************************************************************
** Function: EXOBJTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void EXOBJTBL_Constructor(EXOBJTBL_Class_t *ExObjTblPtr, const INITBL_Class_t *IniTbl)
{

   ExObjTbl = ExObjTblPtr;

   CFE_PSP_MemSet(ExObjTbl, 0, sizeof(EXOBJTBL_Class_t));

   ExObjTbl->AppName = INITBL_GetStrConfig(IniTbl, CFG_APP_CFE_NAME);
   ExObjTbl->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
         
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
bool EXOBJTBL_LoadCmd(TBLMGR_Tbl_t* Tbl, uint8 LoadType, const char* Filename)
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
**  2. Can assume valid table filename because this is a callback from 
**     the app framework table manager that has verified the file.
**  3. DumpType is unused.
**  4. File is formatted so it can be used as a load file.
**  5. Creates a new dump file, overwriting anything that may have existed
**     previously
*/
bool EXOBJTBL_DumpCmd(TBLMGR_Tbl_t *Tbl, uint8 DumpType, const char *Filename)
{

   bool       RetStatus = false;
   int32      SysStatus;
   osal_id_t  FileHandle;
   os_err_name_t OsErrStr;
   char DumpRecord[256];
   char SysTimeStr[128];

   
   SysStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_CREATE, OS_READ_WRITE);

   if (SysStatus == OS_SUCCESS)
   {
 
      sprintf(DumpRecord,"{\n   \"app-name\": \"%s\",\n   \"tbl-name\": \"Limits\",\n",ExObjTbl->AppName);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      CFE_TIME_Print(SysTimeStr, CFE_TIME_GetTime());
      sprintf(DumpRecord,"   \"description\": \"Table dumped at %s\",\n",SysTimeStr);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      sprintf(DumpRecord,"     \"low-limit\": %d\n   },\n", ExObjTbl->Data.LowLimit);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"     \"low-limit\": %d\n   }\n}\n", ExObjTbl->Data.HighLimit);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      OS_close(FileHandle);

      RetStatus = true;

   } /* End if file create */
   else
   {
      OS_GetErrorName(SysStatus, &OsErrStr);
      CFE_EVS_SendEvent(EXOBJTBL_DUMP_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error creating dump file '%s', status=%s",
                        Filename, OsErrStr);
   
   } /* End if file create error */

   return RetStatus;
   
} /* End of EXOBJTBL_DumpCmd() */


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

      CFE_EVS_SendEvent(EXOBJTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Table has never been loaded and new table only contains %ld of %ld data objects",
                        ObjLoadCnt, ExObjTbl->JsonObjCnt);
   
   }
   else
   {
      if (TblData.LowLimit >= TblData.HighLimit)
      {
         CFE_EVS_SendEvent(EXOBJTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR, 
                           "Table rejected. Low Limit %d must be less than high limit %d",
                           TblData.LowLimit, TblData.HighLimit);
      }
      else
      {
         memcpy(&ExObjTbl->Data,&TblData, sizeof(EXOBJTBL_Data_t));
         ExObjTbl->LastLoadCnt = ObjLoadCnt;
         RetStatus = true;
      }
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */
