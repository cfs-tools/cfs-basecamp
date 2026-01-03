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
**       bin count. The JSON bin-cnt can be less than the maximum count, however
**       the unused bin array entries must still be defined. The bins are
**       processed sequentially so unused bins are at the end of the array. 
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
static bool ValidTblData(void);

/**********************/
/** Global File Data **/
/**********************/

static HISTOGRAM_TBL_Class_t  *HistogramTbl = NULL;

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
                               HISTOGRAM_TBL_LoadFunc_t LoadFunc)
{

   HistogramTbl = HistogramTblPtr;

   CFE_PSP_MemSet(HistogramTbl, 0, sizeof(HISTOGRAM_TBL_Class_t));
 
   HistogramTbl->LoadFunc   = LoadFunc;
   HistogramTbl->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
         
} /* End HISTOGRAM_TBL_Constructor() */


/******************************************************************************
** Function: HISTOGRAM_TBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. TBLMGR opens the JSON dump file and writes standard JSON header objects.
**     This only writes table-specific JSON objects excluding the final closing
**     bracket } for the table's main JSON object. 
**  3. File is formatted so it can be used as a load file.
*/
bool HISTOGRAM_TBL_DumpCmd(osal_id_t FileHandle)
{

   uint16  i;
   char    DumpRecord[256];
   
   
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
      sprintf(DumpRecord,"   {\n      \"lo-lim\": %d,\n      \"hi-lim\": %d\n   }",
              HistogramTbl->Data.Bin[i].LoLim, HistogramTbl->Data.Bin[i].HiLim);
      OS_write(FileHandle, DumpRecord, strlen(DumpRecord));
   }
    
   sprintf(DumpRecord,"]\n");
   OS_write(FileHandle, DumpRecord, strlen(DumpRecord));

   return true;
   
} /* End of HISTOGRAM_TBL_DumpCmd() */


/******************************************************************************
** Function: HISTOGRAM_TBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. This could migrate into table manager but I think I'll keep it here so
**     user's can add table processing code if needed.
**  3. The table load status is part of the TBLMGR_Tbl_t data.
*/
bool HISTOGRAM_TBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, HistogramTbl->JsonBuf, HISTOGRAM_TBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      HistogramTbl->Loaded = true;
      if (HistogramTbl->LoadFunc != NULL) (HistogramTbl->LoadFunc)();
      RetStatus = true;   
   }

   return RetStatus;
   
} /* End HISTOGRAM_TBL_LoadCmd() */


/******************************************************************************
** Function: HISTOGRAM_TBL_ResetStatus
**
*/
void HISTOGRAM_TBL_ResetStatus(void)
{

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

      if (ValidTblData())
      {
         memcpy(&HistogramTbl->Data,&TblData, sizeof(HISTOGRAM_TBL_Data_t));
         HistogramTbl->LastLoadCnt = ObjLoadCnt;
         CFE_EVS_SendEvent(HISTOGRAM_TBL_LOAD_EID, CFE_EVS_EventType_DEBUG, 
                           "Successfully loaded %d JSON objects",
                           (unsigned int)ObjLoadCnt);
         RetStatus = true;
      }
      
   } /* End if valid JSON obj count */
   
   return RetStatus;
   
} /* End LoadJsonData() */


/******************************************************************************
** Function: ValidTblData
**
** Notes:
**  1. Validates new table data before it is accepted.
**  2. Sends an event for first detected error.
*/
static bool ValidTblData(void)
{
   bool  RetStatus = true;
   
   if (TblData.BinCnt <= HISTOGRAM_MAX_BINS)
   {
      for (uint8 i=0; i < TblData.BinCnt; i++)
      {
         if (TblData.Bin[i].HiLim <= TblData.Bin[i].LoLim)
         {
            RetStatus = false;
            CFE_EVS_SendEvent(HISTOGRAM_TBL_VALID_EID, CFE_EVS_EventType_ERROR,
                              "Histogram table rejected, invalid bin %d limits. HiLim %i <= LoLim %d",
                              i, TblData.Bin[i].HiLim, TblData.Bin[i].LoLim);
            break;
         }
      }
   }
   else
   {
      RetStatus = false;
      CFE_EVS_SendEvent(HISTOGRAM_TBL_VALID_EID, CFE_EVS_EventType_ERROR,
                        "Histogram table rejected, bin count %d exceeds maximum count %d",
                        TblData.BinCnt, HISTOGRAM_MAX_BINS);
   }
   
   return RetStatus;
   
} /* End ValidTblData() */
