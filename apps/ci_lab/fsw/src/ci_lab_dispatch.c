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
#include "ci_lab_events.h"

#include "ci_lab_eds_dictionary.h"
#include "ci_lab_eds_dispatcher.h"

/*
 * Define a lookup table for CI lab command codes
 */
static const CI_LAB_Application_Component_Telecommand_DispatchTable_t CI_LAB_TC_DISPATCH_TABLE = {
    .CMD =
        {
            .NoopCmd_indication          = CI_LAB_Noop,
            .ResetCountersCmd_indication = CI_LAB_ResetCounters,

        },
    .SEND_HK = {.indication = CI_LAB_ReportHousekeeping}};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  CI_LAB_ProcessCommandPacket                                        */
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
void CI_LAB_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId;
    CFE_Status_t   Status;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    Status = CI_LAB_Application_Component_Telecommand_Dispatch(CFE_SB_Telecommand_indication_Command_ID, SBBufPtr,
                                                               &CI_LAB_TC_DISPATCH_TABLE);

    if (Status != CFE_SUCCESS)
    {
        CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
        CI_LAB_Global.HkTlm.Payload.CommandErrorCounter++;
        CFE_EVS_SendEvent(CI_LAB_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR, "CI: invalid command packet,MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }

} /* End CI_LAB_ProcessCommandPacket */
