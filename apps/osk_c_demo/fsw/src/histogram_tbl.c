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
**    Implement the Histogram table
**
**  Notes:
**    1. The static "TblData" serves as a table load buffer. Table dump data is
**       read directly from table owner's table storage.
**    2. The number of bins defined in the JSON bin array must match the maximum
**       bin count. The JSON bin-cnt can be less than the maimum count, however
**       the unused bin array entries must still be defined. The bins are
**       processed sequentially so unused bins are at the end of the array. 
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "histogram_tbl.h"


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

static HISTOGRAM_TBL_Class_t* HistogramTbl = NULL;

static HISTOGRAM_TBL_Data_t TblData; /* Working buffer for loads */

static CJSON_Obj_t JsonTblObjs[] = {

   /* Table Data Address        Table Data Length        Updated, Data Type,  Float,  core-json query string, length of query string(exclude '\0') */
   
   { &TblData.BinCnt,           sizeof(TblData.BinCnt),  false,   JSONNumber, false, { "bin-cnt",             (sizeof("bin-cnt")-1)}             },
   { &TblData.Bin[0].LoLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[0].lo-lim",       (sizeof("bin[0].lo-lim")-1)}       },
   { &TblData.Bin[0].HiLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[0].hi-lim",       (sizeof("bin[0].hi-lim")-1)}       },
   { &TblData.Bin[1].LoLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[1].lo-lim",       (sizeof("bin[1].lo-lim")-1)}       },
   { &TblData.Bin[1].HiLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[1].hi-lim",       (sizeof("bin[1].hi-lim")-1)}       },
   { &TblData.Bin[2].LoLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[2].lo-lim",       (sizeof("bin[2].lo-lim")-1)}       },
   { &TblData.Bin[2].HiLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[2].hi-lim",       (sizeof("bin[2].hi-lim")-1)}       },
   { &TblData.Bin[3].LoLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[3].lo-lim",       (sizeof("bin[3].lo-lim")-1)}       },
   { &TblData.Bin[3].HiLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[3].hi-lim",       (sizeof("bin[3].hi-lim")-1)}       },
   { &TblData.Bin[4].LoLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[4].lo-lim",       (sizeof("bin[4].lo-lim")-1)}       },
   { &TblData.Bin[4].HiLim,     sizeof(uint16),          false,   JSONNumber, false, { "bin[4].hi-lim",       (sizeof("bin[4].hi-lim")-1)}       }   
};


/******************************************************************************
** Function: HISTOGRAM_TBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void HISTOGRAM_TBL_Constructor(HISTOGRAM_TBL_Class_t *HistogramTblPtr, 
                               HISTOGRAM_TBL_LoadFunc_t LoadFunc,
                               const char *AppName)
{

   HistogramTbl = HistogramTblPtr;

   CFE_PSP_MemSet(HistogramTbl, 0, sizeof(HISTOGRAM_TBL_Class_t));
 
   HistogramTbl->LoadFunc = LoadFunc;
   HistogramTbl->AppName  = AppName;
   HistogramTbl->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
         
} /* End HISTOGRAM_TBL_Constructor() */


/******************************************************************************
** Function: HISTOGRAM_TBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. Can assume valid table filename because this is a callback from 
**     the app framework table manager that has verified the file.
**  3. DumpType is unused.
**  4. File is formatted so it can be used as a load file. It does not follow
**     the cFE table file format. 
**  5. Creates a new dump file, overwriting anything that may have existed
**     previously
*/
bool HISTOGRAM_TBL_DumpCmd(TBLMGR_Tbl_t *Tbl, uint8 DumpType, const char *Filename)
{

   bool       RetStatus = false;
   int32      SysStatus;
   uint16     i;
   osal_id_t  FileHandle;
   os_err_name_t OsErrStr;
   char DumpRecord[256];
   char SysTimeStr[128];

   
   SysStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_CREATE, OS_READ_WRITE);

   if (SysStatus == OS_SUCCESS)
   {
 
      sprintf(DumpRecord,"{\n   \"app-name\": \"%s\",\n   \"tbl-name\": \"Histogram\",\n", HistogramTbl->AppName);
      OS_write(FileHandle, DumpRecord, strlen(DumpRecord));

      CFE_TIME_Print(SysTimeStr, CFE_TIME_GetTime());
      sprintf(DumpRecord,"   \"description\": \"Table dumped at %s\",\n",SysTimeStr);
      OS_write(FileHandle, DumpRecord, strlen(DumpRecord));

      sprintf(DumpRecord,"   \"bin-cnt\": %d,\n", HistogramTbl->Data.BinCnt);
      OS_write(FileHandle, DumpRecord, strlen(DumpRecord));

      sprintf(DumpRecord,"   \"bin\": [\n");
      OS_write(FileHandle, DumpRecord, strlen(DumpRecord));
      
      for (i=0; i < HistogramTbl->Data.BinCnt; i++)
      {
         if (i > 0)
         {
            sprintf(DumpRecord,",\n");
            OS_write(FileHandle, DumpRecord, strlen(DumpRecord));      
         }
         sprintf(DumpRecord,"   {\n         \"lo-lim\": \"%d\",\n         \"hi-lim\": %d\n      }",
                 HistogramTbl->Data.Bin[i].LoLim, HistogramTbl->Data.Bin[i].HiLim);
         OS_write(FileHandle, DumpRecord, strlen(DumpRecord));
      }
       
      sprintf(DumpRecord,"   ]\n}\n");
      OS_write(FileHandle, DumpRecord, strlen(DumpRecord));

      OS_close(FileHandle);

      CFE_EVS_SendEvent(HISTOGRAM_TBL_DUMP_EID, CFE_EVS_EventType_DEBUG,
                        "Successfully created dump file %s", Filename);

      RetStatus = true;

   } /* End if file create */
   else
   {
      OS_GetErrorName(SysStatus, &OsErrStr);
      CFE_EVS_SendEvent(HISTOGRAM_TBL_DUMP_EID, CFE_EVS_EventType_ERROR,
                        "Error creating dump file '%s', status=%s",
                        Filename, OsErrStr);
   
   } /* End if file create error */

   return RetStatus;
   
} /* End of HISTOGRAM_TBL_DumpCmd() */


/******************************************************************************
** Function: HISTOGRAM_TBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. This could migrate into table manager but I think I'll keep it here so
**     user's can add table processing code if needed.
*/
bool HISTOGRAM_TBL_LoadCmd(TBLMGR_Tbl_t *Tbl, uint8 LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, HistogramTbl->JsonBuf, HISTOGRAM_TBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      HistogramTbl->Loaded = true;
      HistogramTbl->LastLoadStatus = TBLMGR_STATUS_VALID;
      if (HistogramTbl->LoadFunc != NULL) (HistogramTbl->LoadFunc)();
      RetStatus = true;   
   }
   else
   {
      HistogramTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;
   }

   return RetStatus;
   
} /* End HISTOGRAM_TBL_LoadCmd() */


/******************************************************************************
** Function: HISTOGRAM_TBL_ResetStatus
**
*/
void HISTOGRAM_TBL_ResetStatus(void)
{

   HistogramTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;
   HistogramTbl->LastLoadCnt = 0;
 
} /* End HISTOGRAM_TBL_ResetStatus() */


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


   HistogramTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &HistogramTbl->Data, sizeof(HISTOGRAM_TBL_Data_t));
   
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, HistogramTbl->JsonObjCnt, HistogramTbl->JsonBuf, HistogramTbl->JsonFileLen);

   /* Only accept fixed sized bin arrays */
   if (!HistogramTbl->Loaded && (ObjLoadCnt != HistogramTbl->JsonObjCnt))
   {

      CFE_EVS_SendEvent(HISTOGRAM_TBL_LOAD_EID, CFE_EVS_EventType_ERROR, 
                        "Table has never been loaded and new table only contains %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)HistogramTbl->JsonObjCnt);
   
   }
   else
   {
   
      memcpy(&HistogramTbl->Data,&TblData, sizeof(HISTOGRAM_TBL_Data_t));
      HistogramTbl->LastLoadCnt = ObjLoadCnt;
      CFE_EVS_SendEvent(HISTOGRAM_TBL_LOAD_EID, CFE_EVS_EventType_DEBUG, 
                        "Successfully loaded %d JSON objects",
                        (unsigned int)ObjLoadCnt);
      RetStatus = true;
      
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

