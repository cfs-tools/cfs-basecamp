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
**    Define JSON Initialization table API
**
**  Notes:
**    1. Apps interface with IniTbl using the 'CFG_' definitions in
**       their app_cfg.h file. There are some details hidden from the API
**       that maintainers of this file should know. 'CFG_' definitions use
**       an IniLib enumtype that defines the first enumeration as 'start'
**       with a value of 0. The 'CFG_' parameter enum definitions follow
**       'start' so their values begin at 1. The 'CFG_' parameters are used
**       as an index into the config data storage array IniTbl->CfgData[]
**       and ('CFG_' - 1) is used to index into IniTbl->JsonParams[] because
**       CJSON assumes [0] is a valid entry.
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
#include "initbl.h"


/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool BuildJsonTblObjArray (INITBL_Class_t* IniTbl);
static bool LoadJsonData(size_t JsonFileLen, void* UserDataPtr);
static bool ValidJsonObjCfg(const INITBL_Class_t* IniTbl, uint16 JsonObjIndex, JSONTypes_t Type);


/******************************************************************************
** Function: INITBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
bool INITBL_Constructor(INITBL_Class_t *IniTbl, const char *IniTblFile,
                       INILIB_CfgEnum_t *CfgEnum)
{
   
   bool RetStatus = false;
   
   CFE_PSP_MemSet(IniTbl, 0, sizeof(INITBL_Class_t));
   IniTbl->CfgEnum = *CfgEnum;
 
 
   if (BuildJsonTblObjArray (IniTbl))
   {
      RetStatus = CJSON_ProcessFileAlt(IniTblFile, IniTbl->JsonBuf, INITBL_MAX_JSON_FILE_CHAR, LoadJsonData, IniTbl);
   }
   else 
   {
      CFE_EVS_SendEvent(INITBL_CONFIG_DEF_ERR_EID, CFE_EVS_EventType_ERROR,
                        "JSON INITBL definition error. JSON config file contains % d which is greater than frame maximum defined at %d",
                        IniTbl->CfgEnum.End, INITBL_MAX_CFG_ITEMS);
   }
   
   return RetStatus;
   
} /* End INITBL_Constructor() */


/******************************************************************************
** Function: INITBL_GetFltConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retrieving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. If the parameter is out of range or of the wrong type, a zero is
**       returned and an event message is sent.
**
*/
float INITBL_GetFltConfig(const INITBL_Class_t *IniTbl, uint16 Param)
{
   
   float RetValue = 0.0;
   uint16 JsonObjIndex = (Param-1);

   if (ValidJsonObjCfg(IniTbl, JsonObjIndex, JSONNumber))
   {
      RetValue = IniTbl->CfgData[Param].Flt;
   }

   return RetValue;
   
} /* INITBL_GetFltConfig() */


/******************************************************************************
** Function: INITBL_GetIntConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retrieving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. If the parameter is out of range or of the wrong type, a zero is
**       returned and an event message is sent.
**
*/
uint32 INITBL_GetIntConfig(const INITBL_Class_t *IniTbl, uint16 Param)
{
   
   uint32 RetValue = 0;
   uint16 JsonObjIndex = (Param-1);
   
   if (ValidJsonObjCfg(IniTbl, JsonObjIndex, JSONNumber))
   {
      RetValue = IniTbl->CfgData[Param].Int;
   }

   return RetValue;
   
} /* INITBL_GetIntConfig() */


/******************************************************************************
** Function: INITBL_GetStrConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retrieving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. If the parameter is out of range or of the wrong type, a null string 
**       is returned and an event message is sent.
**
*/
const char* INITBL_GetStrConfig(const INITBL_Class_t *IniTbl, uint16 Param)
{
   
   const char* RetStrPtr = NULL;
   uint16      JsonObjIndex = (Param-1);
   
   if (ValidJsonObjCfg(IniTbl, JsonObjIndex, JSONString))
   {
      RetStrPtr = IniTbl->CfgData[Param].Str;
   }

   return RetStrPtr;
   
} /* INITBL_GetStrConfig() */


