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
 *   Specification for the @TEMPLATE@ command and telemetry
 *   message payload and constant definitions.
 */
#ifndef DEFAULT_@TEMPLATE@_MSGDEFS_H
#define DEFAULT_@TEMPLATE@_MSGDEFS_H

#include "common_types.h"
#include "@template@_fcncodes.h"

typedef struct @TEMPLATE@_ExampleParam_Payload
{
    int16  ValI16;                                    /**< 16 bit signed integer value */
} @TEMPLATE@_ExampleParam_Payload_t;

/*************************************************************************/
/*
** Type definition (@TEMPLATE@ App housekeeping)
*/

typedef struct @TEMPLATE@_HkTlm_Payload
{
    uint8 CommandCounter;
    uint8 CommandErrorCounter;
    int16 ExampleCmdParam;
    
} @TEMPLATE@_HkTlm_Payload_t;

#endif
