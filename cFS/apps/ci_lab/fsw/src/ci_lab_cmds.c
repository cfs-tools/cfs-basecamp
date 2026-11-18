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

/*
**   Include Files:
*/

#include "ci_lab_app.h"
#include "ci_lab_cmds.h"
#include "ci_lab_version.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                             */
/*  Purpose:                                                                   */
/*     Handle NOOP command packets                                             */
/*                                                                             */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t CI_LAB_NoopCmd(const CI_LAB_NoopCmd_t *cmd)
{
    /* Does everything the name implies */
    CI_LAB_Global.HkTlm.Payload.CommandCounter++;

    CFE_EVS_SendEvent(CI_LAB_NOOP_INF_EID,
                      CFE_EVS_EventType_INFORMATION,
                      "CI: NOOP command. Version %d.%d.%d.%d",
                      CI_LAB_MAJOR_VERSION,
                      CI_LAB_MINOR_VERSION,
                      CI_LAB_REVISION,
                      CI_LAB_MISSION_REV);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                             */
/*  Purpose:                                                                   */
/*     Handle ResetCounters command packets                                    */
/*                                                                             */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t CI_LAB_ResetCountersCmd(const CI_LAB_ResetCountersCmd_t *cmd)
{
    CFE_EVS_SendEvent(CI_LAB_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "CI: RESET command");
    CI_LAB_ResetCounters_Internal();
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the CI task  */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t CI_LAB_SendHkCmd(const CI_LAB_SendHkCmd_t *cmd)
{
    CI_LAB_Global.HkTlm.Payload.SocketConnected = CI_LAB_Global.SocketConnected;
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(CI_LAB_Global.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(CI_LAB_Global.HkTlm.TelemetryHeader), true);
    return CFE_SUCCESS;
}

CFE_Status_t CI_LAB_ReadUplinkCmd(const CI_LAB_ReadUplinkCmd_t *cmd)
{
    /* Any occurrence of this request will cause CI to read ONLY on this request thereafter */
    CI_LAB_Global.Scheduled = true;
    CI_LAB_ReadUpLink();
    return CFE_SUCCESS;
}