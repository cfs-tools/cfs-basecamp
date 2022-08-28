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
**    Define platform configurations for the @Template@ application
**
**  Notes:
**   1. Compile-time configurations that are applied to each platform
**      deployment of this app.
**   2. These definitions should minimal and only contain parameters that
**      need to be configurable at compile time that allows users to tuned
**      the app for a particular platform. Use @template@_mission_cfg.h
**      for compile-time parameters that need to be tuned at the mission
**      level across all platform deployments. Use app_cfg.h for compile-time
**      parameters that don't need to be configured when an app is deployed
**      but are useful to be parameterized for maintaining the app and use
**      the JSON initialization file for parameters that can be defined at
**      runtime.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/
#ifndef _@template@_platform_cfg_
#define _@template@_platform_cfg_


/*
** Includes
*/

#include "@template@_mission_cfg.h"


/*
** Platform Deployment Configurations
*/

#define @TEMPLATE@_PLATFORM_REV   0
#define @TEMPLATE@_INI_FILENAME   "/cf/@template@_ini.json"


#endif /* _@template@_platform_cfg_ */
