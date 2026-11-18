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

#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * Message initialization
 */
#include "cfe_msg.h"
#include "cfe_time.h"
#include "cfe_sb.h"
#include "cfe_missionlib_runtime.h"

#include "ccsds_spacepacket_eds_datatypes.h"
#include "cfe_hdr_eds_datatypes.h"

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_Init
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_Init(CFE_MSG_Message_t *MsgPtr, CFE_SB_MsgId_t MsgId, CFE_MSG_Size_t Size)
{
    CFE_Status_t Status;

    if (MsgPtr == NULL || Size < sizeof(EdsDataType_CCSDS_CommonHdr_t))
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    /* Clear and set defaults */
    memset(MsgPtr, 0, Size);

    /* Allow "CFE_SB_INVALID_MSG_ID" here - in some cases the real msgid is filled in later */
    /* however the size should always be a valid value */
    if (CFE_SB_MsgIdToValue(MsgId) == CFE_SB_MsgIdToValue(CFE_SB_INVALID_MSG_ID))
    {
        Status = CFE_SUCCESS;
    }
    else
    {
        Status = CFE_MSG_SetMsgId(MsgPtr, MsgId);
    }

    if (Status == CFE_SUCCESS)
    {
        CFE_MSG_SetSegmentationFlag(MsgPtr, CFE_MSG_SegFlag_Unsegmented);

        Status = CFE_MSG_SetSize(MsgPtr, Size);
    }

    return Status;
}
