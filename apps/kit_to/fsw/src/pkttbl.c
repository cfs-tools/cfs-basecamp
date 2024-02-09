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
**    Implement KIT_TO's Packet Table management functions
**
**  Notes:
**    None
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "pkttbl.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define JSON_DATA_WORD_STR_MAX 32


/**********************/
/** Type Definitions **/
/**********************/

/* See LoadJsonData() prologue for details */

typedef CJSON_IntObj_t JsonTopicId_t;
typedef CJSON_StrObj_t JsonForward_t;
typedef CJSON_IntObj_t JsonPriority_t;
typedef CJSON_IntObj_t JsonReliability_t;
typedef CJSON_IntObj_t JsonBufLimit_t;
typedef CJSON_IntObj_t JsonFilterType_t;
typedef CJSON_IntObj_t JsonFilterX_t;
typedef CJSON_IntObj_t JsonFilterN_t;
typedef CJSON_IntObj_t JsonFilterO_t;

typedef struct
{
   JsonTopicId_t      TopicId;
   JsonForward_t      Forward;
   JsonPriority_t     Priority;
   JsonReliability_t  Reliability;
   JsonBufLimit_t     BufLimit;
   JsonFilterType_t   FilterType;
   JsonFilterX_t      FilterX;
   JsonFilterN_t      FilterN;
   JsonFilterO_t      FilterO;
     
} JsonPacket_t;

/**********************/
/** Global File Data **/
/**********************/

static PKTTBL_Class_t  *PktTbl = NULL;
static PKTTBL_Data_t   TblData;        /* Working buffer for loads */


/******************************/
/** File Function Prototypes **/
/******************************/

static void ConstructJsonPacket(JsonPacket_t *JsonPacket, uint16 PktArrayIdx);
static bool LoadJsonData(size_t JsonFileLen);
static bool WriteJsonPkt(int32 FileHandle, const PKTTBL_Pkt_t* Pkt, bool FirstPktWritten);


/******************************************************************************
** Function: PKTTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void PKTTBL_Constructor(PKTTBL_Class_t *ObjPtr, const char *AppName,
                        PKTTBL_LoadNewTbl_t LoadNewTbl)
{
   
   PktTbl = ObjPtr;

   CFE_PSP_MemSet(PktTbl, 0, sizeof(PKTTBL_Class_t));
   PKTTBL_SetTblToUnused(&(PktTbl->Data));

   PktTbl->AppName        = AppName;
   PktTbl->LoadNewTbl     = LoadNewTbl;
   PktTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;
   
} /* End PKTTBL_Constructor() */


/******************************************************************************
** Function: PKTTBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
**  3. DumpType is unused.
**  4. File is formatted so it can be used as a load file. It does not follow
**     the cFE table file format. 
**  5. Creates a new dump file, overwriting anything that may have existed
**     previously
*/

bool PKTTBL_DumpCmd(TBLMGR_Tbl_t *Tbl, uint8 DumpType, const char *Filename)
{

   bool          RetStatus = false;
   osal_id_t     FileHandle;
   int32         OsStatus;
   bool          FirstPktWritten = false;
   uint16        AppId;
   char          DumpRecord[512];
   char          SysTimeStr[256];
   os_err_name_t OsErrStr;

   OsStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_READ_WRITE);
   
   if (OsStatus == OS_SUCCESS)
   {

      sprintf(DumpRecord,"\n{\n\"name\": \"Kit Telemetry Output (KIT_TO) Packet Table\",\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      CFE_TIME_Print(SysTimeStr, CFE_TIME_GetTime());
      
      sprintf(DumpRecord,"\"description\": \"KIT_TO dumped at %s\",\n",SysTimeStr);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));


      /* 
      ** Packet Array 
      **
      ** - Not all fields in ground table are saved in FSW so they are not
      **   populated in the dump file. However, the dump file can still
      **   be loaded.
      */
      
      sprintf(DumpRecord,"\"packet-array\": [\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++)
      {
               
         if (WriteJsonPkt(FileHandle, &(PktTbl->Data.Pkt[AppId]), FirstPktWritten)) FirstPktWritten = true;
              
      } /* End packet loop */

      sprintf(DumpRecord,"\n]}\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      RetStatus = true;

      OS_close(FileHandle);

   } /* End if file create */
   else
   {
      OS_GetErrorName(OsStatus, &OsErrStr);
      CFE_EVS_SendEvent(PKTTBL_CREATE_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error creating dump file '%s', Status = %s", 
                        Filename, OsErrStr);
   
   } /* End if file create error */

   return RetStatus;
   
} /* End of PKTTBL_DumpCmd() */


