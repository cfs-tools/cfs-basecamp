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
**    Implement the File System table
**
**  Notes:
**    1. The static "TblData" serves as a table load buffer. Table dump
**       data is read directly from table owner's table storage. 
**    2. Describe different table load scenarios
*/

/*
** Include Files:
*/

#include <string.h>
#include "filesys_tbl.h"


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

static FILESYS_TBL_Class_t  *FileSysTbl = NULL;

static FILESYS_TBL_Data_t   TblData; /* Working buffer for loads */

static CJSON_Obj_t JsonTblObjs[] = {

   /* Table Data Address        Table Data Length  Updated, Data Type,  Float,  core-json query string, length of query string(exclude '\0') */
   
   { &TblData.Entry[0].State,   sizeof(uint8),     false,   JSONNumber, false, { "entry[0].state",      (sizeof("entry[0].state")-1)}  },
   { &TblData.Entry[0].Type,    sizeof(uint8),     false,   JSONNumber, false, { "entry[0].type",       (sizeof("entry[0].type")-1)}   },
   { &TblData.Entry[0].Name,    OS_MAX_PATH_LEN,   false,   JSONString, false, { "entry[0].name",       (sizeof("entry[0].name")-1)}   },

   { &TblData.Entry[1].State,   sizeof(uint8),     false,   JSONNumber, false, { "entry[1].state",      (sizeof("entry[1].state")-1)}  },
   { &TblData.Entry[1].Type,    sizeof(uint8),     false,   JSONNumber, false, { "entry[1].type",       (sizeof("entry[1].type")-1)}   },
   { &TblData.Entry[1].Name,    OS_MAX_PATH_LEN,   false,   JSONString, false, { "entry[1].name",       (sizeof("entry[1].name")-1)}   },

   { &TblData.Entry[2].State,   sizeof(uint8),     false,   JSONNumber, false, { "entry[2].state",      (sizeof("entry[2].state")-1)}  },
   { &TblData.Entry[2].Type,    sizeof(uint8),     false,   JSONNumber, false, { "entry[2].type",       (sizeof("entry[2].type")-1)}   },
   { &TblData.Entry[2].Name,    OS_MAX_PATH_LEN,   false,   JSONString, false, { "entry[2].name",       (sizeof("entry[2].name")-1)}   },

   { &TblData.Entry[3].State,   sizeof(uint8),     false,   JSONNumber, false, { "entry[3].state",      (sizeof("entry[3].state")-1)}  },
   { &TblData.Entry[3].Type,    sizeof(uint8),     false,   JSONNumber, false, { "entry[3].type",       (sizeof("entry[3].type")-1)}   },
   { &TblData.Entry[3].Name,    OS_MAX_PATH_LEN,   false,   JSONString, false, { "entry[3].name",       (sizeof("entry[3].name")-1)}   }

};


/******************************************************************************
** Function: FILESYS_TBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void FILESYS_TBL_Constructor(FILESYS_TBL_Class_t *FileSysTblPtr, 
                             FILESYS_TBL_LoadFunc_t LoadFunc)
{

   FileSysTbl = FileSysTblPtr;

   CFE_PSP_MemSet(FileSysTbl, 0, sizeof(FILESYS_TBL_Class_t));
 
   FileSysTbl->LoadFunc    = LoadFunc;
   FileSysTbl->Json.ObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
         
} /* End FILESYS_TBL_Constructor() */


/******************************************************************************
** Function: FILESYS_TBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. TBLMGR opens the JSON dump file and writes standard JSON header objects.
**     This only writes table-specific JSON objects excluding the final closing
**     bracket } for the table's main JSON object. 
**  3. File is formatted so it can be used as a load file.
*/
bool FILESYS_TBL_DumpCmd(osal_id_t FileHandle)
{

   uint16  i;
   char    DumpRecord[256];
   
   sprintf(DumpRecord,"   \"entry\": [\n");
   OS_write(FileHandle, DumpRecord, strlen(DumpRecord));
   
   for (i=0; i < EdsParam_FILE_MGR_FILESYS_TBL_ENTRY_CNT; i++)
   {
      if (i > 0)
      {
         sprintf(DumpRecord,",\n");
         OS_write(FileHandle, DumpRecord, strlen(DumpRecord));      
      }
      sprintf(DumpRecord,"   {\n      \"state\": %d,\n      \"type\": %d,\n      \"name\": \"%s\"\n   }",
              FileSysTbl->Data.Entry[i].State,
              FileSysTbl->Data.Entry[i].Type,
              FileSysTbl->Data.Entry[i].Name);
      OS_write(FileHandle, DumpRecord, strlen(DumpRecord));
   }
    
   sprintf(DumpRecord,"\n   ]");
   OS_write(FileHandle, DumpRecord, strlen(DumpRecord));

   return true;
   
} /* End of FILESYS_TBL_DumpCmd() */


/******************************************************************************
** Function: FILESYS_TBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. The table load status is part of the TBLMGR_Tbl_t data.
*/
bool FILESYS_TBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, FileSysTbl->Json.Buf, FILESYS_TBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      FileSysTbl->Json.Loaded = true;
      RetStatus = true;   
      if (FileSysTbl->LoadFunc != NULL)
      {
         RetStatus = (FileSysTbl->LoadFunc)(&TblData);
      }
   }

   return RetStatus;
   
} /* End FILESYS_TBL_LoadCmd() */


