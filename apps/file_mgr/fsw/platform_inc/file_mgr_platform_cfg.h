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
**    Define platform scope configurations for the File Manager application
**
**  Notes:
**    1. This is part of a refactoring prototype.
**    2. These definitions should be minimal and only contain parameters that
**       need to be configurable and that must be defined at compile time.  
**       Use app_cfg.h for compile-time parameters that don't need to be
**       configured when an app is deployed and the JSON initialization
**       file for parameters that can be defined at runtime.
**
*/

#ifndef _file_mgr_platform_cfg_
#define _file_mgr_platform_cfg_

/*
** Includes
*/

#include "file_mgr_mission_cfg.h"

/******************************************************************************
** Platform Deployment Configurations
*/

#define FILE_MGR_PLATFORM_REV   0
#define FILE_MGR_INI_FILENAME   "/cf/file_mgr_ini.json"


/******************************************************************************
** These are frustrating. They're only needed statically because of the table
** decsriptor build process. 
*/

#define FILE_MGR_APP_CFE_NAME   "FILE_MGR"
#define FILE_MGR_TBL_CFE_NAME   "FileSysTbl"

#endif /* _file_mgr_platform_cfg_ */
