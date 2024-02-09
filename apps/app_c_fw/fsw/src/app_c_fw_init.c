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
**    Entry point function for Application C framework library
**
**  Notes:
**    None
**
*/

/*
** Includes
*/

#include "app_c_fw_cfg.h"
#include "app_c_fw_ver.h"

/*
** Exported Functions
*/

/******************************************************************************
** Entry function
**
*/
uint32 APP_C_FW_LibInit(void)
{

   OS_printf("Application C Framework Library Initialized. Version %d.%d.%d\n",
             APP_C_FW_MAJOR_VER, APP_C_FW_MINOR_VER, APP_C_FW_LOCAL_REV);
   
   return OS_SUCCESS;

} /* End APP_C_FW_LibInit() */

