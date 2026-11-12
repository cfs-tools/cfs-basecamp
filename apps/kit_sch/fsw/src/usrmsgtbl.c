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
**    Implement KIT_SCH's User Message Table management functions
**
**  Notes:
**    None
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "cfe_endian.h"
#include "usrmsgtbl.h"
#include "cfe_msgids.h"  /* Used for debug */

/***********************/
/** Macro Definitions **/
/***********************/

#define JSON_DATA_WORD_STR_MAX 32

#define CMD_MSG_WORD_LEN    ((unsigned int)sizeof(CFE_MSG_CommandHeader_t)/2


/**********************/
/** Type Definitions **/
/**********************/

/* See LoadJsonData()prologue for details */
typedef CJSON_IntObj_t JsonId_t;
typedef CJSON_IntObj_t JsonTopicId_t;
typedef CJSON_IntObj_t JsonPeriodId_t;
typedef struct
{
   CJSON_Obj_t  Obj;
   char         Value[JSON_DATA_WORD_STR_MAX];
} JsonDataWords_t;


typedef struct
{
   JsonId_t         Id;
   JsonTopicId_t    TopicId;
   JsonPeriodId_t   PeriodId;
     
} JsonMessage_t;


/************************************/
/** Local File Function Prototypes **/
/************************************/

static void ConstructJsonMessage(JsonMessage_t *JsonMessage, uint16 MsgArrayIdx);
static bool LoadJsonData(size_t JsonFileLen);
static bool WriteMsgArrayEntry(osal_id_t FileHandle, uint16 Entry, bool *CompletePrevEntry);

/**********************/
/** Global File Data **/
/**********************/

static USRMSGTBL_Class_t  *UsrMsgTbl = NULL;
static USRMSGTBL_Data_t   TblData;        /* Working buffer for loads */


/******************************************************************************
** Function: USRMSGTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void USRMSGTBL_Constructor(USRMSGTBL_Class_t *ObjPtr, const INITBL_Class_t *IniTbl)
{
   
   UsrMsgTbl = ObjPtr;

   CFE_PSP_MemSet(UsrMsgTbl, 0, sizeof(USRMSGTBL_Class_t));
   
   UsrMsgTbl->InitDebugEvtType    = INITBL_GetIntConfig(IniTbl, CFG_INIT_DEBUG_EVT_TYPE);
   UsrMsgTbl->LowFreqDebugEvtType = INITBL_GetIntConfig(IniTbl, CFG_LOW_FREQ_DEBUG_EVT_TYPE);
   
} /* End USRMSGTBL_Constructor() */


/******************************************************************************
** Function: USRMSGTBL_DumpCmd
**
** Notes:
**   1. Function signature must match TBLMGR_DumpTblFuncPtr.
**   2. Not all load file entries are saved (e.g. "topic-id-map") however all
**      required fields are saved and formatted so the dump file can be used
**      as a load file. Assumes the Topic IDs haven't changed between when the 
**      dump file was created and used as a load file.
**  
*/
bool USRMSGTBL_DumpCmd(osal_id_t FileHandle)
{
   
   bool    WroteEntry;
   bool    CompletePrevEntry;
   uint16  Entry;
   char    DumpRecord[256];


   sprintf(DumpRecord,"\"message-array\": [\n");
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

   WroteEntry = false;
   CompletePrevEntry = false;
   for (Entry=0; Entry < USRMSGTBL_MAX_ENTRIES; Entry++)
   {      
      WroteEntry = WriteMsgArrayEntry(FileHandle, Entry, &CompletePrevEntry);
      
      if (WroteEntry) CompletePrevEntry = true;
      
   } /* End message loop */
   
   /* Close message-array. Top-level object closed by TBLMGR */      
   sprintf(DumpRecord,"\n]");
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

   return true;
   
} /* End of USRMSGTBL_DumpCmd() */

      
/******************************************************************************
** Function: USRMSGTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
*/
bool USRMSGTBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, UsrMsgTbl->Json.Buf, USRMSGTBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      UsrMsgTbl->Json.Loaded = true;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End USRMSGTBL_LoadCmd() */


/******************************************************************************
** Function: USRMSGTBL_ResetStatus
**
*/
void USRMSGTBL_ResetStatus(void)
{
   
   UsrMsgTbl->Json.LastLoadCnt = 0;
    
} /* End USRMSGTBL_ResetStatus() */


