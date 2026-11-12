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
**    Define configurations for the Scheduler application
**
**  Notes:
**    1. These configurations should have an application scope and define
**       parameters that shouldn't need to change across deployments. If
**       a change is made to this file or any other app source file during
**       a deployment then the definition of the KIT_SCH_PLATFORM_REV
**       macro in kit_sch_platform_cfg.h should be updated.
**
*/

#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "app_c_fw.h"
#include "kit_sch_platform_cfg.h"
#include "kit_sch_eds_typedefs.h"

/******************************************************************************
** Versions
**   
** 1.0 - Initial release
** 1.1 - Refactored for OSK 2.2
** 2.0 - Added Sch & Msg table commands and diagnostics telemetry
** 3.0 - New baseline for separate OSK app repo compatible with cFE Bootes
** 5.0 - Updated to cFS 7.0 (Basecamp 4.0 release)
*/

#define  KIT_SCH_MAJOR_VER      4
#define  KIT_SCH_MINOR_VER      0


/******************************************************************************
** JSON init file definitions/declarations.
**
** CFG_STARTUP_SYNCH_TIMEOUT
**   Defines the timeout for the CFE_ES_WaitForStartupSync call during app
**   initialization. The scheduler will wait this amount of time before
**   assuming all apps have been started and will then begin nominal scheduler
**   processing.
*/

#define CFG_APP_CFE_NAME   APP_CFE_NAME
#define CFG_APP_PERF_ID    APP_PERF_ID

#define CFG_KIT_SCH_CMD_TOPICID           KIT_SCH_CMD_TOPICID
#define CFG_KIT_SCH_SEND_HK_TOPICID       BC_SCH_4_SEC_TOPICID
#define CFG_KIT_SCH_HK_TLM_TOPICID        KIT_SCH_HK_TLM_TOPICID
#define CFG_KIT_SCH_DIAG_TLM_TOPICID      KIT_SCH_DIAG_TLM_TOPICID
#define CFG_KIT_SCH_TBL_ENTRY_TLM_TOPICID KIT_SCH_TBL_ENTRY_TLM_TOPICID

#define CFG_CMD_PIPE_NAME         CMD_PIPE_NAME
#define CFG_CMD_PIPE_DEPTH        CMD_PIPE_DEPTH

#define CFG_MSG_TBL_LOAD_FILE     MSG_TBL_LOAD_FILE
#define CFG_MSG_TBL_DUMP_FILE     MSG_TBL_DUMP_FILE

#define CFG_SCH_TBL_LOAD_FILE     SCH_TBL_LOAD_FILE
#define CFG_SCH_TBL_DUMP_FILE     SCH_TBL_DUMP_FILE

#define CFG_USRMSG_TBL_LOAD_FILE  USRMSG_TBL_LOAD_FILE
#define CFG_USRMSG_TBL_DUMP_FILE  USRMSG_TBL_DUMP_FILE

#define CFG_STARTUP_SYNC_TIMEOUT  STARTUP_SYNC_TIMEOUT

#define CFG_INIT_DEBUG_EVT_TYPE     INIT_DEBUG_EVT_TYPE
#define CFG_LOW_FREQ_DEBUG_EVT_TYPE LOW_FREQ_DEBUG_EVT_TYPE

#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_PERF_ID,uint32) \
   XX(KIT_SCH_CMD_TOPICID,uint32) \
   XX(BC_SCH_4_SEC_TOPICID,uint32) \
   XX(KIT_SCH_HK_TLM_TOPICID,uint32) \
   XX(KIT_SCH_DIAG_TLM_TOPICID,uint32) \
   XX(KIT_SCH_TBL_ENTRY_TLM_TOPICID,uint32) \
   XX(CMD_PIPE_NAME,char*) \
   XX(CMD_PIPE_DEPTH,uint32) \
   XX(MSG_TBL_LOAD_FILE,char*) \
   XX(MSG_TBL_DUMP_FILE,char*) \
   XX(SCH_TBL_LOAD_FILE,char*) \
   XX(SCH_TBL_DUMP_FILE,char*) \
   XX(USRMSG_TBL_LOAD_FILE,char*) \
   XX(USRMSG_TBL_DUMP_FILE,char*) \
   XX(STARTUP_SYNC_TIMEOUT,uint32) \
   XX(INIT_DEBUG_EVT_TYPE,uint32) \
   XX(LOW_FREQ_DEBUG_EVT_TYPE,uint32) \
   
DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
**
*/

#define KIT_SCH_APP_BASE_EID  (EdsParam_APP_C_FW_APP_BASE_EID +   0)
#define SCHTBL_BASE_EID       (EdsParam_APP_C_FW_APP_BASE_EID + 100)
#define MSGTBL_BASE_EID       (EdsParam_APP_C_FW_APP_BASE_EID + 200)
#define USRMSGTBL_BASE_EID    (EdsParam_APP_C_FW_APP_BASE_EID + 300)
#define SCHEDULER_BASE_EID    (EdsParam_APP_C_FW_APP_BASE_EID + 400)


/******************************************************************************
** Table Macros
*/

#define MSGTBL_NAME     "Message" 
#define SCHTBL_NAME     "Schedule"
#define USRMSGTBL_NAME  "User Message" 


#endif /* _app_cfg_ */