/******************************************************************************
** Function: FILESYS_TBL_ResetStatus
**
*/
void FILESYS_TBL_ResetStatus(void)
{

   FileSysTbl->Json.LastLoadCnt = 0;
 
} /* End FILESYS_TBL_ResetStatus() */


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


   FileSysTbl->Json.FileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &FileSysTbl->Data, sizeof(FILESYS_TBL_Data_t));
   
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, FileSysTbl->Json.ObjCnt, FileSysTbl->Json.Buf, FileSysTbl->Json.FileLen);

   /* Only accept full table loads for the initial load */
   if (!FileSysTbl->Json.Loaded && (ObjLoadCnt != FileSysTbl->Json.ObjCnt))
   {

      CFE_EVS_SendEvent(FILESYS_TBL_LOAD_EID, CFE_EVS_EventType_ERROR, 
                        "Table has never been loaded and new table only contains %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)FileSysTbl->Json.ObjCnt);
   
   }
   else
   {
      /*
      ** A valid table can have disabled entries that are not valid and the Set Table
      ** State command will be rejected for these entries.
      */
      if (ValidTblData())
      {
         memcpy(&FileSysTbl->Data,&TblData, sizeof(FILESYS_TBL_Data_t));
         FileSysTbl->Json.LastLoadCnt = ObjLoadCnt;
         CFE_EVS_SendEvent(FILESYS_TBL_LOAD_EID, CFE_EVS_EventType_DEBUG, 
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
**  1. Validates new table data before it is accepted. Criteria includes:
**     - Table state must be enabled, disabled or unused
**     - Enabled entries must have a valid type and file system name
**     - Disabled entries must have a valid type
**     - File system names for disabled & unused entries are ignored
**  2. Sets the 'valid' boolean flag for each entry
*/
static bool ValidTblData(void)
{
   bool  RetStatus;
   bool  ValidState;
   bool  ValidType;
   bool  ValidName;
   
   uint16  FilenameLen;
   uint16  ValidEntries   = 0;
   uint16  InvalidEntries = 0;
   uint16  WarningEntries = 0;
   uint16  UnusedEntries  = 0;
   
  
   for (uint8 i=0; i < EdsParam_FILE_MGR_FILESYS_TBL_ENTRY_CNT; i++)
   {
      TblData.Entry[i].Valid = false;
      
      ValidState = (TblData.Entry[i].State >= EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MIN &&
                    TblData.Entry[i].State <= EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MAX);
      ValidType  = (TblData.Entry[i].Type >= EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MIN &&
                    TblData.Entry[i].Type <= EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MAX);
      
      FilenameLen = FileUtil_GetFilenameLen(TblData.Entry[i].Name);
      ValidName = (FilenameLen > 0);
      if (ValidName)
      {
         ValidName = (TblData.Entry[i].Name[0] == '/');
      }
               
      if (ValidState)
      {
         if (TblData.Entry[i].State == APP_C_FW_ConfigEnaTriState_ENABLED)
         {
            if (ValidType)
            {
               if (ValidName)
               {
                  ValidEntries++;
                  TblData.Entry[i].Valid = true;
               }
               else
               {
                  InvalidEntries++;
                  TblData.Entry[i].Name[sizeof(EdsDataType_BASE_TYPES_PathName_t)-1] = '\0';
                  CFE_EVS_SendEvent(FILESYS_TBL_VALIDATE_EID, CFE_EVS_EventType_ERROR,
                                    "FileSys table rejected, enabled entry %d has an invalid name %s",
                                    i, TblData.Entry[i].Name);
               }
            }
            else
            {
               InvalidEntries++;
               CFE_EVS_SendEvent(FILESYS_TBL_VALIDATE_EID, CFE_EVS_EventType_ERROR,
                                 "FileSys table rejected, enabled entry %d has an invalid type %d. Type must be between %d <= Type <= %d",
                                 i, TblData.Entry[i].Type,
                                 EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MIN,
                                 EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MAX);               
            }
         } /* End if enabled */
         else
         {
            if (TblData.Entry[i].State == APP_C_FW_ConfigEnaTriState_DISABLED)            
            {
               bool Warning = false;
               /*
               ** Warnings are issued for disabled entries with invalid entries
               ** because a set state command will be rejected if an attempt is
               ** made to enable a disabled entry with errors.
               */
               if (!ValidType)
               {
                  Warning = true;
                  CFE_EVS_SendEvent(FILESYS_TBL_VALIDATE_EID, CFE_EVS_EventType_DEBUG,
                                    "Warning disabled entry %d has an invalid type %d. Type must be between %d <= Type <= %d",
                                    i, TblData.Entry[i].Type,
                                    EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MIN,
                                    EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MAX); 
               }
               if (FilenameLen > 0 && !ValidName)
               {
                  Warning = true;
                  CFE_EVS_SendEvent(FILESYS_TBL_VALIDATE_EID, CFE_EVS_EventType_DEBUG,
                                    "Warning disabled entry %d has an invalid file system name", i); 
               }
               if (Warning) WarningEntries++;
            }
            else
            {
               UnusedEntries++;
            }
            
         } /* End if disabled */
      } /* End valid state */
      else
      {
         InvalidEntries++;
         CFE_EVS_SendEvent(FILESYS_TBL_VALIDATE_EID, CFE_EVS_EventType_ERROR,
                           "FileSys table rejected, entry %d has an invalid state %d. State must be between %d <= State <= %d",
                           i, TblData.Entry[i].State,
                           EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MIN,
                           EdsDataType_EdsEnum_APP_C_FW_ConfigEnaTriState_t_MAX);
      }
   } /* End entry loop */
   
   CFE_EVS_SendEvent(FILESYS_TBL_VALIDATE_EID, CFE_EVS_EventType_INFORMATION,
                     "File system table entry validation counts: valid: %d, invalid: %d, warning: %d, unused: %d",
                     ValidEntries, InvalidEntries, WarningEntries, UnusedEntries);

   RetStatus = (InvalidEntries == 0);

   return RetStatus;
   
} /* End ValidTblData() */
