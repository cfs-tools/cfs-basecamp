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
**    Define application configurations for the File Transfer (FILE_XFER) application
**
**  Notes:
**    1. These configurations should have an application scope and define
**       parameters that shouldn't need to change across deployments. If
**       a change is made to this file or any other app source file during
**       a deployment then the definition of the FILEMGR_PLATFORM_REV
**       macro in filemgr_platform_cfg.h should be updated.
**
*/

#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "file_xfer_eds_typedefs.h"
#include "file_xfer_eds_designparameters.h"

#include "file_xfer_platform_cfg.h"
#include "app_c_fw.h"

/******************************************************************************
**
** Versions:
**
** 1.0 - Initial release
** 2.0 - Updated to cFS 7.0 (Basecamp 4.0 release)
*/

#define  FILE_XFER_MAJOR_VER   2
#define  FILE_XFER_MINOR_VER   0


/******************************************************************************
** JSON init file definitions/declarations.
**    
*/

#define CFG_APP_CFE_NAME        APP_CFE_NAME
#define CFG_APP_PERF_ID         APP_PERF_ID

#define CFG_APP_CMD_PIPE_NAME   APP_CMD_PIPE_NAME
#define CFG_APP_CMD_PIPE_DEPTH  APP_CMD_PIPE_DEPTH

/*
** KIT_TO creates a semaphore that is used to control the flow
** of file data segments from FILE_XFER -> KIT_TO
** - The order of app startup is not guaranteed so FOTP uses
**   a loop to repeated try to get KIT_TO's semaphore IDs
** - LOOP_DELAY is in milliseconds
** - It's okay to conservatively define the loop parameters
**   becuase starting a file transfer immediately after intialization
**   is typically not a high priority
*/ 
#define CFG_KIT_TO_SEM_NAME            KIT_TO_SEM_NAME
#define CFG_KIT_TO_SEM_LOOP_ATTEMPTS   KIT_TO_SEM_LOOP_ATTEMPTS
#define CFG_KIT_TO_SEM_LOOP_DELAY      KIT_TO_SEM_LOOP_DELAY


#define CFG_FILE_XFER_CMD_TOPICID      FILE_XFER_CMD_TOPICID
#define CFG_FILE_XFER_SEND_HK_TOPICID  BC_SCH_2_SEC_TOPICID
#define CFG_FILE_XFER_EXE_TOPICID      BC_SCH_500_MS_TOPICID

#define CFG_FILE_XFER_HK_TLM_TOPICID                 FILE_XFER_HK_TLM_TOPICID
#define CFG_FILE_XFER_START_FOTP_TLM_TOPICID         FILE_XFER_START_FOTP_TLM_TOPICID
#define CFG_FILE_XFER_FOTP_DATA_SEGMENT_TLM_TOPICID  FILE_XFER_FOTP_DATA_SEGMENT_TLM_TOPICID
#define CFG_FILE_XFER_FINISH_FOTP_TLM_TOPICID        FILE_XFER_FINISH_FOTP_TLM_TOPICID
 


#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_PERF_ID,uint32) \
   XX(APP_CMD_PIPE_NAME,char*) \
   XX(APP_CMD_PIPE_DEPTH,uint32) \
   XX(KIT_TO_SEM_NAME,char*) \
   XX(KIT_TO_SEM_LOOP_ATTEMPTS,uint32) \
   XX(KIT_TO_SEM_LOOP_DELAY,uint32) \
   XX(FILE_XFER_CMD_TOPICID,uint32) \
   XX(BC_SCH_2_SEC_TOPICID,uint32) \
   XX(BC_SCH_500_MS_TOPICID,uint32) \
   XX(FILE_XFER_HK_TLM_TOPICID,uint32) \
   XX(FILE_XFER_START_FOTP_TLM_TOPICID,uint32) \
   XX(FILE_XFER_FOTP_DATA_SEGMENT_TLM_TOPICID,uint32) \
   XX(FILE_XFER_FINISH_FOTP_TLM_TOPICID,uint32) \

DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** App level definitions that don't need to be in the ini file
**
*/

#define FILE_XFER_UNDEF_TLM_STR "Undefined"


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define FILE_XFER_APP_BASE_EID  (EdsParam_APP_C_FW_APP_BASE_EID +  0)
#define FITP_BASE_EID           (EdsParam_APP_C_FW_APP_BASE_EID + 20)
#define FOTP_BASE_EID           (EdsParam_APP_C_FW_APP_BASE_EID + 40)


/******************************************************************************
** File Input Transfer Protocol (FITP)
*/

#define FITP_FILENAME_LEN       (OS_MAX_PATH_LEN)
#define FITP_DATA_SEG_ID_NULL      0
#define FITP_DATA_SEG_ID_START     1 


/******************************************************************************
** File Output Transfer Protocol (FOTP)
**
** FOTP_DATA_SEGMENT_PER_EXEC defines the number data segments that are sent
** in a single FILE_XFER execution cycle. In normal operations the flow control
** semaphore created by KIT_TO should be used. This parameter is only used
** when the flow control semaphore is not being used (e.g. development).
*/

#define FOTP_FILENAME_LEN           (OS_MAX_PATH_LEN)
#define FOTP_DATA_SEGMENT_ID_START  1 
#define FOTP_DATA_SEGMENT_PER_EXEC  4 

#endif /* _app_cfg_ */
