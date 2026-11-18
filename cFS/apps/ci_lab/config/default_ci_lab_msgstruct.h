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
 *   Specification for the CI_LAB command and telemetry
 *   message data types.
 *
 * @note
 *   Constants and enumerated types related to these message structures
 *   are defined in ci_lab_msgdefs.h.
 */
#ifndef DEFAULT_CI_LAB_MSGSTRUCT_H
#define DEFAULT_CI_LAB_MSGSTRUCT_H

/************************************************************************
 * Includes
 ************************************************************************/

#include "ci_lab_mission_cfg.h"
#include "ci_lab_msgdefs.h"
#include "cfe_msg_hdr.h"

/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/

/*
 * Neither the Noop nor ResetCounters command
 * have any payload, but should still "reserve" a unique
 * structure type to employ a consistent handler pattern.
 *
 * This matches the pattern in CFE core and other modules.
 */
typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader;
} CI_LAB_NoopCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader;
} CI_LAB_ResetCountersCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader;
} CI_LAB_SendHkCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader;
} CI_LAB_ReadUplinkCmd_t;

typedef struct
{
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    CI_LAB_HkTlm_Payload_t    Payload;
} CI_LAB_HkTlm_t;

#endif /* CI_LAB_MSGSTRUCT_H */