/******************************************************************************
** Function: BuildJsonTblObjArray
**
** Notes:
**   1. This uses the INILIB parameter definitions to create an array of 
**      CJSON_Obj_t that can be used to process the JSON ini file.
**
*/
static bool BuildJsonTblObjArray (INITBL_Class_t *IniTbl)
{

   bool RetStatus = true;
   int  Param, i;
   const char *CfgStrPtr;
   const char *CfgTypePtr;
   
   
   IniTbl->JsonParamCnt = 0;

   /* 
   ** IniTbl->CfgEnum.Start is defined as 0 so the first array index is unused.
   ** 'i' starts at 0 and is used to index IniTbl->JsonParams[] because the CJSON
   ** assumes [0] is a valid entry. See file prologue for more details. 
   */
   if (IniTbl->CfgEnum.End <= (INITBL_MAX_CFG_ITEMS+1))
   {
  
      IniTbl->JsonParamCnt = IniTbl->CfgEnum.End - 1;
      
      for ( Param=(IniTbl->CfgEnum.Start+1),i=0; Param < IniTbl->CfgEnum.End; Param++,i++)
      {
   
         CfgStrPtr  = (IniTbl->CfgEnum.GetStr)(Param);
         CfgTypePtr = (IniTbl->CfgEnum.GetType)(Param);
         
         CJSON_Obj_t *JsonParam = &IniTbl->JsonParams[i];
         
         if (strcmp(CfgTypePtr, INILIB_TYPE_INT) == 0)
         {
      
            JsonParam->TblData      = &IniTbl->CfgData[Param].Int;
            JsonParam->TblDataLen   = sizeof(uint32);
            JsonParam->Updated      = false;
            JsonParam->Type         = JSONNumber;
            JsonParam->TypeFlt      = false;
            strncpy(JsonParam->Query.Key, INITBL_JSON_CONFIG_OBJ_PREFIX, CJSON_MAX_KEY_LEN);
            strncat(JsonParam->Query.Key, CfgStrPtr, CJSON_MAX_KEY_LEN);
            JsonParam->Query.KeyLen = strlen(JsonParam->Query.Key);
         
         } /* End if integer */
         else if (strcmp(CfgTypePtr, INILIB_TYPE_FLT) == 0)
         {
            JsonParam->TblData      = &IniTbl->CfgData[Param].Flt;
            JsonParam->TblDataLen   = sizeof(float);
            JsonParam->Updated      = false;
            JsonParam->Type         = JSONNumber;
            JsonParam->TypeFlt      = true;
            strncpy(JsonParam->Query.Key, INITBL_JSON_CONFIG_OBJ_PREFIX, CJSON_MAX_KEY_LEN);
            strncat(JsonParam->Query.Key, CfgStrPtr, CJSON_MAX_KEY_LEN);
            JsonParam->Query.KeyLen = strlen(JsonParam->Query.Key);
         } /* End if float */
         else if (strcmp(CfgTypePtr, INILIB_TYPE_STR) == 0)
         {
            JsonParam->TblData      = &IniTbl->CfgData[Param].Str;
            JsonParam->TblDataLen   = INITBL_MAX_CFG_STR_LEN;
            JsonParam->Updated      = false;
            JsonParam->Type         = JSONString;
            JsonParam->TypeFlt      = false;
            strncpy(JsonParam->Query.Key, INITBL_JSON_CONFIG_OBJ_PREFIX, CJSON_MAX_KEY_LEN);
            strncat(JsonParam->Query.Key, CfgStrPtr, CJSON_MAX_KEY_LEN);
            JsonParam->Query.KeyLen = strlen(JsonParam->Query.Key);

         }  /* End if string */
         else 
         {
            RetStatus = false;
            CFE_EVS_SendEvent(INITBL_CFG_PARAM_ERR_EID, CFE_EVS_EventType_ERROR,
                               "Invalid Configuration parameter type %s", CfgTypePtr);
         }

      } /* End Param loop */      
   } /* End if valid number of paramaters */
   else
   {
      
      RetStatus = false;
      CFE_EVS_SendEvent(INITBL_CFG_PARAM_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Number of configuration parameters %d is greater than IniTbl max %d",
                        IniTbl->CfgEnum.End, (INITBL_MAX_CFG_ITEMS+1));
                      
   } /* End if invalid number of paramaters */     
   
   
   return RetStatus;

} /* BuildJsonTblObjArray() */



