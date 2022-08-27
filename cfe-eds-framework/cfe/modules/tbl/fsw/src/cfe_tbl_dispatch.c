/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#include "cfe_tbl_module_all.h"
#include "cfe_tbl_eds_dictionary.h"
#include "cfe_tbl_eds_dispatcher.h"
#include "cfe_msg.h"

/*
** Table task const data
*/
/*
 * Define a lookup table for TBL command codes
 */
static const CFE_TBL_Application_Component_Telecommand_DispatchTable_t CFE_TBL_TC_DISPATCH_TABLE = {
    .CMD     = {.AbortLoadCmd_indication     = CFE_TBL_AbortLoadCmd,
            .ActivateCmd_indication      = CFE_TBL_ActivateCmd,
            .DeleteCDSCmd_indication     = CFE_TBL_DeleteCDSCmd,
            .DumpCmd_indication          = CFE_TBL_DumpCmd,
            .DumpRegistryCmd_indication  = CFE_TBL_DumpRegistryCmd,
            .LoadCmd_indication          = CFE_TBL_LoadCmd,
            .NoopCmd_indication          = CFE_TBL_NoopCmd,
            .ResetCountersCmd_indication = CFE_TBL_ResetCountersCmd,
            .SendRegistryCmd_indication  = CFE_TBL_SendRegistryCmd,
            .ValidateCmd_indication      = CFE_TBL_ValidateCmd},
    .SEND_HK = {.indication = CFE_TBL_HousekeepingCmd}

};

/*----------------------------------------------------------------
 *
 * Function: CFE_TBL_TaskPipe
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void CFE_TBL_TaskPipe(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_Status_t      Status;
    CFE_SB_MsgId_t    MsgId;
    CFE_MSG_Size_t    MsgSize;
    CFE_MSG_FcnCode_t MsgFc;

    Status = CFE_TBL_Application_Component_Telecommand_Dispatch(CFE_SB_Telecommand_indication_Command_ID, SBBufPtr,
                                                                &CFE_TBL_TC_DISPATCH_TABLE);

    /* These specific status codes require sending an event with the details */
    if (Status == CFE_STATUS_BAD_COMMAND_CODE || Status == CFE_STATUS_WRONG_MSG_LENGTH ||
        Status == CFE_STATUS_UNKNOWN_MSG_ID || Status == CFE_TBL_NOT_IMPLEMENTED)
    {
        CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
        CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &MsgFc);
        CFE_MSG_GetSize(&SBBufPtr->Msg, &MsgSize);

        if (Status == CFE_TBL_NOT_IMPLEMENTED || Status == CFE_STATUS_BAD_COMMAND_CODE)
        {
            CFE_EVS_SendEvent(CFE_TBL_CC1_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid command code -- ID = 0x%04X, CC = %d", (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                              (int)MsgFc);
        }
        else if (Status == CFE_STATUS_WRONG_MSG_LENGTH)
        {
            CFE_EVS_SendEvent(CFE_TBL_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid msg length -- ID = 0x%04X, CC = %d, Len = %d",
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId), (int)MsgFc, (int)MsgSize);
        }
        else
        {
            CFE_EVS_SendEvent(CFE_TBL_MID_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid message ID -- ID = 0x%04X",
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId));
        }
    }

    if (Status == CFE_TBL_INC_CMD_CTR)
    {
        CFE_TBL_Global.CommandCounter++;
    }
    else if (Status < 0 && Status != CFE_STATUS_UNKNOWN_MSG_ID)
    {
        /*
         * Any other code in the error domain should increment the error count.
         *
         * Note that CFE_TBL_DONT_INC_CTR is an informational code
         * and therefore in the positive range, so it will avoid this
         * increment.
         */
        CFE_TBL_Global.CommandErrorCounter++;
    }

} /* End of CFE_TBL_TaskPipe() */
