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
**    Define application configurations for the OSK C Demo application
**
**  Notes:
**    1. These configurations should have an application scope and define
**       parameters that shouldn't need to change across deployments. If
**       a change is made to this file or any other app source file during
**       a deployment then the definition of the OSK_C_DEMO_PLATFORM_REV
**       macro in osk_c_demo_platform_cfg.h should be updated.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "osk_c_fw.h"
#include "osk_c_demo_platform_cfg.h"
#include "osk_c_demo_eds_typedefs.h"

/******************************************************************************
** Versions
**
** 1.0 - Initial version
** 3.0 - New baseline for OSK app repo compatible with cFE Bootes
** 4.0 - New baseline for OSK app repo compatible with cFE Caelum
**       Changed app functionality from logging message headers to managing a 
**       histogram for data read from a simulated device. A data processing
**       app is more in line with what cubesat developers need to do.
*/

#define  OSK_C_DEMO_MAJOR_VER   4
#define  OSK_C_DEMO_MINOR_VER   0


/******************************************************************************
** Init JSON file declarations. The following steps show how to define and
** use initialization parameters defined in the JSON ini file. Users don't 
** need to know the structures created by the macros but they are shown for
** completeness. The app's command pipe definitions are used as an example. 
**
** 1. Define configuration parameter names
**
**    #define CFG_CMD_PIPE_NAME   CMD_PIPE_NAME
**    #define CFG_CMD_PIPE_DEPTH  CMD_PIPE_DEPTH
**
** 2. Add the parameter to the APP_CONFIG(XX) macro using the name as defined
**    in step 1
**
**    #define APP_CONFIG(XX) \
**       XX(CMD_PIPE_NAME,char*) \
**       XX(CMD_PIPE_DEPTH,uint32) \
**
** 3. Define the parameterin the JSON ini file's "config" object using the
**    same parameter as defined in step 1
**
**    "config": {
**       "CMD_PIPE_NAME":  "OSK_C_DEMO_CMD",
**       "CMD_PIPE_DEPTH": 5,
** 
** 4. Access the parameteres in your code 
**    
**    INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME)
**    INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH)
**
** The following declarations are created using the APP_CONFIG(XX) and 
** XX(name,type) macros:
** 
**    typedef enum {
**       CMD_PIPE_DEPTH,
**       CMD_PIPE_NAME
**    } INITBL_ConfigEnum;
**    
**    typedef struct {
**       CMD_PIPE_DEPTH,
**       CMD_PIPE_NAME
**    } INITBL_ConfigStruct;
**
**    const char *GetConfigStr(value);
**    ConfigEnum GetConfigVal(const char *str);
**
*/

#define CFG_APP_CFE_NAME        APP_CFE_NAME
#define CFG_APP_PERF_ID         APP_PERF_ID

#define CFG_APP_CMD_PIPE_NAME   APP_CMD_PIPE_NAME
#define CFG_APP_CMD_PIPE_DEPTH  APP_CMD_PIPE_DEPTH

#define CFG_OSK_C_DEMO_CMD_TOPICID             OSK_C_DEMO_CMD_TOPICID
#define CFG_OSK_C_DEMO_EXE_TOPICID             OSK_C_DEMO_EXE_TOPICID
#define CFG_OSK_C_DEMO_STATUS_TLM_TOPICID      OSK_C_DEMO_STATUS_TLM_TOPICID
#define CFG_OSK_C_DEMO_BIN_PLAYBK_TLM_TOPICID  OSK_C_DEMO_BIN_PLAYBK_TLM_TOPICID

#define CFG_CHILD_NAME        CHILD_NAME
#define CFG_CHILD_PERF_ID     CHILD_PERF_ID
#define CFG_CHILD_STACK_SIZE  CHILD_STACK_SIZE
#define CFG_CHILD_PRIORITY    CHILD_PRIORITY

#define CFG_DEVICE_DATA_MODULO  DEVICE_DATA_MODULO

#define CFG_HIST_LOG_FILE_PREFIX    HIST_LOG_FILE_PREFIX
#define CFG_HIST_LOG_FILE_EXTENSION HIST_LOG_FILE_EXTENSION

#define CFG_HIST_TBL_LOAD_FILE      HIST_TBL_LOAD_FILE
#define CFG_HIST_TBL_DUMP_FILE      HIST_TBL_DUMP_FILE


#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_PERF_ID,uint32) \
   XX(APP_CMD_PIPE_NAME,char*) \
   XX(APP_CMD_PIPE_DEPTH,uint32) \
   XX(OSK_C_DEMO_CMD_TOPICID,uint32) \
   XX(OSK_C_DEMO_EXE_TOPICID,uint32) \
   XX(OSK_C_DEMO_STATUS_TLM_TOPICID,uint32) \
   XX(OSK_C_DEMO_BIN_PLAYBK_TLM_TOPICID,uint32) \
   XX(CHILD_NAME,char*) \
   XX(CHILD_PERF_ID,uint32) \
   XX(CHILD_STACK_SIZE,uint32) \
   XX(CHILD_PRIORITY,uint32) \
   XX(DEVICE_DATA_MODULO,uint32)\
   XX(HIST_LOG_FILE_PREFIX,char*) \
   XX(HIST_LOG_FILE_EXTENSION,char*) \
   XX(HIST_TBL_LOAD_FILE,char*) \
   XX(HIST_TBL_DUMP_FILE,char*) \

DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define OSK_C_DEMO_BASE_EID     (OSK_C_FW_APP_BASE_EID +  0)
#define DEVICE_BASE_EID         (OSK_C_FW_APP_BASE_EID + 20)
#define HISTOGRAM_BASE_EID      (OSK_C_FW_APP_BASE_EID + 40)
#define HISTOGRAM_LOG_BASE_EID  (OSK_C_FW_APP_BASE_EID + 60)
#define HISTOGRAM_TBL_BASE_EID  (OSK_C_FW_APP_BASE_EID + 80)

/******************************************************************************
** Histogram Macros
*/

#define HISTOGRAM_MAX_BINS  10

/******************************************************************************
** Histogram Log Macros
*/

#define HISTOGRAM_LOG_FILE_EXT_MAX_LEN  5  /* Includes '.' and null terminator */


/******************************************************************************
** Histogram Table Macros
*/

#define HISTOGRAM_TBL_JSON_MAX_OBJ          10
#define HISTOGRAM_TBL_JSON_FILE_MAX_CHAR  4090 

#endif /* _app_cfg_ */