/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. This is a callback function from CJSON. All of the initialization
**     configuration parameters should be defined and it is considered and
**     error if this is not the case.
*/
static bool LoadJsonData(size_t JsonFileLen, void *UserDataPtr)
{

   bool            RetStatus = false;
   size_t          ObjLoadCnt;
   INITBL_Class_t* IniTbl = (INITBL_Class_t*)UserDataPtr; 


   IniTbl->JsonFileLen = JsonFileLen;
   
   ObjLoadCnt = CJSON_LoadObjArray(IniTbl->JsonParams, IniTbl->JsonParamCnt, IniTbl->JsonBuf, IniTbl->JsonFileLen);

   if (ObjLoadCnt == IniTbl->JsonParamCnt)
   {
      RetStatus = true;
      CFE_EVS_SendEvent(INITBL_LOAD_JSON_EID, CFE_EVS_EventType_INFORMATION, 
                        "JSON initialization file successfully processed with %d parameters",
                        (unsigned int)IniTbl->JsonParamCnt);
   }
   else
   {
      CFE_EVS_SendEvent(INITBL_LOAD_JSON_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing JSON initialization file. %d of %d parameters processed",
                        (unsigned int)ObjLoadCnt, (unsigned int)IniTbl->JsonParamCnt);  
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */


/******************************************************************************
** Function: ValidJsonObjCfg
**
*/
static bool ValidJsonObjCfg(const INITBL_Class_t *IniTbl, uint16 JsonObjIndex, JSONTypes_t Type)
{
   
   bool RetStatus = false;
   
   
   CFE_EVS_SendEvent(INITBL_CFG_PARAM_EID, CFE_EVS_EventType_DEBUG,
                     "ValidJsonObjCfg %d: Type = %s, Key %s with type %s\n", 
                     JsonObjIndex, CJSON_ObjTypeStr(Type), 
                     IniTbl->JsonParams[JsonObjIndex].Query.Key, 
                     CJSON_ObjTypeStr(IniTbl->JsonParams[JsonObjIndex].Type));      
   
   if ( JsonObjIndex >= IniTbl->CfgEnum.Start && JsonObjIndex < IniTbl->CfgEnum.End) 
   {
   
      if (IniTbl->JsonParams[JsonObjIndex].Updated)
      {
         if (IniTbl->JsonParams[JsonObjIndex].Type == Type)
         {
            RetStatus = true;
         }
         else
         {
            CFE_EVS_SendEvent(INITBL_CFG_PARAM_ERR_EID, CFE_EVS_EventType_ERROR, 
                              "Attempt to retrieve parameter of type %s that was loaded as type %s",
                              CJSON_ObjTypeStr(Type), CJSON_ObjTypeStr(IniTbl->JsonParams[JsonObjIndex].Type));      
         }
      }
      else
      {
         CFE_EVS_SendEvent(INITBL_CFG_PARAM_ERR_EID, CFE_EVS_EventType_ERROR, 
                           "Attempt to retrieve uninitialized parameter %d",
                           JsonObjIndex);
      }         
   }
   else
   {
      CFE_EVS_SendEvent(INITBL_CFG_PARAM_ERR_EID, CFE_EVS_EventType_ERROR, "Attempt to retrieve invalid parameter %d that is not in valid range: %d < param < %d",
                        JsonObjIndex, IniTbl->CfgEnum.Start, IniTbl->CfgEnum.End);
   }
   
   return RetStatus;
   
} /* ValidJsonObjCfg() */
