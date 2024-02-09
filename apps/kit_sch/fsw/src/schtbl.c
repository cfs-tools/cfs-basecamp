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
**    Implement KIT_SCH's Schedule Table management functions
**
**  Notes:
**    None
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "schtbl.h"

/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/

/* See LoadJsonData()prologue for details */
typedef CJSON_IntObj_t JsonIndex_t;
typedef CJSON_StrObj_t JsonEnabled_t;
typedef CJSON_IntObj_t JsonPeriod_t;
typedef CJSON_IntObj_t JsonOffset_t;
typedef CJSON_IntObj_t JsonMsgIdx_t;

typedef struct
{

   JsonIndex_t    Index;
   JsonEnabled_t  Enabled;
   JsonPeriod_t   Period;
   JsonOffset_t   Offset;
   JsonMsgIdx_t   MsgIdx;

} JsonActivity_t;



typedef struct
{

   JsonIndex_t    Index;

} JsonSlot_t;

/*******************************/
/** Local Function Prototypes **/
/*******************************/


static void ConstructJsonActivity(JsonActivity_t* JsonActivity, uint16 ActivityArrayIdx, uint16 SlotArrayIdx);
static void ConstructJsonSlot(JsonSlot_t* JsonSlot, uint16 SlotArrayIdx);
static bool LoadJsonData(size_t JsonFileLen);


/**********************/
/** File Global Data **/
/**********************/

static SCHTBL_Class_t* SchTbl = NULL;
static SCHTBL_Data_t   TblData;        /* Working buffer for loads */


/******************************************************************************
** Function: SCHTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void SCHTBL_Constructor(SCHTBL_Class_t* ObjPtr, const char* AppName)
{
   
   SchTbl = ObjPtr;

   CFE_PSP_MemSet(SchTbl, 0, sizeof(SCHTBL_Class_t));

   SchTbl->AppName        = AppName;
   SchTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;

} /* End SCHTBL_Constructor() */


/******************************************************************************
** Function: SCHTBL_ResetStatus
**
*/
void SCHTBL_ResetStatus(void)
{

   SchTbl->LastLoadCnt     = 0;
   SchTbl->LastLoadStatus  = TBLMGR_STATUS_UNDEF;
   
} /* End SCHTBL_ResetStatus() */


/******************************************************************************
** Function: SCHTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
*/
bool SCHTBL_LoadCmd(TBLMGR_Tbl_t* Tbl, uint8 LoadType, const char* Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, SchTbl->JsonBuf, SCHTBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      SchTbl->Loaded = true;
      SchTbl->LastLoadStatus = TBLMGR_STATUS_VALID;
      RetStatus = true;
   }
   else
   {
      SchTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;
   }

   return RetStatus;

} /* End of SchTBL_LoadCmd() */


/******************************************************************************
** Function: SCHTBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file. If the
**     filename exists it will be overwritten.
**  3. File is formatted so it can be used as a load file. However all of the
**     entries are dumped so you will get errors on the load for unused entries
**     because unused entries have invalid  message indices.
**  4. DumpType is unused.
*/

