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
**    Define platform configurations for the OpenSatKit File Transfer application
**
**  Notes:
**    1. These definitions should be minimal and only contain parameters that
**       need to be configurable and that must be defined at compile time.  
**       Use app_cfg.h for compile-time parameters that don't need to be
**       configured when an app is deployed and the JSON initialization
**       file for parameters that can be defined at runtime.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _file_xfer_platform_cfg_
#define _file_xfer_platform_cfg_

/*
** Includes
*/

#include "file_xfer_mission_cfg.h"


/******************************************************************************
** Platform Deployment Configurations
*/

#define FILE_XFER_PLATFORM_REV   0
#define FILE_XFER_INI_FILENAME   "/cf/file_xfer_ini.json"


#endif /* _file_xfer_platform_cfg_ */
