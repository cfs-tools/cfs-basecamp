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
 *   Specification for the CI_LAB command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef CI_LAB_FCNCODES_H
#define CI_LAB_FCNCODES_H

#include "ci_lab_fcncode_values.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** CI_LAB command codes
*/
#define CI_LAB_NOOP_CC           CI_LAB_CCVAL(NOOP)
#define CI_LAB_RESET_COUNTERS_CC CI_LAB_CCVAL(RESET_COUNTERS)

#endif
