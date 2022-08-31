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
**    None
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _ini_tbl_
#define _ini_tbl_

/*
** Include Files
*/

#include "osk_c_fw.h" /* Needs JSON with FW config so just include everything */


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Table Structure Objects 
*/

#define INITBL_JSON_CONFIG_OBJ_PREFIX  "config."

#define INITBL_CONFIG_DEF_ERR_EID  (INITBL_BASE_EID + 0)
#define INITBL_CFG_PARAM_EID       (INITBL_BASE_EID + 1)
#define INITBL_CFG_PARAM_ERR_EID   (INITBL_BASE_EID + 2)
#define INITBL_LOAD_JSON_EID       (INITBL_BASE_EID + 3)
#define INITBL_LOAD_JSON_ERR_EID   (INITBL_BASE_EID + 4)

/**********************/
/** Type Definitions **/
/**********************/

typedef struct 
{
   
   uint32   Int;
   float    Flt;
   char     Str[INITBL_MAX_CFG_STR_LEN];

} INITBL_CfgData_t;


typedef struct 
{
 
   INILIB_CfgEnum_t  CfgEnum;
   INITBL_CfgData_t  CfgData[INITBL_MAX_CFG_ITEMS+1];  /* '+1' accounts for [0] being unused */
   
   size_t      JsonParamCnt;
   CJSON_Obj_t JsonParams[INITBL_MAX_CFG_ITEMS+1];       /* Indexed via cfg param; '+1' accounts for [0] being ununsed */

   size_t      JsonFileLen;
   char        JsonBuf[INITBL_MAX_JSON_FILE_CHAR];   

} INITBL_Class_t;
 
 
/******************************************************************************
** Function: INITBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**    2. Reads, validates, and processes the JSON file. If construction is
**       successful then the query functions below can be used using the
**       "CFG_" parameters defined in app_cfg.h.
**
*/
bool INITBL_Constructor(INITBL_Class_t *IniTbl, const char *IniFile,
                        INILIB_CfgEnum_t *CfgEnum);


/******************************************************************************
** Function: INITBL_GetFltConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retreiving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. Param is one of the JSON init file "CFG_" configuration parameters
**       defined in ap_cfg.h.  If the parameter is out of range or of the wrong
**       type, a zero is returned and an event message is sent. If the 
**       parameters are defined correctly they should neverbe out of range. 
**
*/
float INITBL_GetFltConfig(const INITBL_Class_t *IniTbl, uint16 Param);


/******************************************************************************
** Function: INITBL_GetIntConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retreiving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. Param is one of the JSON init file "CFG_" configuration parameters
**       defined in ap_cfg.h.  If the parameter is out of range or of the wrong
**       type, a zero is returned and an event message is sent. If the 
**       parameters are defined correctly they should neverbe out of range. 
**
*/
uint32 INITBL_GetIntConfig(const INITBL_Class_t *IniTbl, uint16 Param);


/******************************************************************************
** Function: INITBL_GetStrConfig
**
** Notes:
**    1. This does not return a status as to whether the configuration 
**       parameter was successfully retrieved. The logic for retreiving
**       parameters should be simple and any issues should be resolved during
**       testing.
**    2. Param is one of the JSON init file "CFG_" configuration parameters
**       defined in ap_cfg.h.  If the parameter is out of range or of the wrong
**       type, a zero is returned and an event message is sent. If the 
**       parameters are defined correctly they should neverbe out of range. 
**
*/
const char* INITBL_GetStrConfig(const INITBL_Class_t *IniTbl, uint16 Param);


#endif /* _ini_tbl_ */
