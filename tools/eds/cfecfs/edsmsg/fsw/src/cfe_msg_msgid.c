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

/******************************************************************************
 *  Message id access functions, shared cFS implementation
 */
#include "cfe_platform_cfg.h"
#include "cfe_msg.h"
#include "cfe_sb.h"
#include "cfe_error.h"

#include "ccsds_spacepacket_eds_typedefs.h"
#include "cfe_sb_eds_typedefs.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_missionlib_runtime.h"

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetTypeFromMsgId
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetTypeFromMsgId(CFE_SB_MsgId_t MsgId, CFE_MSG_Type_t *Type)
{
    CFE_Status_t                          Status;
    CFE_SB_SoftwareBus_PubSub_Interface_t Params;

    if (Type == NULL)
    {
        Status = CFE_MSG_BAD_ARGUMENT;
    }
    else
    {
        Params.MsgId = MsgId;
        if (CFE_MissionLib_PubSub_IsListenerComponent(&Params))
        {
            *Type  = CFE_MSG_Type_Cmd;
            Status = CFE_SUCCESS;
        }
        else if (CFE_MissionLib_PubSub_IsPublisherComponent(&Params))
        {
            *Type  = CFE_MSG_Type_Tlm;
            Status = CFE_SUCCESS;
        }
        else
        {
            *Type  = CFE_MSG_Type_Invalid;
            Status = CFE_MSG_BAD_ARGUMENT;
        }
    }

    return Status;
}

CFE_Status_t CFE_MSG_GetMsgId(const CFE_MSG_Message_t *MsgPtr, CFE_SB_MsgId_t *MsgId)
{
    CFE_SB_SoftwareBus_PubSub_Interface_t Params;

    if (MsgPtr == NULL || MsgId == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MissionLib_Get_PubSub_Parameters(&Params, &MsgPtr->BaseMsg);

    *MsgId = Params.MsgId;

    return CFE_SUCCESS;
}

CFE_Status_t CFE_MSG_SetMsgId(CFE_MSG_Message_t *MsgPtr, CFE_SB_MsgId_t MsgId)
{
    CFE_SB_SoftwareBus_PubSub_Interface_t Params;

    /*
     * NOTE: in reality the EDS may map any bits of the MsgId to the message, there is
     * not really a concept of a "highest" msg ID at all.  However, for historical/backward
     * compatibility reasons, this symbol is still defined in SB.
     */
    if (MsgPtr == NULL || CFE_SB_MsgIdToValue(MsgId) > CFE_PLATFORM_SB_HIGHEST_VALID_MSGID)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Params.MsgId = MsgId;
    CFE_MissionLib_Set_PubSub_Parameters(&MsgPtr->BaseMsg, &Params);

    return CFE_SUCCESS;
}
