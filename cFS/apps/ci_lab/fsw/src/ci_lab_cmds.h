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
 * \file
 *   This file contains the command handler functions for the Command Ingest task.
 */
#ifndef CI_LAB_CMDS_H
#define CI_LAB_CMDS_H

#include "common_types.h"
#include "cfe_error.h"
#include "ci_lab_msg.h"

/**
 * @brief Handle NOOP command packets
 *
 * @param cmd Input message pointer
 * @returns CFE Status code
 * @retval #CFE_SUCCESS on successful processing
 */
CFE_Status_t CI_LAB_NoopCmd(const CI_LAB_NoopCmd_t *cmd);

/**
 * @brief Handle ResetCounters command packets
 *
 * @param cmd Input message pointer
 * @returns CFE Status code
 * @retval #CFE_SUCCESS on successful processing
 */
CFE_Status_t CI_LAB_ResetCountersCmd(const CI_LAB_ResetCountersCmd_t *cmd);

/**
 * @brief Handle Send HK command packets
 *
 * This function is triggered in response to a task telemetry request
 * from the housekeeping task. This function will gather the CI task
 * telemetry, packetize it and send it to the housekeeping task via
 * the software bus
 *
 * @param cmd Input message pointer
 * @returns CFE Status code
 * @retval #CFE_SUCCESS on successful processing
 */
CFE_Status_t CI_LAB_SendHkCmd(const CI_LAB_SendHkCmd_t *cmd);

/**
 * @brief Read Uplink command packets
 *
 * This allows servicing of the CI_LAB uplink to be scheduled in a designated timeslot.
 *
 * For backward compatibility, CI_LAB will service the uplink periodically by default,
 * using a local timeout.  However, if this message is sent by the system scheduler,
 * CI_LAB will stop servicing based on the timeout and ONLY based on that command.
 *
 * @param cmd Input message pointer
 * @returns CFE Status code
 * @retval #CFE_SUCCESS on successful processing
 */
CFE_Status_t CI_LAB_ReadUplinkCmd(const CI_LAB_ReadUplinkCmd_t *cmd);

#endif
