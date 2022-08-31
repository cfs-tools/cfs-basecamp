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
**    Define a coreJSON adapter for JSON table management
**
**  Notes:
**    1. Provide an interface to the FreeRTOS coreJSON library that simplifies
**       app JSON table management and makes the usage consistent.
**    2. The JSON table is a stream of characters so string terminators are not
**       part of length definitions 
**    3. This is designed as a functional library rather than an object-based
**       service for a couple of reasons. 
**       - Users can write custom code for table loads. A general purpose load
**         could be provided with a table validation callback but when I looked
**         into the user table-to-CJSON interface it got more complex. The
**         amount of code a user needs to write with the current design is 
**         trivial and it allows more control especially for partial table load
**         situations.
**    4. Supported JSON types as defined by core_json
**       - JSONNumber
**       - JSONString
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _cjson_
#define _cjson_

/*
** Includes
*/

#include "osk_c_fw_cfg.h"
#include "core_json.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define CJSON_MAX_KEY_LEN  64  /* Number of characters in key, does not include a string terminator */ 

/*
** Event Message IDs
*/

#define CJSON_OBJ_CONSTRUCT_ERR_EID  (CJSON_BASE_EID + 0)
#define CJSON_PROCESS_FILE_ERR_EID   (CJSON_BASE_EID + 1)
#define CJSON_LOAD_OBJ_EID           (CJSON_BASE_EID + 2)
#define CJSON_LOAD_OBJ_ERR_EID       (CJSON_BASE_EID + 3)
#define CJSON_INTERNAL_ERR_EID       (CJSON_BASE_EID + 4)

/**********************/
/** Type Definitions **/
/**********************/

/* TODO - Consider refactor into 2 structures so one can be passed as a const */

typedef struct
{

   char     Key[CJSON_MAX_KEY_LEN];
   size_t   KeyLen;

} CJSON_Query_t;

typedef struct
{

   void*          TblData;
   size_t         TblDataLen;
   bool           Updated;
   JSONTypes_t    Type;
   bool           TypeFlt;   /* Distinguish between integer and float number types */
   CJSON_Query_t  Query;

} CJSON_Obj_t;


/*
** These structures are helpful when processing JSON tables with more complex 
** data structures such as arrays. The CJSON_LoadObj*() methods are typically
** used as each array element is processed. It's often convenient to defined
** both CJSON_OBJ information with the data storage.
*/

typedef struct
{
   CJSON_Obj_t  Obj;
   uint16       Value;
} CJSON_IntObj_t;

typedef struct
{
   CJSON_Obj_t  Obj;
   char         Value[INITBL_MAX_CFG_STR_LEN];
} CJSON_StrObj_t;


/* User callback function to load table data */
typedef bool (*CJSON_LoadJsonData_t)(size_t JsonFileLen);
typedef bool (*CJSON_LoadJsonDataAlt_t)(size_t JsonFileLen, void* UserDataPtr);


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: CJSON_FltObjConstructor
**
** Notes:
**    1. Float support was added much later than initial release so in order
**       to preserve the API and since float objects are the exception, a 
**       separate constructor was added.
*/
void CJSON_FltObjConstructor(CJSON_Obj_t *Obj, const char *QueryKey, 
                             JSONTypes_t JsonType, void *TblData, size_t TblDataLen);


/******************************************************************************
** Function: CJSON_ObjConstructor
**
** Initialize a JSON table object
**
** Notes:
**   1. See file prologue for supported JSON types.
**
*/
void CJSON_ObjConstructor(CJSON_Obj_t *Obj, const char *QueryKey, 
                          JSONTypes_t JsonType, void *TblData, size_t TblDataLen);


/******************************************************************************
** Function: CJSON_LoadObj
**
** Notes:
**   1. It is considered an error if the object is not found 
**   2. See file prologue for supported JSON types.
**
*/
bool CJSON_LoadObj(CJSON_Obj_t *Obj, const char *Buf, size_t BufLen);


/******************************************************************************
** Function: CJSON_LoadObjArray
**
** Notes:
**   1. See file prologue for supported JSON types.
**
*/
size_t CJSON_LoadObjArray(CJSON_Obj_t *Obj, size_t ObjCnt, const char* Buf, size_t BufLen);


/******************************************************************************
** Function: CJSON_LoadObjOptional
**
** Notes:
**   1. If the object is not found an event message is not sent. This is useful
**      for objects that are optional or when an array is being traversed until
**      an entry is not found
**   2. See file prologue for supported JSON types.
**
*/
bool CJSON_LoadObjOptional(CJSON_Obj_t *Obj, const char *Buf, size_t BufLen);


/******************************************************************************
** Function: CJSON_ObjTypeStr
**
** Notes:
**   1. Returns a string for the enumerated type. 
*/
const char* CJSON_ObjTypeStr(JSONTypes_t  ObjType);


/******************************************************************************
** Function: CJSON_ProcessFile
**
** Notes:
**  1. Takes care of all generic table processing and validation. User's 
**     callback function performs table-specific data processing.
*/
bool CJSON_ProcessFile(const char *Filename, char *JsonBuf, 
                       size_t MaxJsonFileChar, CJSON_LoadJsonData_t LoadJsonData);


/******************************************************************************
** Function: CJSON_ProcessFileAlt
**
** Notes:
**  1. Takes care of all generic table processing and validation. User's 
**     callback function performs table-specific data processing
**  2. Same functionality as CJSON_ProcessFile except the callback function
**     has the UserDataPtr passed as a parameter.
*/
bool CJSON_ProcessFileAlt(const char *Filename, char *JsonBuf, 
                          size_t MaxJsonFileChar, CJSON_LoadJsonDataAlt_t LoadJsonDataAlt,
                          void* UserDataPtr);

#endif /* _cjson_ */
