/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *
 * @TEMPLATE@ Application Platform Configuration Header File
 *
 * This is a compatibility header for the "platform_cfg.h" file that has
 * traditionally provided both public and private config definitions
 * for each CFS app.
 *
 * These definitions are now provided in two separate files, one for
 * the public/mission scope and one for internal scope.
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef @TEMPLATE@_INTERNAL_CFG_H
#define @TEMPLATE@_INTERNAL_CFG_H

#include "@template@_internal_cfg_values.h"

/***********************************************************************/
#define @TEMPLATE@_PLATFORM_PIPE_DEPTH         @TEMPLATE@_PLATFORM_CFGVAL(PIPE_DEPTH)
#define DEFAULT_@TEMPLATE@_PLATFORM_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define @TEMPLATE@_PLATFORM_PIPE_NAME         @TEMPLATE@_PLATFORM_CFGVAL(PIPE_NAME)
#define DEFAULT_@TEMPLATE@_PLATFORM_PIPE_NAME "@TEMPLATE@_CMD_PIPE"

#define @TEMPLATE@_PLATFORM_TABLE_FILE         @TEMPLATE@_PLATFORM_CFGVAL(TABLE_FILE)
#define DEFAULT_@TEMPLATE@_PLATFORM_TABLE_FILE "/cf/@template@_tbl.tbl"
//EX1,1,
#define @TEMPLATE@_PLATFORM_TABLE_OUT_OF_RANGE_ERR_CODE         @TEMPLATE@_PLATFORM_CFGVAL(TABLE_OUT_OF_RANGE_ERR_CODE)
#define DEFAULT_@TEMPLATE@_PLATFORM_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define @TEMPLATE@_PLATFORM_TBL_ELEMENT_1_MAX         @TEMPLATE@_PLATFORM_CFGVAL(TBL_ELEMENT_1_MAX)
#define DEFAULT_@TEMPLATE@_PLATFORM_TBL_ELEMENT_1_MAX 10
//EX1
#endif
