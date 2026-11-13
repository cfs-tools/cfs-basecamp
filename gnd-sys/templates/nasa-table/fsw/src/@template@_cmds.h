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
 *   This file contains the prototypes for the @TEMPLATE@ Ground Command-handling functions
 */

#ifndef @TEMPLATE@_CMDS_H
#define @TEMPLATE@_CMDS_H

/*
** Required header files.
*/
#include "cfe_error.h"
#include "@template@_msg.h"

CFE_Status_t @TEMPLATE@_SendHkCmd(const @TEMPLATE@_SendHkCmd_t *Msg);
CFE_Status_t @TEMPLATE@_NoopCmd(const @TEMPLATE@_NoopCmd_t *Msg);
CFE_Status_t @TEMPLATE@_ResetCountersCmd(const @TEMPLATE@_ResetCountersCmd_t *Msg);
CFE_Status_t @TEMPLATE@_ExampleParamCmd(const @TEMPLATE@_ExampleParamCmd_t *Msg);

#endif /* @TEMPLATE@_CMDS_H */
