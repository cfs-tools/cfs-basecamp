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

#include "@template@_eds_dispatcher.h"
#include "@template@_eds_dictionary.h"

/*
 * Define a lookup table for @TEMPLATE@ app command codes
 */
/* clang-format off */
static const EdsDispatchTable_EdsComponent_@TEMPLATE@_Application_CFE_SB_Telecommand_t @TEMPLATE@_TC_DISPATCH_TABLE =
{
    .CMD =
    {
        .NoopCmd_indication          = @TEMPLATE@_NoopCmd,
        .ResetCountersCmd_indication = @TEMPLATE@_ResetCountersCmd
    },
    .SEND_HK =
    {
        .indication = @TEMPLATE@_SendHkCmd
    }
};
/* clang-format on */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the           */
/*     @TEMPLATE@ command pipe.                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void @TEMPLATE@_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_Status_t      Status;
    CFE_SB_MsgId_t    MsgId;
    CFE_MSG_Size_t    MsgSize;
    CFE_MSG_FcnCode_t MsgFc;

    Status = EdsDispatch_EdsComponent_@TEMPLATE@_Application_Telecommand(SBBufPtr, &@TEMPLATE@_TC_DISPATCH_TABLE);

    if (Status != CFE_SUCCESS)
    {
        CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
        CFE_MSG_GetSize(&SBBufPtr->Msg, &MsgSize);
        CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &MsgFc);
        ++@TEMPLATE@_Data.CommandErrorCounter;

        if (Status == CFE_STATUS_UNKNOWN_MSG_ID)
        {
            CFE_EVS_SendEvent(@TEMPLATE@_MID_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@ App: invalid command packet,MID = 0x%x",
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId));
        }
        else if (Status == CFE_STATUS_WRONG_MSG_LENGTH)
        {
            CFE_EVS_SendEvent(@TEMPLATE@_CMD_LEN_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u",
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                              (unsigned int)MsgFc,
                              (unsigned int)MsgSize);
        }
        else
        {
            CFE_EVS_SendEvent(@TEMPLATE@_CC_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@ App: Invalid ground command code: CC = %d",
                              (int)MsgFc);
        }
    }
}
