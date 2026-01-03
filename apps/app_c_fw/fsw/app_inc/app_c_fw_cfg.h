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
**    Define configurations for the application framework
**
**  Notes:
**    1. These definitions are intrinsic to the framework and should not
**       change across platform deployments. They should be fixed between
**       framework releases but may need to can during framework development
**       and for a release. See app_c_fw_platform.h and app_c_fw_mission.h
**       for parameters that can be configured for a deployment.
**    TODO:  Decide which macros should be moved to EDS
**
*/

#ifndef _app_c_fw_cfg_
#define _app_c_fw_cfg_

/*
** Includes
*/

#include "cfe.h"
#include "app_c_fw_platform_cfg.h"
#include "app_c_fw_eds_typedefs.h"
#include "app_c_fw_eds_defines.h"

//TODO: Remove this once the EDS issue in app_c_fw.xml is resolved
#if APP_C_FW_CRC_16 != CFE_MISSION_ES_CRC_16
#error APP_C_FW_CrcUint8_CRC_16 definition must match CFE_MISSION_ES_CRC_16
#endif

/******************************************************************************
** Command Manager (CMDMGR)
*/

/*
** Defines maximum number of command function codes (FC) an app can have. FCs
** must start at zero. The standard codes range from 0..9 so the number of 
** app-specific commands is CMDMGR_CMD_FUNC_TOTAL-10.
** 
** Note EDS duplicates the standard app FC definitions. The definiitons are 
** here for backwards compatibility with apps that don't use EDS.
*/

#define CMDMGR_CMD_FUNC_TOTAL  32

/* Standard function definitions for every app */

#define CMDMGR_NOOP_CMD_FC      0  
#define CMDMGR_RESET_CMD_FC     1
#define CMDMGR_LOAD_TBL_CMD_FC  2
#define CMDMGR_DUMP_TBL_CMD_FC  3
#define CMDMGR_APP_START_FC    10  /* First FC available for app */

/* 
** Uncomment CMDMGR_DISABLE_CHECKSUM_VALIDATION to disable checksum
** verification. It can be useful to disable checksum verification in
** situations when commands are being sent from a test environment without
** checksum capability.
** 
*/
#define CMDMGR_DISABLE_CHECKSUM_VALIDATION


/******************************************************************************
** Debug macros
**
** Set debug macros to 1 to enable debug message outputs to the console
*/


#define DBG_INITBL      0
#define DBG_CMDMGR      0
#define DBG_TBLMGR      0
#define DBG_JSON        0
#define DBG_FAULTREP    0
#define DBG_CHILDMGR    0


#endif /* _app_c_fw_cfg_ */
