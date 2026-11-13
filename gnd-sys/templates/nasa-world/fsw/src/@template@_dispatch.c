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
 *   This file contains the source code for the @TEMPLATE@ App.
 */

/*
** Include Files:
*/
#include "@template@_app.h"
#include "@template@_dispatch.h"
#include "@template@_cmds.h"
#include "@template@_eventids.h"
#include "@template@_msgids.h"
#include "@template@_msg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool @TEMPLATE@_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(@TEMPLATE@_CMD_LEN_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                          (unsigned int)FcnCode,
                          (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        @TEMPLATE@_Data.CommandErrorCounter++;
    }

    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* @TEMPLATE@ ground commands                                                 */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void @TEMPLATE@_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process @TEMPLATE@ app ground commands
    */
    switch (CommandCode)
    {
        case @TEMPLATE@_NOOP_CC:
            if (@TEMPLATE@_VerifyCmdLength(&SBBufPtr->Msg, sizeof(@TEMPLATE@_NoopCmd_t)))
            {
                @TEMPLATE@_NoopCmd((const @TEMPLATE@_NoopCmd_t *)SBBufPtr);
            }
            break;

        case @TEMPLATE@_RESET_COUNTERS_CC:
            if (@TEMPLATE@_VerifyCmdLength(&SBBufPtr->Msg, sizeof(@TEMPLATE@_ResetCountersCmd_t)))
            {
                @TEMPLATE@_ResetCountersCmd((const @TEMPLATE@_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case @TEMPLATE@_EXAMPLE_PARAM_CC:
            if (@TEMPLATE@_VerifyCmdLength(&SBBufPtr->Msg, sizeof(@TEMPLATE@_ExampleParamCmd_t)))
            {
                @TEMPLATE@_ExampleParamCmd((const @TEMPLATE@_DisplayParamCmd_t *)SBBufPtr);
            }
            break;

        default:
            CFE_EVS_SendEvent(@TEMPLATE@_CC_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "Invalid ground command code: CC = %d",
                              CommandCode);
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the           */
/*     @TEMPLATE@ command pipe.                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void @TEMPLATE@_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID     = CFE_SB_ValueToMsgId(@TEMPLATE@_CMD_MID);
        SEND_HK_MID = CFE_SB_ValueToMsgId(@TEMPLATE@_SEND_HK_MID);
    }

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    /* Process all SB messages */
    if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    {
        /* Housekeeping request */
        @TEMPLATE@_SendHkCmd((const @TEMPLATE@_SendHkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        /* Ground command */
        @TEMPLATE@_ProcessGroundCommand(SBBufPtr);
    }
    else
    {
        /* Unknown command */
        CFE_EVS_SendEvent(@TEMPLATE@_MID_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "@TEMPLATE@ App: invalid command packet,MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}
