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
 *  Function code field access functions
 */
#include "cfe_msg.h"

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetFcnCode
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetFcnCode(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_FcnCode_t *FcnCode)
{
    const CFE_HDR_CommandHeader_t *CmdPtr;
    const CCSDS_CommonHdr_t *      CommonHdr;

    if (MsgPtr == NULL || FcnCode == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CommonHdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    /* This confirms if it is OK to access the pointer as a "CommandHeader" type */
    if (CommonHdr->SecHdrFlags != CCSDS_SecHdrFlags_Cmd)
    {
        return CFE_MSG_WRONG_MSG_TYPE;
    }

    CmdPtr = (const CFE_HDR_CommandHeader_t *)(const void *)MsgPtr;

    *FcnCode = CmdPtr->Sec.FunctionCode;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetFcnCode
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetFcnCode(CFE_MSG_Message_t *MsgPtr, CFE_MSG_FcnCode_t FcnCode)
{
    CFE_HDR_CommandHeader_t *CmdPtr;
    const CCSDS_CommonHdr_t *CommonHdr;

    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CommonHdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    /* This confirms if it is OK to access the pointer as a "CommandHeader" type */
    if (CommonHdr->SecHdrFlags != CCSDS_SecHdrFlags_Cmd)
    {
        return CFE_MSG_WRONG_MSG_TYPE;
    }

    CmdPtr = (CFE_HDR_CommandHeader_t *)(void *)MsgPtr;

    CmdPtr->Sec.FunctionCode = FcnCode;

    return CFE_SUCCESS;
}

/*
 * NOTE on the checksum computations, this is done by EdsLib at the time the packet goes
 * to/from the network or external consumer.  Applications should not be dealing directly
 * with checksum field in commands.
 *
 * The checksum field definitely _is_ implemented, it is just not implemented here.
 */

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GenerateChecksum
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GenerateChecksum(CFE_MSG_Message_t *MsgPtr)
{
    return CFE_MSG_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_ValidateChecksum
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_ValidateChecksum(const CFE_MSG_Message_t *MsgPtr, bool *IsValid)
{
    return CFE_MSG_NOT_IMPLEMENTED;
}