bool SCHTBL_DumpCmd(TBLMGR_Tbl_t* Tbl, uint8 DumpType, const char* Filename)
{

   bool      RetStatus = false;
   osal_id_t FileHandle;
   int32     OsStatus;
   uint16    EntryIdx, Slot, Activity;
   char      DumpRecord[256];
   char      SysTimeStr[64];
   os_err_name_t OsErrStr;
   
   OsStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_READ_WRITE);
   
   if (OsStatus == OS_SUCCESS)
   {

      sprintf(DumpRecord,"\n{\n\"name\": \"Kit Scheduler (KIT_SCH) Scheduler Activity Table\",\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      CFE_TIME_Print(SysTimeStr, CFE_TIME_GetTime());
      
      sprintf(DumpRecord,"\"description\": \"KIT_SCH table dumped at %s\",\n",SysTimeStr);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));


      /* 
      ** - Not all fields in ground table are saved in FSW so they are not
      **   populated in the dump file. However, the dump file can still
      **   be loaded.
      ** - The slot and activity 'index' field is used to indicate whether
      **   an activity has been loaded.      
      ** 
      **   "slot-array": [
      **
      **      {"slot": {
      **         "index": 4
      **         "activity-array" : [
      **   
      **            {"activity": {
      **            "name":   "cFE ES Housekeeping",  # Not saved
      **            "descr":  "",                     # Not saved
      **            "index":   0,
      **            "enabled": "true",
      **            "period":  4,
      **            "offset":  0,
      **            "msg-idx": 0
      **         }},
      **         ...
      **      ...
      */
      
      sprintf(DumpRecord,"\"slot-array\": [\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      for (Slot=0; Slot < SCHTBL_SLOTS; Slot++)
      {
         
         if (Slot > 0)
         {
            sprintf(DumpRecord,",\n");            
            OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
         }
            
         sprintf(DumpRecord,"   {\"slot\": {\n      \"index\": %d,\n      \"activity-array\" : [\n",Slot);         
         OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
         
         for (Activity=0; Activity < SCHTBL_ACTIVITIES_PER_SLOT; Activity++)
         {
            
            EntryIdx = SCHTBL_INDEX(Slot,Activity);

            if (Activity > 0)
            {
               sprintf(DumpRecord,",\n");
               OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
            }
            
            sprintf(DumpRecord,"         {\"activity\": {\n");
            OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
            
            sprintf(DumpRecord,"         \"index\": %d,\n         \"enabled\": \"%s\",\n         \"period\": %d,\n         \"offset\": %d,\n         \"msg-idx\": %d\n      }}",
                 Activity,
                 CMDMGR_BoolStr(SchTbl->Data.Entry[EntryIdx].Enabled),
                 SchTbl->Data.Entry[EntryIdx].Period,
                 SchTbl->Data.Entry[EntryIdx].Offset,
                 SchTbl->Data.Entry[EntryIdx].MsgTblIndex); 
            OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
         
         } /* End activity loop */             
      
         sprintf(DumpRecord,"\n      ]\n   }}");
         OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      } /* End slot loop */
 
      /* Close slot-array and top-level object */
      sprintf(DumpRecord,"\n   ]\n}\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      RetStatus = true;

      OS_close(FileHandle);

   } /* End if file create */
   else
   {
      OS_GetErrorName(OsStatus, &OsErrStr);
      CFE_EVS_SendEvent(SCHTBL_DUMP_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error creating dump file %s. Status = %s",
                        Filename, OsErrStr);
   
   } /* End if file create error */

   if (RetStatus)
   {
      
      CFE_EVS_SendEvent(SCHTBL_DUMP_EID, CFE_EVS_EventType_INFORMATION,
                        "Successfully dumped scheduler table to %s", Filename);
   }
   
   return RetStatus;
   
} /* End of SCHTBL_DumpCmd() */


/******************************************************************************
** Function: SCHTBL_GetEntryPtr
**
*/
bool SCHTBL_GetEntryPtr(uint16  EntryId, SCHTBL_Entry_t **EntryPtr)
{

   bool RetStatus = false;

   if (EntryId < SCHTBL_MAX_ENTRIES)
   {

      *EntryPtr = &SchTbl->Data.Entry[EntryId];
      RetStatus = true;
 
   } /* End if valid EntryId */

   return RetStatus;

} /* End SCHTBL_GetEntryPtr() */



/******************************************************************************
** SCHTBL_GetEntryIndex
**
** Compute and load EntryIndex if the SlotIndex and ActivityIndex are valid.
** Event message text assumes commands are being validated 
*/
bool SCHTBL_GetEntryIndex(const char* EventStr, uint16 SlotIndex, 
                          uint16 ActivityIndex, uint16* EntryIndex)
{
   
   bool RetStatus = false;
   
   if (SlotIndex < SCHTBL_SLOTS)
   {

      if (ActivityIndex < SCHTBL_ACTIVITIES_PER_SLOT)
      {
         
         *EntryIndex = SCHTBL_INDEX(SlotIndex, ActivityIndex);
         RetStatus = true;
         
      }
      else
      {
         
         CFE_EVS_SendEvent (SCHTBL_CMD_ACTIVITY_ERR_EID, CFE_EVS_EventType_ERROR, 
                            "%s. Invalid activity index %d greater than max %d",
                            EventStr, ActivityIndex, (SCHTBL_ACTIVITIES_PER_SLOT-1));
      }

   } /* End if valid activity ID */
   else
   {
      
      CFE_EVS_SendEvent (SCHTBL_CMD_SLOT_ERR_EID, CFE_EVS_EventType_ERROR, 
                         "%s. Invalid slot index %d greater than max %d",
                         EventStr, SlotIndex, (SCHTBL_SLOTS-1));

   } /* End if invalid slot ID */

   return RetStatus;
   
} /* End SCHTBL_GetEntryIndex() */


