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
**    Implement KIT_SCH's Message Table management functions
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
#include "msgtbl.h"
#include "cfe_msgids.h"  /* Used for debug */

/***********************/
/** Macro Definitions **/
/***********************/

#define JSON_DATA_WORD_STR_MAX 32

/**********************/
/** Type Definitions **/
/**********************/

/* See LoadJsonData()prologue for details */
typedef CJSON_IntObj_t JsonId_t;
typedef CJSON_IntObj_t JsonTopicId_t;
typedef CJSON_IntObj_t JsonSeqSeg_t;
typedef CJSON_IntObj_t JsonLength_t;
typedef struct
{
   CJSON_Obj_t  Obj;
   char         Value[JSON_DATA_WORD_STR_MAX];
} JsonDataWords_t;


typedef struct
{
   JsonId_t         Id;
   JsonTopicId_t    TopicId;
   JsonSeqSeg_t     SeqSeg;
   JsonLength_t     Length;
   JsonDataWords_t  DataWords;
     
} JsonMessage_t;


/************************************/
/** Local File Function Prototypes **/
/************************************/

static void ConstructJsonMessage(JsonMessage_t *JsonMessage, uint16 MsgArrayIdx);
static bool LoadJsonData(size_t JsonFileLen);

/**********************/
/** Global File Data **/
/**********************/

static MSGTBL_Class_t  *MsgTbl = NULL;
static MSGTBL_Data_t   TblData;        /* Working buffer for loads */


/******************************************************************************
** Function: MSGTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void MSGTBL_Constructor(MSGTBL_Class_t *ObjPtr)
{
   
   MsgTbl = ObjPtr;

   CFE_PSP_MemSet(MsgTbl, 0, sizeof(MSGTBL_Class_t));

   CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                    "MSGTBL_MAX_MSG_WORDS: %d, sizeof(CFE_MSG_CommandHeader_t): %d, sizeof(MSGTBL_Cmd_t): %d",
                    MSGTBL_MAX_MSG_WORDS, (unsigned int)sizeof(CFE_MSG_CommandHeader_t), (unsigned int)sizeof(MSGTBL_CmdMsg_t));    
   CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                    "CFE_ES_SEND_HK_MID: 0x%04X (%d)", CFE_ES_SEND_HK_MID, CFE_ES_SEND_HK_MID);
   CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                    "CFE_EVS_SEND_HK_MID: 0x%04X (%d)", CFE_EVS_SEND_HK_MID, CFE_EVS_SEND_HK_MID);
   CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                    "CFE_SB_SEND_HK_MID: 0x%04X (%d)", CFE_SB_SEND_HK_MID, CFE_SB_SEND_HK_MID);
   CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                    "CFE_TBL_SEND_HK_MID: 0x%04X (%d)", CFE_TBL_SEND_HK_MID, CFE_TBL_SEND_HK_MID);
   CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                    "CFE_TIME_SEND_HK_MID: 0x%04X (%d)", CFE_TIME_SEND_HK_MID, CFE_TIME_SEND_HK_MID);
   

} /* End MSGTBL_Constructor() */


/******************************************************************************
** Function: MSGTBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr.
**  2. File is formatted so it can be used as a load file. 
*/

