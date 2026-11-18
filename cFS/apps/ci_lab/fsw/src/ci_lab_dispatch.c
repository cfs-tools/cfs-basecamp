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
 *   This file contains the command dispatch logic for the CI_LAB app
 */

#include "cfe.h"
#include "ci_lab_app.h"
#include "ci_lab_dispatch.h"
#include "ci_lab_cmds.h"
#include "ci_lab_msgids.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool CI_LAB_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_MSG_FcnCode_t FcnCode      = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length...
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(CI_LAB_CMD_LEN_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "Invalid msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                          (unsigned int)FcnCode,
                          (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);
        result = false;
        CI_LAB_Global.HkTlm.Payload.CommandErrorCounter++;
    }

    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* CI ground commands                                                         */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void CI_LAB_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t FcnCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &FcnCode);

    /* Process "known" CI task ground commands */
    switch (FcnCode)
    {
        case CI_LAB_NOOP_CC:
            if (CI_LAB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CI_LAB_NoopCmd_t)))
            {
                CI_LAB_NoopCmd((const CI_LAB_NoopCmd_t *)SBBufPtr);
            }
            break;

        case CI_LAB_RESET_COUNTERS_CC:
            if (CI_LAB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CI_LAB_ResetCountersCmd_t)))
            {
                CI_LAB_ResetCountersCmd((const CI_LAB_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        default:
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
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
    static CFE_SB_MsgId_t CMD_MID         = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t SEND_HK_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t READ_UPLINK_MID = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID         = CFE_SB_ValueToMsgId(CI_LAB_CMD_MID);
        SEND_HK_MID     = CFE_SB_ValueToMsgId(CI_LAB_SEND_HK_MID);
        READ_UPLINK_MID = CFE_SB_ValueToMsgId(CI_LAB_READ_UPLINK_MID);
    }

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    /* Process all SB messages */
    if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    {
        /* Housekeeping request */
        CI_LAB_SendHkCmd((const CI_LAB_SendHkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, READ_UPLINK_MID))
    {
        /* Scheduler command */
        CI_LAB_ReadUplinkCmd((const CI_LAB_ReadUplinkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        /* Ground command */
        CI_LAB_ProcessGroundCommand(SBBufPtr);
    }
    else
    {
        /* Unknown command */
        CI_LAB_Global.HkTlm.Payload.CommandErrorCounter++;
        CFE_EVS_SendEvent(CI_LAB_MID_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "CI: invalid command packet,MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}
