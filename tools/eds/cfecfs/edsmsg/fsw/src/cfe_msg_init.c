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
#include "cfe_missionlib_runtime.h"

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
    CCSDS_CommonHdr_t *CommonHdr;
    CFE_Status_t       Status;

    if (MsgPtr == NULL || Size < sizeof(CCSDS_CommonHdr_t))
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    /* Clear and set defaults */
    memset(MsgPtr, 0, Size);

    /* Set various fields in the MsgPtr Metadata object from the bits in MsgId */
    Status = CFE_MSG_SetMsgId(MsgPtr, MsgId);
    if (Status == CFE_SUCCESS)
    {
        CommonHdr = (CCSDS_CommonHdr_t *)MsgPtr;

        /* Default to complete packets */
        CommonHdr->SeqFlag = 3; /* jphfix: enum? */

        /* Set the standard size field */
        CommonHdr->Length = Size - 7;
    }

    return Status;
}