bool MSGTBL_DumpCmd(osal_id_t FileHandle)
{

   int32   i, d;
   char    DumpRecord[256];
   uint16  DataWords;
   CFE_MSG_Size_t      MsgBytes;
   const MSGTBL_Data_t *MsgTblPtr = &MsgTbl->Data;
   

   /* 
   ** Message Array 
   **
   ** - Not all fields in ground table are saved in FSW so they are not
   **   populated in the dump file. However, the dump file can still
   **   be loaded.
   **
   **   "name":  Not loaded,
   **   "descr": Not Loaded,
   **   "id": 101,
   **   "topic-id": 65303,
   **   "seq-seg": 192,
   **   "length": 1792,
   **   "data-words": "0,1,2,3,4,5"
   */
   
   sprintf(DumpRecord,"\"message-array\": [\n");
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

   for (i=0; i < MSGTBL_MAX_ENTRIES; i++)
   {
      
      if (i > 0)  /* Complete previous entry */
      { 
         sprintf(DumpRecord,",\n");
         OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      }
       
      sprintf(DumpRecord,"   {\"message\": {\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      sprintf(DumpRecord,"      \"id\": %d,\n      \"topic-id\": %d,\n      \"seq-seg\": %d,\n      \"length\": %d",
              i,
              CFE_MAKE_BIG16(MsgTblPtr->Entry[i].Buffer[0]),
              CFE_MAKE_BIG16(MsgTblPtr->Entry[i].Buffer[1]),
              CFE_MAKE_BIG16(MsgTblPtr->Entry[i].Buffer[2]));
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      /*
      ** DataWords is everything past the primary header so they include
      ** the secondary header and don't distinguish between cmd or tlm
      ** packets. 
      */
      
      // TODO - cFE7.0 Rethink table itself. Either only allow commands or use message type to get header length and data words will truly just be payload
      
      CFE_MSG_GetSize((const CFE_MSG_Message_t *)MsgTblPtr->Entry[i].Buffer, &MsgBytes);

      DataWords = (MsgBytes-PKTUTIL_CMD_HDR_BYTES)/2;
      if (DataWords > (uint8)(MSGTBL_MAX_MSG_WORDS))
      {
         
         CFE_EVS_SendEvent(MSGTBL_DUMP_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Error creating dump file message entry %d. Message word length %d is greater than max data buffer %d",
                           i, DataWords, (unsigned int)PKTUTIL_CMD_HDR_WORDS);         
      }
      else
      {

         /* 
         ** Omit "data-words" property if no data
         ** - Properly terminate 'length' line 
         */
         if (DataWords > 0)
         {
      
            sprintf(DumpRecord,",\n      \"data-words\": \"");         
            OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
               
            for (d=0; d < DataWords; d++)
            {
               
               if (d == (DataWords-1))
               {
                  sprintf(DumpRecord,"%d\"\n   }}",MsgTbl->Data.Entry[i].Buffer[PKTUTIL_CMD_HDR_WORDS+d]);
               }
               else
               {
                  sprintf(DumpRecord,"%d,",MsgTbl->Data.Entry[i].Buffer[PKTUTIL_CMD_HDR_WORDS+d]);
               }
               OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

            } /* End DataWord loop */
                        
         } /* End if non-zero data words */
         else
         {
            sprintf(DumpRecord,"\n   }}");         
            OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
         }
      } /* End if DataWords within range */

   } /* End message loop */

   /* Close message-array and top-level object */      
   sprintf(DumpRecord,"\n]");
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

   return true;
   
} /* End of MSGTBL_DumpCmd() */


/******************************************************************************
** Function: MSGTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
*/
bool MSGTBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, MsgTbl->JsonBuf, MSGTBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      MsgTbl->Loaded = true;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End MSGTBL_LoadCmd() */


