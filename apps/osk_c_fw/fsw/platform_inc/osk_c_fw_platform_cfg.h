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
**    Define the platform configurations for the OSK application framework
**
**  Notes:
**    1. Resources are statically allocated for each application's
**       data structures so these configurations must be sized to 
**       accommodate the 'largest' application.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _osk_c_fw_platform_cfg_h_
#define _osk_c_fw_platform_cfg_h_

#include "osk_c_fw_mission_cfg.h"

/* 
** Mission specific version number
**
**  An application version number consists of four parts:
**  major version number, minor version number, revision
**  number and mission specific revision number. The mission
**  specific revision number is defined here and the other
**  parts are defined in "osk_c_fw_ver.h".
**
*/
#define OSK_C_FW_MISSION_REV  (0)

/******************************************************************************
** OSK Framework Error Codes
**
** TODO - This needs to be expanded and moved to framework utility
*/

#define  OSK_C_FW_ERROR      (0x0001)
#define  OSK_C_FW_CFS_ERROR  ((int32)(CFE_SEVERITY_ERROR | OSK_C_FW_ERROR))

/******************************************************************************
** Initialization Table (INITBL)
**
*/

#define  INITBL_MAX_CFG_ITEMS         32   /* Max number of JSON ini file config items */
#define  INITBL_MAX_CFG_STR_LEN       64   /* This is INITTBL's storage max. A config parameter such as a filename may have more restrictive length constraints */ 
#define  INITBL_MAX_JSON_FILE_CHAR  8192   /* Max number of JSON file characters       */

/******************************************************************************
** Table Manager (TBLMGR)
*/

#define TBLMGR_MAX_TBL_PER_APP  5


/******************************************************************************
** Child Manager (CHILDMGR)
**
** Define child manager object parameters. The command buffer length must big
** enough to hold the largest command packet of all the apps using this utility.
*/

#define CHILDMGR_MAX_TASKS  5   /* Max number of child tasks registered for all apps */


#define CHILDMGR_CMD_PAYLOAD_LEN  256   /* Must be greater than largest cmd msg */ 
#define CHILDMGR_CMD_Q_ENTRIES      3   
#define CHILDMGR_CMD_FUNC_TOTAL    32

/******************************************************************************
** State Reporter (STATEREP)
*/

#define STATEREP_BIT_ID_MAX  32


#endif /* _osk_c_fw_platform_cfg_h_ */