/******************************************************************************
** Function: ConstructJsonMessage
**
*/
static void ConstructJsonMessage(JsonMessage_t *JsonMessage, uint16 MsgArrayIdx)
{

   char KeyStr[64];

   sprintf(KeyStr,"message-array[%d].message.id", MsgArrayIdx);
   CJSON_ObjConstructor(&JsonMessage->Id.Obj, KeyStr, JSONNumber, &JsonMessage->Id.Value, 4);

   sprintf(KeyStr,"message-array[%d].message.topic-id", MsgArrayIdx);
   CJSON_ObjConstructor(&JsonMessage->TopicId.Obj, KeyStr, JSONNumber, &JsonMessage->TopicId.Value, 4);

   sprintf(KeyStr,"message-array[%d].message.period-id", MsgArrayIdx);
   CJSON_ObjConstructor(&JsonMessage->PeriodId.Obj, KeyStr, JSONNumber, &JsonMessage->PeriodId.Value, 4);

} /* ConstructJsonMessage() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. The JSON file can contain 1 to USRMSGTBL_MAX_ENTRIES entries. The table can
**     be sparsely populated. The Scheduler Table uses indices into the Message
**     Table and it's the developer's responsibility to make sure they are
**     defined correctly.  
**  2. JSON message object
**
**        "name":  Not saved,
**        "descr": Not saved,
**        "id": 0,
**        "topic-id": 0,
**        "period-id": 0
**
*/
static bool LoadJsonData(size_t JsonFileLen)
{

   bool    RetStatus = true;
   bool    ReadMsg = true;
   uint16  AttributeCnt;
   uint16  MsgArrayIdx;
   
   JsonMessage_t       JsonMessage;
   USRMSGTBL_Entry_t   MsgEntry;
   USRMSGTBL_CmdMsg_t  *CmdMsg;

   UsrMsgTbl->Json.FileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &UsrMsgTbl->Data, sizeof(USRMSGTBL_Data_t));

   MsgArrayIdx = 0;
   while (ReadMsg)
   {

      ConstructJsonMessage(&JsonMessage, MsgArrayIdx);
      memset((void*)&MsgEntry,0,sizeof(USRMSGTBL_Entry_t));

      /*
      ** Use 'id' field to determine whether processing the file
      ** is complete. A missing or malformed 'id' field error will
      ** not be caught or reported.
      ** The 'id' field is required but CJSON_LoadObjOptional() is
      ** used so the 'object not found' event will be suppressed 
      */      
      
      if (CJSON_LoadObjOptional(&JsonMessage.Id.Obj, UsrMsgTbl->Json.Buf, UsrMsgTbl->Json.FileLen))
      {
         if (JsonMessage.Id.Value < USRMSGTBL_MAX_ENTRIES)
         {
            
            AttributeCnt = 0;
            if (CJSON_LoadObj(&JsonMessage.TopicId.Obj,  UsrMsgTbl->Json.Buf, UsrMsgTbl->Json.FileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonMessage.PeriodId.Obj, UsrMsgTbl->Json.Buf, UsrMsgTbl->Json.FileLen)) AttributeCnt++;
            
            if (AttributeCnt == 2)
            {
               
               MsgEntry.TopicId  = (uint16)JsonMessage.TopicId.Value;
               MsgEntry.PeriodId = (uint16)JsonMessage.PeriodId.Value;
               
               CFE_EVS_SendEvent(EdsParam_APP_C_FW_APP_LOW_FREQ_DEBUG_EID, UsrMsgTbl->LowFreqDebugEvtType,
                                 "MsgEntry: TopicIds=%d, PeriodId=%d", MsgEntry.TopicId, MsgEntry.PeriodId);

               memcpy(&TblData.Entry[JsonMessage.Id.Value],&MsgEntry,sizeof(USRMSGTBL_Entry_t));

               // Create command message
               CmdMsg = &UsrMsgTbl->Cmd.Msg[JsonMessage.Id.Value];
               CFE_MSG_Init(CFE_MSG_PTR(CmdMsg->Header),  CFE_SB_ValueToMsgId((uint16)JsonMessage.TopicId.Value), sizeof(CmdMsg->Header));


            } /* End if valid attributes */
            else
            {
               CFE_EVS_SendEvent(USRMSGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "Message[%d] only has %d attributes. topic-id, seq-seg, or length is missing",
                                 MsgArrayIdx, AttributeCnt);
               ReadMsg = false;
               RetStatus = false;
            }
         } /* End if valid ID */
         else
         {
            RetStatus = false;
            CFE_EVS_SendEvent(USRMSGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Message[%d] has an invalid ID value of %d. Valid ID range is 0 to %d",
                              MsgArrayIdx, JsonMessage.Id.Value, (USRMSGTBL_MAX_ENTRIES-1));
         }
         
         MsgArrayIdx++;
         
      } /* End if 'id' */
      else
      {
         ReadMsg = false;
      }
      
   } /* End ReadMsg */
   
   if (MsgArrayIdx == 0)
   {
      RetStatus = false;
      CFE_EVS_SendEvent(USRMSGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                        "JSON table file has no message entries");
   }
   else
   {
      if (RetStatus == true)
      {
         memcpy(&UsrMsgTbl->Data,&TblData, sizeof(USRMSGTBL_Data_t));
         UsrMsgTbl->Json.LastLoadCnt = MsgArrayIdx;
         CFE_EVS_SendEvent(USRMSGTBL_LOAD_EID, CFE_EVS_EventType_INFORMATION,
                           "Message Table load updated %d entries", MsgArrayIdx);
      }
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */


/******************************************************************************
** Function: WriteMsgArrayEntry
**
*/
static bool WriteMsgArrayEntry(osal_id_t FileHandle, uint16 Entry, bool *CompletePrevEntry)
{
   bool    WroteEntry = false;
   char    DumpRecord[256];
   uint16  TopicId;
   const USRMSGTBL_Data_t  *MsgTblData = &UsrMsgTbl->Data;


   TopicId = MsgTblData->Entry[Entry].TopicId;
   if ( TopicId != 0 && TopicId != PKTUTIL_UNUSED_MSG_ID)
   {
      // Complete a previouly written entry
      if (*CompletePrevEntry)
      { 
         sprintf(DumpRecord,",\n");
         OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
         *CompletePrevEntry = false;
      }

      /* 
      ** Message Array 
      **
      ** - Not all fields in ground table are saved in FSW so they are not
      **   populated in the dump file. However, the dump file can still
      **   be loaded."topic-id-map"
      **
      **   "message-array": [
      **      {"message": {
      **         "id": 0,
      **         "topic-id": 0,
      **         "period-id": 5
      **      }},
      **   ]
      */
      
      sprintf(DumpRecord,"   {\"message\": {\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
         
      sprintf(DumpRecord,"      \"id\": %d,\n      \"topic-id\": %d,\n      \"period-id\": %d\n   }}",
              Entry, TopicId, MsgTblData->Entry[Entry].PeriodId);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      WroteEntry = true;

   } /* End if valid Topic ID */

   return WroteEntry;
   
} /* End WriteMsgArrayEntry() */