/******************************************************************************
** Function: PKTTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
*/
bool PKTTBL_LoadCmd(TBLMGR_Tbl_t *Tbl, uint8 LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, PktTbl->JsonBuf, PKTTBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      PktTbl->Loaded = true;
      PktTbl->LastLoadStatus = TBLMGR_STATUS_VALID;
      RetStatus = true;
   }
   else
   {
      PktTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;
   }

   return RetStatus;
   
} /* End PKTTBL_LoadCmd() */


/******************************************************************************
** Function: PKTTBL_ResetStatus
**
*/
void PKTTBL_ResetStatus(void)
{
   
   PktTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;
   PktTbl->LastLoadCnt = 0;
    
} /* End PKTTBL_ResetStatus() */


/******************************************************************************
** Function: PKTTBL_SetPacketToUnused
**
**
*/
void PKTTBL_SetPacketToUnused(PKTTBL_Pkt_t *PktPtr)
{
   
   CFE_PSP_MemSet(PktPtr, 0, sizeof(PKTTBL_Pkt_t));

   PktPtr->MsgId       = PKTTBL_UNUSED_MSG_ID;
   PktPtr->Filter.Type = PKTUTIL_FILTER_ALWAYS;
   
} /* End PKTTBL_SetPacketToUnused() */


/******************************************************************************
** Function: PKTTBL_SetTblToUnused
**
**
*/
void PKTTBL_SetTblToUnused(PKTTBL_Data_t *TblPtr)
{
  
   uint16 AppId;
   
   CFE_PSP_MemSet(TblPtr, 0, sizeof(PKTTBL_Data_t));

   for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++)
   {
      
      TblPtr->Pkt[AppId].MsgId       = PKTTBL_UNUSED_MSG_ID;
      TblPtr->Pkt[AppId].Filter.Type = PKTUTIL_FILTER_ALWAYS;
   
   }
   
} /* End PKTTBL_SetTblToUnused() */


/******************************************************************************
** Function: ConstructJsonPacket
**
*/
static void ConstructJsonPacket(JsonPacket_t* JsonPacket, uint16 PktArrayIdx)
{

   char KeyStr[64];
   
   sprintf(KeyStr,"packet-array[%d].packet.topic-id", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->TopicId.Obj, KeyStr, JSONNumber, &JsonPacket->TopicId.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.forward", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->Forward.Obj, KeyStr, JSONNumber, &JsonPacket->Forward.Value, 5); // Max length is 'false'

   sprintf(KeyStr,"packet-array[%d].packet.priority", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->Priority.Obj, KeyStr, JSONNumber, &JsonPacket->Priority.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.reliability", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->Reliability.Obj, KeyStr, JSONNumber, &JsonPacket->Reliability.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.buf-limit", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->BufLimit.Obj, KeyStr, JSONNumber, &JsonPacket->BufLimit.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.type", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterType.Obj, KeyStr, JSONNumber, &JsonPacket->FilterType.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.X", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterX.Obj, KeyStr, JSONNumber, &JsonPacket->FilterX.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.N", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterN.Obj, KeyStr, JSONNumber, &JsonPacket->FilterN.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.O", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterO.Obj, KeyStr, JSONNumber, &JsonPacket->FilterO.Value, 4);

} /* ConstructJsonPacket() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. The JSON file can contain 1 to PKTUTIL_MAX_APP_ID entries. The table can
**     be sparsely populated. 
**  2. JSON "packet-array" contains the following "packet" object entries
**       {"packet": {
**          "name": "ES_APP_TLM_TOPICID",  # Not saved
**          "topic-id-N": 0,               # Not saved
**          "topic-id": 0,
**          "forward": false,
**          "priority": 0,
**          "reliability": 0,
**          "buf-limit": 4,
**          "filter": { "type": 2, "X": 1, "N": 1, "O": 0}
**       }},
**
*/
static bool LoadJsonData(size_t JsonFileLen)
{

   bool    RetStatus = true;
   bool    ReadPkt = true;
   uint16  AttributeCnt;
   uint16  PktArrayIdx;
   uint16  AppIdIdx;
   
   JsonPacket_t  JsonPacket;
   PKTTBL_Pkt_t  Pkt;


   PktTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &PktTbl->Data, sizeof(PKTTBL_Data_t));

   PktArrayIdx = 0;
   while (ReadPkt)
   {

      ConstructJsonPacket(&JsonPacket, PktArrayIdx);
      memset((void*)&Pkt,0,sizeof(PKTTBL_Pkt_t));

      /*
      ** Use 'topic-id' field to determine whether processing the file
      ** is complete. A missing or malformed 'topic-id' field error will
      ** not be caught or reported.
      ** The 'topic-id' field is required but CJSON_LoadObjOptional() is
      ** used so the 'object not found' event will be suppressed 
      */      
      
      if (CJSON_LoadObjOptional(&JsonPacket.TopicId.Obj, PktTbl->JsonBuf, PktTbl->JsonFileLen))
      {
         
         AppIdIdx = JsonPacket.TopicId.Value & PKTTBL_APP_ID_MASK;
         
         if (AppIdIdx < PKTUTIL_MAX_APP_ID)
         {
         
            AttributeCnt = 0;
            if (CJSON_LoadObj(&JsonPacket.Forward.Obj,     PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.Priority.Obj,    PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.Reliability.Obj, PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.BufLimit.Obj,    PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterType.Obj,  PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterX.Obj,     PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterN.Obj,     PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterO.Obj,     PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            
            if (AttributeCnt == 8)
            {
               
               Pkt.MsgId           = JsonPacket.TopicId.Value;
               Pkt.Forward         = (strcmp(JsonPacket.Forward.Value, "true") == 0);
               Pkt.Qos.Priority    = JsonPacket.Priority.Value;
               Pkt.Qos.Reliability = JsonPacket.Reliability.Value;
               Pkt.BufLim          = JsonPacket.BufLimit.Value;
               Pkt.Filter.Type     = JsonPacket.FilterType.Value;
               Pkt.Filter.Param.X  = JsonPacket.FilterX.Value; 
               Pkt.Filter.Param.N  = JsonPacket.FilterN.Value; 
               Pkt.Filter.Param.O  = JsonPacket.FilterO.Value; 
                              
               memcpy(&TblData.Pkt[AppIdIdx],&Pkt,sizeof(PKTTBL_Pkt_t));
               
            } /* End if valid attributes */
            else
            {
               CFE_EVS_SendEvent(PKTTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "Packet[%d] has missing attributes, only %d of 8 defined",
                                 PktArrayIdx, AttributeCnt);
               ReadPkt = false;
               RetStatus = false;
            }
         } /* End if valid ID */
         else
         {
            CFE_EVS_SendEvent(PKTTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Packet[%d]'s topic-id %d has an invlaid app-id value of %d. Valid range is 0 to %d",
                              PktArrayIdx, JsonPacket.TopicId.Value, AppIdIdx, (PKTUTIL_MAX_APP_ID-1));
         }

         PktArrayIdx++;
         
      } /* End if 'topic-id' */
      else
      {
         ReadPkt = false;
      }
      
   } /* End ReadPkt */
   
   if (PktArrayIdx == 0)
   {
      CFE_EVS_SendEvent(PKTTBL_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
                        "JSON table file has no message entries");
   }
   else
   {
      if (RetStatus == true)
      {
         PktTbl->LoadNewTbl(&TblData);
         PktTbl->LastLoadCnt = PktArrayIdx;
         CFE_EVS_SendEvent(PKTTBL_LOAD_EID, CFE_EVS_EventType_INFORMATION,
                           "Packet Table load updated %d entries", PktArrayIdx);
      }
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */


/******************************************************************************
** Function: WriteJsonPkt
**
** Notes:
**   1. Can't end last record with a comma so logic checks that commas only
**      start to be written after the first packet has been written
*/
static bool WriteJsonPkt(int32 FileHandle, const PKTTBL_Pkt_t* Pkt, bool FirstPktWritten)
{
   
   bool PktWritten = false;
   char DumpRecord[256];

   if (Pkt->MsgId != PKTTBL_UNUSED_MSG_ID)
   {
      
      if (FirstPktWritten)
      {
         sprintf(DumpRecord,",\n");
         OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      }
      
      sprintf(DumpRecord,"\"packet\": {\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"   \"topic-id\": %d,\n   \"forward\": %d,\n   \"priority\": %d,\n   \"reliability\": %d,\n   \"buf-limit\": %d,\n",
              Pkt->MsgId, Pkt->Forward, Pkt->Qos.Priority, Pkt->Qos.Reliability, Pkt->BufLim);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      sprintf(DumpRecord,"   \"filter\": { \"type\": %d, \"X\": %d, \"N\": %d, \"O\": %d}\n}",
              Pkt->Filter.Type, Pkt->Filter.Param.X, Pkt->Filter.Param.N, Pkt->Filter.Param.O);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
   
      PktWritten = true;
      
   } /* End if MsgId record has been loaded */
   
   return PktWritten;
   
} /* End WriteJsonPkt() */


