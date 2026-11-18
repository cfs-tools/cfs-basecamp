/*******************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: ci_lab_app.c
**
** Purpose:
**   This file contains the source code for the Command Ingest task.
**
*******************************************************************************/

/*
**   Include Files:
*/

#include "ci_lab_app.h"
#include "ci_lab_eventids.h"
#include "ci_lab_dispatch.h"
#include "ci_lab_cmds.h"

#include "ci_lab_eds_dictionary.h"
#include "ci_lab_eds_dispatcher.h"

/*
 * Define a lookup table for CI lab command codes
 */
/* clang-format off */
static const EdsDispatchTable_EdsComponent_CI_LAB_Application_CFE_SB_Telecommand_t CI_LAB_TC_DISPATCH_TABLE = 
{
    .CMD =
    {
        .NoopCmd_indication          = CI_LAB_NoopCmd,
        .ResetCountersCmd_indication = CI_LAB_ResetCountersCmd,
    },
    .SEND_HK =
    {
        .indication = CI_LAB_SendHkCmd
    },
    .READ_UPLINK =
    {
        .indication = CI_LAB_ReadUplinkCmd
    }
};
/* clang-format on */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  CI_LAB_TaskPipe                                        */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the CI command*/
/*     pipe. The packets received on the CI command pipe are listed here:     */
/*                                                                            */
/*        1. NOOP command (from ground)                                       */
/*        2. Request to reset telemetry counters (from ground)                */
/*        3. Request for housekeeping telemetry packet (from HS task)         */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void CI_LAB_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId;
    CFE_Status_t   Status;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    Status = EdsDispatch_EdsComponent_CI_LAB_Application_Telecommand(SBBufPtr, &CI_LAB_TC_DISPATCH_TABLE);

    if (Status != CFE_SUCCESS)
    {
        CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
        CI_LAB_Global.HkTlm.Payload.CommandErrorCounter++;
        CFE_EVS_SendEvent(CI_LAB_MID_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "CI: invalid command packet,MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }

} /* End CI_LAB_TaskPipe */
