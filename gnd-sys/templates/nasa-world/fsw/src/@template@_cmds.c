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
 *   This file contains the source code for the @TEMPLATE@ App Ground Command-handling functions
 */

/*
** Include Files:
*/
#include "@template@_app.h"
#include "@template@_cmds.h"
#include "@template@_msgids.h"
#include "@template@_eventids.h"
#include "@template@_version.h"
#include "@template@_utils.h"
#include "@template@_msg.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t @TEMPLATE@_SendHkCmd(const @TEMPLATE@_SendHkCmd_t *Msg)
{

    /*
    ** Get command execution counters...
    */
    @TEMPLATE@_Data.HkTlm.Payload.CommandErrorCounter = @TEMPLATE@_Data.CommandErrorCounter;
    @TEMPLATE@_Data.HkTlm.Payload.CommandCounter      = @TEMPLATE@_Data.CommandCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(@TEMPLATE@_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(@TEMPLATE@_Data.HkTlm.TelemetryHeader), true);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* @TEMPLATE@ NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t @TEMPLATE@_NoopCmd(const @TEMPLATE@_NoopCmd_t *Msg)
{
    @TEMPLATE@_Data.CommandCounter++;

    CFE_EVS_SendEvent(@TEMPLATE@_NOOP_INF_EID,
                      CFE_EVS_EventType_INFORMATION,
                      "@TEMPLATE@ App: NOOP command %s",
                      @TEMPLATE@_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t @TEMPLATE@_ResetCountersCmd(const @TEMPLATE@_ResetCountersCmd_t *Msg)
{
    @TEMPLATE@_Data.CommandCounter      = 0;
    @TEMPLATE@_Data.CommandErrorCounter = 0;

    CFE_EVS_SendEvent(@TEMPLATE@_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "@TEMPLATE@: RESET command");

    return CFE_SUCCESS;
}