/******************************************************************************
** Function: SCHTBL_ValidEntry
**
** A pointer to a structure isn't passed because this function is used to
** validate command and table parameters that may not be packed identically
** to the internal structure.
*/
bool SCHTBL_ValidEntry(const char* EventStr, uint16 Enabled, uint16 Period, 
                       uint16 Offset, uint16 MsgTblIndex)
{

   bool RetStatus = false;

   if (CMDMGR_ValidBoolArg(Enabled))
   {
      
      /* 
      ** In theory Period and Offset can have any 16-bit value so an absolute
      ** limit isn't checked. However if an Offset is greater than the Period 
      ** then it will never execute. The Enabled flag should be used if a user
      ** wants to explicitly disable a slot.
      */
      if (Offset <= Period)
      {
         
         if ( MsgTblIndex >= 0 && MsgTblIndex < MSGTBL_MAX_ENTRIES)
         {
         
           
           RetStatus = true;
            
         }
         else
         {
         
            CFE_EVS_SendEvent(SCHTBL_MSG_TBL_INDEX_ERR_EID, CFE_EVS_EventType_ERROR, 
                              "%s. Invalid msg index %d. Valid index: 0 <= Index < %d.",
                              EventStr, MsgTblIndex, MSGTBL_MAX_ENTRIES);
      
         }
      } /* End if valid offset */
      else
      {
         
         CFE_EVS_SendEvent(SCHTBL_OFFSET_ERR_EID, CFE_EVS_EventType_ERROR,
                           "%s. Offset %d is greater than Period %d",
                           EventStr, Offset, Period);    
      
      } /* End if invalid offset */            
   } /* End if valid boolean config */
   else
   {
   
      CFE_EVS_SendEvent(SCHTBL_ENTRY_ERR_EID, CFE_EVS_EventType_ERROR,
                        "%s. Invalid Enabled value %d. Must be True(%d) or False(%d)",
                        EventStr, Enabled, true, false);    
         
   } /* End if invalid boolean config */

   return RetStatus;

} /* End SCHTBL_ValidEntry() */


/******************************************************************************
** Function: ConstructJsonActivity
**
*/
static void ConstructJsonActivity(JsonActivity_t* JsonActivity, uint16 ActivityArrayIdx, uint16 SlotArrayIdx)
{

   char KeyStr[64];

   sprintf(KeyStr,"slot-array[%d].slot.activity-array[%d].activity.index", SlotArrayIdx, ActivityArrayIdx);
   CJSON_ObjConstructor(&JsonActivity->Index.Obj, KeyStr, JSONNumber, &JsonActivity->Index.Value, 4);

   sprintf(KeyStr,"slot-array[%d].slot.activity-array[%d].activity.enabled", SlotArrayIdx, ActivityArrayIdx);
   CJSON_ObjConstructor(&JsonActivity->Enabled.Obj, KeyStr, JSONString, &JsonActivity->Enabled.Value, 10);

   sprintf(KeyStr,"slot-array[%d].slot.activity-array[%d].activity.period", SlotArrayIdx, ActivityArrayIdx);
   CJSON_ObjConstructor(&JsonActivity->Period.Obj, KeyStr, JSONNumber, &JsonActivity->Period.Value, 4);

   sprintf(KeyStr,"slot-array[%d].slot.activity-array[%d].activity.offset", SlotArrayIdx, ActivityArrayIdx);
   CJSON_ObjConstructor(&JsonActivity->Offset.Obj, KeyStr, JSONNumber, &JsonActivity->Offset.Value, 4);

   sprintf(KeyStr,"slot-array[%d].slot.activity-array[%d].activity.msg-idx", SlotArrayIdx, ActivityArrayIdx);
   CJSON_ObjConstructor(&JsonActivity->MsgIdx.Obj, KeyStr, JSONNumber, &JsonActivity->MsgIdx.Value, 4);
   
} /* ConstructJsonActivity() */


