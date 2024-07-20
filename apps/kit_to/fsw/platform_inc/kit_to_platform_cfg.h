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
**    Define platform configurations for the KIT_CI application
**
**  Notes:
**    1. These definitions should be minimal and only contain parameters that
**       need to be configurable and that must be defined at compile time.  
**       Use app_cfg.h for compile-time parameters that don't need to be
**       configured when an app is deployed and the JSON initialization
**       file for parameters that can be defined at runtime.
**
*/
#ifndef _kit_to_platform_cfg_
#define _kit_to_platform_cfg_

/*
** Includes
*/

#include "kit_to_mission_cfg.h"


/******************************************************************************
** Platform Deployment Configurations
*/

#define KIT_TO_PLATFORM_REV   0
#define KIT_TO_INI_FILENAME   "/cf/kit_to_ini.json"

/******************************************************************************
** Packet Table Configurations
*/

#define PKTTBL_JSON_FILE_MAX_CHAR  50000


#endif /* _kit_to_platform_cfg_ */