/******************************************************************************
** Function: MSGTBL_ResetStatus
**
*/
void MSGTBL_ResetStatus(void)
{
   
   MsgTbl->LastLoadCnt = 0;
    
} /* End MSGTBL_ResetStatus() */


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

   sprintf(KeyStr,"message-array[%d].message.seq-seg", MsgArrayIdx);
   CJSON_ObjConstructor(&JsonMessage->SeqSeg.Obj, KeyStr, JSONNumber, &JsonMessage->SeqSeg.Value, 4);

   sprintf(KeyStr,"message-array[%d].message.length", MsgArrayIdx);
   CJSON_ObjConstructor(&JsonMessage->Length.Obj, KeyStr, JSONNumber, &JsonMessage->Length.Value, 4);

   sprintf(KeyStr,"message-array[%d].message.data-words", MsgArrayIdx);
   CJSON_ObjConstructor(&JsonMessage->DataWords.Obj, KeyStr, JSONString, JsonMessage->DataWords.Value, JSON_DATA_WORD_STR_MAX);

} /* ConstructJsonMessage() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. The JSON file can contain 1 to MSGTBL_MAX_ENTRIES entries. The table can
**     be sparsely populated. The Scheduler Table uses indices into the Message
**     Table and it's the developer's responsibility to make sure they are
**     defined correctly.  
**  2. JSON message object
**
**        "name":  Not saved,
**        "descr": Not saved,
**        "id": 101,
**        "topic-id": 6209,
**        "seq-seg": 49152,
**        "length": 1,
**        "data-words": "1,2,3,4"  # Optional field
**
**  3. The maximum number of data words is a platform configuration. 
**  4. The current design does not support loading secondary header fields. If
**     it's added, separate JSON variables should be added that correspond to 
**     the CFE_MSG APIs rather than loaded buffers and making assumptions about
**     structure defintiions.  
**        
*/
static bool LoadJsonData(size_t JsonFileLen)
{

   bool    RetStatus = true;
   bool    ReadMsg = true;
   uint16  i;
   uint16  AttributeCnt;
   uint16  MsgArrayIdx;
   char    *DataStrPtr;
   uint16  PayloadDataIdx = sizeof(CFE_MSG_CommandHeader_t)/2;

   JsonMessage_t   JsonMessage;
   MSGTBL_Entry_t  MsgEntry;
   MSGTBL_CmdMsg_t *CmdMsg;

   MsgTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &MsgTbl->Data, sizeof(MSGTBL_Data_t));

   MsgArrayIdx = 0;
   while (ReadMsg)
   {

      ConstructJsonMessage(&JsonMessage, MsgArrayIdx);
      memset((void*)&MsgEntry,0,sizeof(MSGTBL_Entry_t));

      /*
      ** Use 'id' field to determine whether processing the file
      ** is complete. A missing or malformed 'id' field error will
      ** not be caught or reported.
      ** The 'id' field is required but CJSON_LoadObjOptional() is
      ** used so the 'object not found' event will be suppressed 
      */      
      
      if (CJSON_LoadObjOptional(&JsonMessage.Id.Obj, MsgTbl->JsonBuf, MsgTbl->JsonFileLen))
      {
         if (JsonMessage.Id.Value < MSGTBL_MAX_ENTRIES)
         {
            
            AttributeCnt = 0;
            if (CJSON_LoadObj(&JsonMessage.TopicId.Obj,  MsgTbl->JsonBuf, MsgTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonMessage.SeqSeg.Obj,   MsgTbl->JsonBuf, MsgTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonMessage.Length.Obj,   MsgTbl->JsonBuf, MsgTbl->JsonFileLen)) AttributeCnt++;
            
            if (AttributeCnt == 3)
            {
               /* TODO - This is not 'on the wire' so native works
               MsgEntry.Buffer[0] = CFE_MAKE_BIG16((uint16)JsonMessage.TopicId.Value);
               MsgEntry.Buffer[1] = CFE_MAKE_BIG16((uint16)JsonMessage.SeqSeg.Value);
               MsgEntry.Buffer[2] = CFE_MAKE_BIG16((uint16)JsonMessage.Length.Value);
               */
               MsgEntry.Buffer[0] = (uint16)JsonMessage.TopicId.Value;
               MsgEntry.Buffer[1] = (uint16)JsonMessage.SeqSeg.Value;
               MsgEntry.Buffer[2] = (uint16)JsonMessage.Length.Value;

               CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                                 "MsgEntry.Buffer: [0]=%d, [1]=%d, [2]=%d", 
                                 MsgEntry.Buffer[0], MsgEntry.Buffer[1], MsgEntry.Buffer[2]);

               if (CJSON_LoadObjOptional(&JsonMessage.DataWords.Obj, MsgTbl->JsonBuf, MsgTbl->JsonFileLen))
               {
                  if (strlen(JsonMessage.DataWords.Value) > 0)
                  {
                     i = PayloadDataIdx;
                     /* No protection against malformed data array */
                     DataStrPtr = strtok(JsonMessage.DataWords.Value,",");
                     if (DataStrPtr != NULL)
                     {
                        MsgEntry.Buffer[i] = atoi(DataStrPtr);
                        CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                                          "MSGTBL::LoadJsonData data[%d] = 0x%04X, DataStrPtr=%s",i,MsgEntry.Buffer[i],DataStrPtr);         
                        while ((DataStrPtr = strtok(NULL,",")) != NULL)
                        {
                           i++;
                           MsgEntry.Buffer[i] = atoi(DataStrPtr);
                           CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                                             "MSGTBL::LoadJsonData data[%d] = 0x%04X, DataStrPtr=%s",i,MsgEntry.Buffer[i],DataStrPtr);
                           if (i >= MSGTBL_MAX_MSG_WORDS)
                           {
                              CFE_EVS_SendEvent(MSGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                                                "Message Topic %d: Number of data words exceed platform configuration %d",
                                                MsgEntry.Buffer[0], MSGTBL_MAX_MSG_PAYLOAD_WORDS);
                              break;
                           }
                        } /* While data word */
                     }
                     MsgEntry.PayloadWordCnt = i - PayloadDataIdx + 1;
                  } /* End if strlen > 0 */
               } /* End if DataWords */
         
               memcpy(&TblData.Entry[JsonMessage.Id.Value],&MsgEntry,sizeof(MSGTBL_Entry_t));
               CmdMsg = &MsgTbl->Cmd.Msg[JsonMessage.Id.Value];
               CFE_MSG_Init(CFE_MSG_PTR(CmdMsg->Header),  CFE_SB_ValueToMsgId(MsgEntry.Buffer[0]), sizeof(CmdMsg->Header)+MsgEntry.PayloadWordCnt*2);
               if (MsgEntry.PayloadWordCnt > 0)
               {
                  memcpy(&CmdMsg->Payload,&MsgEntry.Buffer[PayloadDataIdx],MsgEntry.PayloadWordCnt*2);
                  /*
                  CFE_EVS_SendEvent(KIT_SCH_INIT_DEBUG_EID, KIT_SCH_INIT_EVS_TYPE,
                                    "MsgEntry.PayloadWordCnt = %d",i-1, MsgEntry.PayloadWordCnt);
                  int WordBufLen = (sizeof(CmdMsg->Header)/2+MsgEntry.PayloadWordCnt);
                  OS_printf("\n*** MsgEntry.PayloadWordCnt = %d, WordBufLen = %d\n", MsgEntry.PayloadWordCnt, WordBufLen);                  
                  OS_printf("sizeof(CmdMsg->Header) = %ld\n", sizeof(CmdMsg->Header)); 
                  for (i=0; i < WordBufLen; i++)
                  {
                      OS_printf("MsgEntry.Buffer[%d] = 0x%02X\n", i, MsgEntry.Buffer[i]);
                  }
                  */
               }                
            } /* End if valid attributes */
            else
            {
               CFE_EVS_SendEvent(MSGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "Message[%d] only has %d attributes. topic-id, seq-seg, or length is missing",
                                 MsgArrayIdx, AttributeCnt);
               ReadMsg = false;
               RetStatus = false;
            }
         } /* End if valid ID */
         else
         {
            RetStatus = false;
            CFE_EVS_SendEvent(MSGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Message[%d] has an invalid ID value of %d. Valid ID range is 0 to %d",
                              MsgArrayIdx, JsonMessage.Id.Value, (MSGTBL_MAX_ENTRIES-1));
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
      CFE_EVS_SendEvent(MSGTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                        "JSON table file has no message entries");
   }
   else
   {
      if (RetStatus == true)
      {
         memcpy(&MsgTbl->Data,&TblData, sizeof(MSGTBL_Data_t));
         MsgTbl->LastLoadCnt = MsgArrayIdx;
         CFE_EVS_SendEvent(MSGTBL_LOAD_EID, CFE_EVS_EventType_INFORMATION,
                           "Message Table load updated %d entries", MsgArrayIdx);
      }
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */
