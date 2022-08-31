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
**    Provides a wrapper for apps to use coreJSON for their tables.
**
**  Notes:
**    None  
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
#include "cjson.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define PRINT_BUF_SEGMENT_BYTES 100   /* Number of bytes in each OS_printf() call */ 


/**********************/
/** Type Definitions **/
/**********************/

typedef enum
{

   OBJ_OPTIONAL,
   OBJ_REQUIRED

} OBJ_Necessity_t;


/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool LoadObj(CJSON_Obj_t* Obj, const char* Buf, size_t BufLen, OBJ_Necessity_t Necessity);

static void PrintJsonBuf(const char* JsonBuf, size_t BufLen);
static bool ProcessFile(const char* Filename, char* JsonBuf, size_t MaxJsonFileChar,
                        CJSON_LoadJsonData_t LoadJsonData,
                        CJSON_LoadJsonDataAlt_t LoadJsonDataAlt, void* UserDataPtr,
                        bool CallbackWithUserData);

static bool StubLoadJsonData(size_t JsonFileLen);
static bool StubLoadJsonDataAlt(size_t JsonFileLen, void* UserDataPtr);


/**********************/
/** Global File Data **/
/**********************/

/* JSONStatus_t - String lookup table */

static const char* JsonStatusStr[] = {
  
  "ValidButPartial",    /* JSONPartial          */
  "Valid",              /* JSONSuccess          */
  "Invalid-Malformed",  /* JSONIllegalDocument  */
  "MaxDepthExceeded",   /* JSONMaxDepthExceeded */
  "QueryKeyNotFound",   /* JSONNotFound         */
  "QueryNullPointer",   /* JSONNullParameter    */
  "QueryKeyInvalid",    /* JSONBadParameter     */
  
};

/* JSONTypes_t -  String lookup table */

static const char* JsonTypeStr[] = {
  
  "Invalid",  /* JSONInvalid */
  "String",   /* JSONString  */
  "Number",   /* JSONNumber  */
  "True",     /* JSONTrue    */
  "False",    /* JSONFalse   */
  "Null",     /* JSONNull    */
  "Object",   /* JSONObject  */
  "Array",    /* JSONArray   */
  
};


/******************************************************************************
** Function: CJSON_FltObjConstructor
**
** Notes:
**    1. Float support was added much later than initial release so in order
**       to preserve the API and since float objects are the exception, a 
**       separate constructor was added.
*/
void CJSON_FltObjConstructor(CJSON_Obj_t *Obj, const char *QueryKey, 
                             JSONTypes_t JsonType, void *TblData, size_t TblDataLen)
{

   CJSON_ObjConstructor(Obj, QueryKey, JsonType, TblData, TblDataLen);
   
   Obj->TypeFlt = true;
         
} /* End CJSON_FltObjConstructor() */


/******************************************************************************
** Function: CJSON_ObjConstructor
**
** Notes:
**    1. This is used to construct individual CJSON_Obj_t structures. This 
**       constructor is not needed if the user creates a static CJSON_Obj_t
**       array with default values.
*/
void CJSON_ObjConstructor(CJSON_Obj_t *Obj, const char *QueryKey, 
                          JSONTypes_t JsonType, void *TblData, size_t TblDataLen)
{

   Obj->Updated    = false;
   Obj->TblData    = TblData;
   Obj->TblDataLen = TblDataLen;   
   Obj->Type       = JsonType;
   Obj->TypeFlt    = false;
   
   if (strlen(QueryKey) <= CJSON_MAX_KEY_LEN)
   {
      
      strncpy (Obj->Query.Key, QueryKey, CJSON_MAX_KEY_LEN);
      Obj->Query.KeyLen = strlen(Obj->Query.Key);
   }
   else
   {
      CFE_EVS_SendEvent(CJSON_OBJ_CONSTRUCT_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error constructing table. Query key %s exceeds maximum key length %d.",
                        QueryKey, CJSON_MAX_KEY_LEN);
   }
      
} /* End CJSON_ObjConstructor() */


/******************************************************************************
** Function: CJSON_LoadObj
**
** Notes:
**    1. See LoadObj()'s switch statement for supported JSON types
**
*/
bool CJSON_LoadObj(CJSON_Obj_t *Obj, const char*Buf, size_t BufLen)
{
   
   return LoadObj(Obj, Buf, BufLen, OBJ_REQUIRED);
   
} /* End CJSON_LoadObj() */


/******************************************************************************
** Function: CJSON_LoadObjArray
**
** Notes:
**    1. See CJSON_LoadObj() for supported JSON types
**
*/
size_t CJSON_LoadObjArray(CJSON_Obj_t *Obj, size_t ObjCnt, const char *Buf, size_t BufLen)
{
   
   int     i;
   size_t  ObjLoadCnt = 0;
   
   for (i=0; i < ObjCnt; i++)
   {
   
      if (CJSON_LoadObj(&Obj[i], Buf, BufLen)) ObjLoadCnt++;
      
   } /* End object loop */
   
   return ObjLoadCnt;
      
} /* End CJSON_LoadObjArray() */


/******************************************************************************
** Function: CJSON_LoadObjOptional
**
** Notes:
**    1. See LoadObj()'s switch statement for supported JSON types
**
*/
bool CJSON_LoadObjOptional(CJSON_Obj_t *Obj, const char *Buf, size_t BufLen)
{
   
   return LoadObj(Obj, Buf, BufLen, OBJ_OPTIONAL);
   
} /* End CJSON_LoadObjOptional() */


/******************************************************************************
** Function: CJSON_ObjTypeStr
**
** Type checking should enforce valid parameter but check just to be safe.
*/
const char* CJSON_ObjTypeStr(JSONTypes_t  ObjType)
{

   uint8 i = 0;
   
   if ( ObjType >= JSONInvalid &&
        ObjType <= JSONArray)
   {
   
      i =  ObjType;
   
   }
        
   return JsonTypeStr[i];

} /* End CJSON_ObjTypeStr() */


/******************************************************************************
** Function: CJSON_ProcessFile
**
** Notes:
**  1. See ProcessFile() for details.
**  2. The JsonBuf pointer is passed in as an unused UserDataPtr. 
*/
bool CJSON_ProcessFile(const char *Filename, char *JsonBuf, 
                       size_t MaxJsonFileChar, CJSON_LoadJsonData_t LoadJsonData)
{


   return ProcessFile(Filename, JsonBuf, MaxJsonFileChar, LoadJsonData, StubLoadJsonDataAlt, (void*)JsonBuf, false);

   
} /* End CJSON_ProcessFile() */


/******************************************************************************
** Function: CJSON_ProcessFileAlt
**
** Notes:
**  1. See ProcessFile() for details.
*/
bool CJSON_ProcessFileAlt(const char *Filename, char *JsonBuf, 
                          size_t MaxJsonFileChar, CJSON_LoadJsonDataAlt_t LoadJsonDataAlt,
                          void *UserDataPtr)
{

   return ProcessFile(Filename, JsonBuf, MaxJsonFileChar, StubLoadJsonData, LoadJsonDataAlt, UserDataPtr, true);

   
} /* End CJSON_ProcessFileAlt() */


/******************************************************************************
** Function: LoadObj
**
** Notes:
**    None
**
*/
static bool LoadObj(CJSON_Obj_t* Obj, const char* Buf, size_t BufLen, OBJ_Necessity_t Necessity)
{
   
   bool         RetStatus = false;
   JSONStatus_t JsonStatus;
   const char   *Value;
   size_t       ValueLen;
   JSONTypes_t  ValueType;
   char         *ErrCheck;
   char         NumberBuf[20], StrBuf[256];
   int          IntValue;
   float        FltValue;
   
   Obj->Updated = false;
      
   JsonStatus = JSON_SearchConst(Buf, BufLen, 
                                 Obj->Query.Key, Obj->Query.KeyLen,
                                 &Value, &ValueLen, &ValueType);
                                 
   if (JsonStatus == JSONSuccess)
   {
   
      CFE_EVS_SendEvent(CJSON_LOAD_OBJ_EID, CFE_EVS_EventType_DEBUG,
                        "CJSON_LoadObj: Type=%s, Value=%s, Len=%d",
                        JsonTypeStr[ValueType], Value, (unsigned int)ValueLen);

      switch (ValueType)
      {
         
         case JSONString:
         
            if (ValueLen <= Obj->TblDataLen)
            {

               strncpy(StrBuf,Value,ValueLen);
               StrBuf[ValueLen] = '\0';
               
               memcpy(Obj->TblData,StrBuf,ValueLen+1);
               Obj->Updated = true;
               RetStatus = true;
            
            }
            else
            {
               
               CFE_EVS_SendEvent(CJSON_LOAD_OBJ_ERR_EID, CFE_EVS_EventType_ERROR, 
                                 "JSON string length %d exceeds %s's max length %d", 
                                 (unsigned int)ValueLen, Obj->Query.Key, (unsigned int)Obj->TblDataLen);
            
            }
            break;
   
         case JSONNumber:
            
            strncpy(NumberBuf, Value, ValueLen);
            NumberBuf[ValueLen] = '\0';
            
            if (Obj->TypeFlt)
            {
               FltValue = (float)strtod(NumberBuf, &ErrCheck);
               if (ErrCheck != NumberBuf)
               {
                  memcpy(Obj->TblData, &FltValue, sizeof(float));
               }
            }
            else
            {
               IntValue = (int)strtol(NumberBuf, &ErrCheck, 10);
               if (ErrCheck != NumberBuf)
               {
                  memcpy(Obj->TblData, &IntValue, sizeof(int));
               }
            }
            if (ErrCheck != NumberBuf)
            {
               Obj->Updated = true;
               RetStatus = true;
            }
            else
            {
               CFE_EVS_SendEvent(CJSON_LOAD_OBJ_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "CJSON number conversion error for value %s",
                                 NumberBuf);
            }
            
            break;

         case JSONArray:
         
            CFE_EVS_SendEvent(CJSON_LOAD_OBJ_EID, CFE_EVS_EventType_INFORMATION,
                              "JSON array %s, len = %d", Value, (unsigned int)ValueLen);
            PrintJsonBuf(Value, ValueLen);
         
            break;

         case JSONObject:
         
            CFE_EVS_SendEvent(CJSON_LOAD_OBJ_EID, CFE_EVS_EventType_INFORMATION,
                              "JSON array %s, len = %d", Value, (unsigned int)ValueLen);
            PrintJsonBuf(Value, ValueLen);
         
            break;

         default:
         
            CFE_EVS_SendEvent(CJSON_LOAD_OBJ_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Unsupported JSON type %s returned for query %s", 
                              JsonTypeStr[ValueType], Obj->Query.Key);
      
      } /* End ValueType switch */
      
   }/* End if successful search */
   else 
   {
   
      if (Necessity == OBJ_REQUIRED)
      {
         CFE_EVS_SendEvent(CJSON_LOAD_OBJ_EID, CFE_EVS_EventType_INFORMATION,
                           "JSON search error for query %s. Status = %s.", 
                           Obj->Query.Key, JsonStatusStr[JsonStatus]);
      }
   }
         
   return RetStatus;
   
} /* End LoadObj() */


/******************************************************************************
** Function: PrintJsonBuf
**
** Notes:
**    1. OS_printf() limits the number of characters so this loops through
**       printing 100 bytes per OS_printf() call
**
*/
static void PrintJsonBuf(const char* JsonBuf, size_t BufLen)
{
   
   int  i = 0;
   char PrintBuf[PRINT_BUF_SEGMENT_BYTES+1];
   
   OS_printf("\n>>> JSON table file buffer:\n");
   for (i=0; i < BufLen; i += PRINT_BUF_SEGMENT_BYTES)
   {
      
      strncpy(PrintBuf, &JsonBuf[i], PRINT_BUF_SEGMENT_BYTES);
      PrintBuf[PRINT_BUF_SEGMENT_BYTES] = '\0';
      OS_printf("%s",PrintBuf); 
      
   }
   OS_printf("\n");
   
} /* End PrintJsonBuf() */


/******************************************************************************
** Function: ProcessFile
**
** Notes:
**  1. Entire JSON file is read into memory
**  2. User callback function LoadJsonData() or LoadJsonDataAlt() calls 
**     a CJSON_LoadObj*() method to load the JSON data into the user's table. 
**     The user's callback function can perform table-specific procesing such  
**     as validation prior to loading the table data.
**  3. The alternate callback method allows the user to pass in a pointer to
**     their JSON file processing data structure which is then passed back to
**     the callback function. This is needed in situations when the caller
**     needs to be reentrant and doesn't own the JSON file procesing data
**     structure. 
**
*/
static bool ProcessFile(const char* Filename, char* JsonBuf, size_t MaxJsonFileChar,
                        CJSON_LoadJsonData_t LoadJsonData,
                        CJSON_LoadJsonDataAlt_t LoadJsonDataAlt, void* UserDataPtr,
                        bool CallbackWithUserData)
{

   bool  RetStatus = false;
   
   osal_id_t     FileHandle;
   int32         SysStatus;
   int32         ReadStatus;
   JSONStatus_t  JsonStatus;
   os_err_name_t OsErrStr;
   
   SysStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_NONE, OS_READ_ONLY);
   
   /*
   ** Read entire JSON table into buffer. Logic kept very simple and JSON
   ** validate will catch if entire file wasn't read.
   */
   if (SysStatus == OS_SUCCESS)
   {

      ReadStatus = OS_read(FileHandle, JsonBuf, MaxJsonFileChar);

      if (ReadStatus >= 0)
      {

         if (DBG_JSON) PrintJsonBuf(JsonBuf, ReadStatus);
         
         /* ReadStatus equals buffer len */

         JsonStatus = JSON_Validate(JsonBuf, ReadStatus);

         if (JsonStatus == JSONSuccess)
         { 
            if (CallbackWithUserData)
            {
               RetStatus = LoadJsonDataAlt(ReadStatus,UserDataPtr);
            }
            else
            {
               RetStatus = LoadJsonData(ReadStatus);
            }
         }
         else
         {
         
            CFE_EVS_SendEvent(CJSON_PROCESS_FILE_ERR_EID, CFE_EVS_EventType_ERROR, 
                              "CJSON error validating file %s.  Status = %s.",
                              Filename, JsonStatusStr[JsonStatus]);

         }

      } /* End if valid read */
      else
      {
         
         CFE_EVS_SendEvent(CJSON_PROCESS_FILE_ERR_EID, CFE_EVS_EventType_ERROR, 
                           "CJSON error reading file %s. Status = %d",
                           Filename, ReadStatus);
         
      } /* End if invalid read */
   
      OS_close(FileHandle);
      
   }/* End if valid open */
   else
   {
      OS_GetErrorName(SysStatus, &OsErrStr);
      CFE_EVS_SendEvent(CJSON_PROCESS_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                        "CJSON error opening file %s. Status = %s", 
                        Filename, OsErrStr);
   }
   
   return RetStatus;
   
} /* End ProcessFile() */


/******************************************************************************
** Function: StubLoadJsonData
**
** Notes:
**   1. This serves as an unused stub parameter in calls to ProcessFile()
**      therefore it should never get executed.
**
*/
static bool StubLoadJsonData(size_t JsonFileLen)
{
   
   CFE_EVS_SendEvent(CJSON_INTERNAL_ERR_EID, CFE_EVS_EventType_CRITICAL, 
      "StubLoadJsonData() called, JsonFileLen %d. Code structural error that requires a developer",
      (unsigned int)JsonFileLen);
      
   return false;

} /* End StubLoadJsonData() */


/******************************************************************************
** Function: StubLoadJsonDataAlt
**
** Notes:
**   1. This serves as an unused stub parameter in calls to ProcessFile()
**      therefore it should never get executed.
**
*/
static bool StubLoadJsonDataAlt(size_t JsonFileLen, void* UserDataPtr)
{
   
   CFE_EVS_SendEvent(CJSON_INTERNAL_ERR_EID, CFE_EVS_EventType_CRITICAL, 
      "StubLoadJsonDataAlt() called, JsonFileLen %d, UserDataPtr 0x%p. Code structural error that requires a developer",
      (unsigned int)JsonFileLen, UserDataPtr);

   return false;

} /* End StubLoadJsonDataAlt() */
