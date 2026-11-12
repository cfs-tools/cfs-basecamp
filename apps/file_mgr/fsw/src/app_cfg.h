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
**    Define application configurations for the File Manager (FILE_MGR) application
**
**  Notes:
**    1. FILE_MGR is a refactoring of NASA's FM app using the app C framework.
**       It's also a prototype for using a JSON init file for application 
**       parameters that can be specified during runtime. 
**    2. These configurations should have an application scope and define
**       parameters that shouldn't need to change across deployments. If
**       a change is made to this file or any other app source file during
**       a deployment then the definition of the FILE_MGR_PLATFORM_REV
**       macro in file_mgr_platform_cfg.h should be updated.
**
*/

#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "file_mgr_eds_typedefs.h"
#include "file_mgr_eds_designparameters.h"

#include "file_mgr_platform_cfg.h"
#include "app_c_fw.h"

/******************************************************************************
** Versions:
**
** 1.0 - Initial refactoring of open source FM 2.5.2
** 1.1 - Moved childmgr utility into app_c_fw, Moved perf & msg ids to ini file
** 3.0 - New baseline for separate OSK app repo compatible with cFE Bootes
** 4.0 - New baseline for separate OSK app repo compatible with cFE Caelum
** 5.0 - Upgrade to cFS Draco (v7.0.1) and changed filesys table from cFE binary
**       to Basecamp JSON
*/

#define  FILE_MGR_MAJOR_VER      5
#define  FILE_MGR_MINOR_VER      0


/******************************************************************************
** JSON init file definitions/declarations.
** - See app_c_demo::app_cfg.h for how to define configuration macros 
*/

#define CFG_APP_CFE_NAME        APP_CFE_NAME

#define CFG_APP_MAIN_PERF_ID    APP_MAIN_PERF_ID
#define CFG_CHILD_TASK_PERF_ID  CHILD_TASK_PERF_ID
      
#define CFG_CMD_PIPE_DEPTH      APP_CMD_PIPE_DEPTH
#define CFG_CMD_PIPE_NAME       APP_CMD_PIPE_NAME
      
#define CFG_FILE_MGR_CMD_TOPICID            FILE_MGR_CMD_TOPICID
#define CFG_FILE_MGR_SEND_HK_TOPICID        BC_SCH_2_SEC_TOPICID

#define CFG_FILE_MGR_HK_TLM_TOPICID         FILE_MGR_HK_TLM_TOPICID
#define CFG_FILE_MGR_FILE_INFO_TLM_TOPICID  FILE_MGR_FILE_INFO_TLM_TOPICID
#define CFG_FILE_MGR_DIR_LIST_TLM_TOPICID   FILE_MGR_DIR_LIST_TLM_TOPICID
#define CFG_FILE_MGR_OPEN_FILE_TLM_TOPICID  FILE_MGR_OPEN_FILE_TLM_TOPICID
#define CFG_FILE_MGR_FILE_SYS_EST_SPACE_TLM_TOPICID FILE_MGR_FILE_SYS_EST_SPACE_TLM_TOPICID
        
#define CFG_FILESYS_TBL_NAME       FILESYS_TBL_NAME
#define CFG_FILESYS_TBL_LOAD_FILE  FILESYS_TBL_LOAD_FILE

#define CFG_DIR_LIST_FILE_DEFNAME    DIR_LIST_FILE_DEFNAME
#define CFG_DIR_LIST_FILE_SUBTYPE    DIR_LIST_FILE_SUBTYPE
#define CFG_DIR_LIST_FILE_ENTRY_MAX  DIR_LIST_FILE_ENTRY_MAX
#define CFG_SEND_DIR_TLM_DELAY       SEND_DIR_TLM_DELAY

#define CFG_CHILD_NAME             CHILD_NAME
#define CFG_CHILD_STACK_SIZE       CHILD_STACK_SIZE
#define CFG_CHILD_PRIORITY         CHILD_PRIORITY
      
#define CFG_TASK_FILE_BLOCK_CNT    TASK_FILE_BLOCK_CNT
#define CFG_TASK_FILE_BLOCK_DELAY  TASK_FILE_BLOCK_DELAY
#define CFG_TASK_FILE_STAT_CNT     TASK_FILE_STAT_CNT
#define CFG_TASK_FILE_STAT_DELAY   TASK_FILE_STAT_DELAY


#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_MAIN_PERF_ID,uint32) \
   XX(CHILD_TASK_PERF_ID,uint32) \
   XX(APP_CMD_PIPE_DEPTH,uint32) \
   XX(APP_CMD_PIPE_NAME,char*) \
   XX(FILE_MGR_CMD_TOPICID,uint32) \
   XX(BC_SCH_2_SEC_TOPICID,uint32) \
   XX(FILE_MGR_HK_TLM_TOPICID,uint32) \
   XX(FILE_MGR_FILE_INFO_TLM_TOPICID,uint32) \
   XX(FILE_MGR_DIR_LIST_TLM_TOPICID,uint32) \
   XX(FILE_MGR_OPEN_FILE_TLM_TOPICID,uint32) \
   XX(FILE_MGR_FILE_SYS_EST_SPACE_TLM_TOPICID,uint32) \
   XX(FILESYS_TBL_NAME,char*) \
   XX(FILESYS_TBL_LOAD_FILE,char*) \
   XX(DIR_LIST_FILE_DEFNAME,char*) \
   XX(DIR_LIST_FILE_SUBTYPE,uint32) \
   XX(DIR_LIST_FILE_ENTRY_MAX,uint32) \
   XX(SEND_DIR_TLM_DELAY,uint32) \
   XX(CHILD_NAME,char*) \
   XX(CHILD_STACK_SIZE,uint32) \
   XX(CHILD_PRIORITY,uint32) \
   XX(TASK_FILE_BLOCK_CNT,uint32) \
   XX(TASK_FILE_BLOCK_DELAY,uint32) \
   XX(TASK_FILE_STAT_CNT,uint32) \
   XX(TASK_FILE_STAT_DELAY,uint32) \

DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define FILE_MGR_BASE_EID    (EdsParam_APP_C_FW_APP_BASE_EID +  0)
#define DIR_BASE_EID         (EdsParam_APP_C_FW_APP_BASE_EID + 20)
#define FILE_BASE_EID        (EdsParam_APP_C_FW_APP_BASE_EID + 40)
#define FILESYS_BASE_EID     (EdsParam_APP_C_FW_APP_BASE_EID + 60)
#define FILESYS_TBL_BASE_EID (EdsParam_APP_C_FW_APP_BASE_EID + 80)


/******************************************************************************
** File System Table Macros
*/

#define FILESYS_TBL_JSON_MAX_OBJ          10
#define FILESYS_TBL_JSON_FILE_MAX_CHAR  4090 


#endif /* _app_cfg_ */