/******************************************************************************
** Function: ConstructJsonSlot
**
*/
static void ConstructJsonSlot(JsonSlot_t* JsonSlot, uint16 SlotArrayIdx)
{

   char KeyStr[64];

   sprintf(KeyStr,"slot-array[%d].slot.index", SlotArrayIdx);
   CJSON_ObjConstructor(&JsonSlot->Index.Obj, KeyStr, JSONNumber, &JsonSlot->Index.Value, 4);
   
} /* ConstructJsonSlot() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. The JSON file can contain 1 to SCHTBL_MAX_ENTRIES entries. The table can
**     be sparsely populated.  
**  2. JSON activity object
**
**        "name":  Not saved,
**        "descr": Not saved,
**        "index": 12,
**        "enabled": true,
**        "period": 4,
**        "offset": 0,
**        "msg-idx": 12
**
*/
static bool LoadJsonData(size_t JsonFileLen)
{

   bool    RetStatus = true;
   bool    ReadSlot = true;
   bool    ReadActivity = false;
   uint16  EntryUdateCnt = 0;
   uint16  AttributeCnt;
   uint16  SlotIdx;
   uint16  SlotArrayIdx;
   uint16  ActivityIdx;
   uint16  ActivityArrayIdx;
   uint16  EntryIdx;

   JsonSlot_t      JsonSlot;
   JsonActivity_t  JsonActivity;
   SCHTBL_Entry_t  SchEntry;


   SchTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &SchTbl->Data, sizeof(SCHTBL_Data_t));

   SlotArrayIdx = 0;
   while (ReadSlot)
   {

      ConstructJsonSlot(&JsonSlot, SlotArrayIdx);

      /*
      ** Use 'slot' and 'activity' fields to control looping over
      ** the 'slot-array' and 'activity-array'. If either of these
      ** are missing or malformed then the array traversal will be
      ** terminated and a potential error will not be caught. Both
      ** fields are required but CJSON_LoadObjOptional() is used 
      ** so the 'object not found' event will be suppressed 
      */      
      
      if (CJSON_LoadObjOptional(&JsonSlot.Index.Obj, SchTbl->JsonBuf, SchTbl->JsonFileLen))
      {

         /* 
         ** Read JSON slot and activity values and let SCHTBL_GetEntryIndex()
         ** perform validation and send error event messages.
         */

         SlotIdx = JsonSlot.Index.Value;
         
         ReadActivity = true;
         ActivityArrayIdx = 0;
            
         while (ReadActivity)
         {

            memset((void*)&SchEntry,0,sizeof(SCHTBL_Entry_t));
            ConstructJsonActivity(&JsonActivity, ActivityArrayIdx, SlotArrayIdx);
            
            if (CJSON_LoadObjOptional(&JsonActivity.Index.Obj, SchTbl->JsonBuf, SchTbl->JsonFileLen))
            {
               
               ActivityIdx = JsonActivity.Index.Value;
               AttributeCnt = 0;
               if (CJSON_LoadObj(&JsonActivity.Enabled.Obj, SchTbl->JsonBuf, SchTbl->JsonFileLen)) AttributeCnt++;
               if (CJSON_LoadObj(&JsonActivity.Period.Obj,  SchTbl->JsonBuf, SchTbl->JsonFileLen)) AttributeCnt++;
               if (CJSON_LoadObj(&JsonActivity.Offset.Obj,  SchTbl->JsonBuf, SchTbl->JsonFileLen)) AttributeCnt++;
               if (CJSON_LoadObj(&JsonActivity.MsgIdx.Obj,  SchTbl->JsonBuf, SchTbl->JsonFileLen)) AttributeCnt++;
               
               if (AttributeCnt == 4)
               {
                  SchEntry.Enabled     = (strcmp(JsonActivity.Enabled.Value,"true")==0);
                  SchEntry.Period      = JsonActivity.Period.Value;
                  SchEntry.Offset      = JsonActivity.Offset.Value;
                  SchEntry.MsgTblIndex = JsonActivity.MsgIdx.Value;

                  if ((RetStatus = SCHTBL_GetEntryIndex("Scheduler table load rejected", SlotIdx, ActivityIdx, &EntryIdx)))
                  {
                     TblData.Entry[EntryIdx] = SchEntry;
                     EntryUdateCnt++;
                  }
               }
               else
               {
                  RetStatus = false;
                  CFE_EVS_SendEvent(SCHTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                                    "Slot[%d] Activity[%d] has missing attributes, only %d of 4 defined",
                                    SlotArrayIdx, ActivityArrayIdx, AttributeCnt);
               }
               
               ActivityArrayIdx++;
               
            } /* End if Activity found */
            else
            {
               ReadActivity = false;
            }
            if (RetStatus == false)
            {
               ReadSlot     = false;
               ReadActivity = false;
            }
            
         } /* End while read activity */
      
         SlotArrayIdx++;
      
      } /* End if Slot found */
      else
      {
         ReadSlot = false;
      }         
      
   } /* End while read slot */

   if (RetStatus == true)
   {
      memcpy(&SchTbl->Data,&TblData, sizeof(SCHTBL_Data_t));
      SchTbl->LastLoadCnt = EntryUdateCnt;
      CFE_EVS_SendEvent(SCHTBL_LOAD_EID, CFE_EVS_EventType_INFORMATION,
                        "Scheduler Table load updated %d entries", EntryUdateCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */
