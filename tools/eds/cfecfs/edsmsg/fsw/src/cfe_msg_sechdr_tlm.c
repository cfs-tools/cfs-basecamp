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

#include "cfe_msg.h"
#include "cfe_error.h"

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetMsgTime
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetMsgTime(CFE_MSG_Message_t *MsgPtr, CFE_TIME_SysTime_t NewTime)
{
    CFE_HDR_TelemetryHeader_t *TlmPtr;
    const CCSDS_CommonHdr_t *  CommonHdr;

    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CommonHdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    /* This confirms if it is OK to access the pointer as a "TelemetryHeader" type */
    if (CommonHdr->SecHdrFlags != CCSDS_SecHdrFlags_Tlm)
    {
        return CFE_MSG_WRONG_MSG_TYPE;
    }

    TlmPtr = (CFE_HDR_TelemetryHeader_t *)(void *)MsgPtr;

    TlmPtr->Sec.Seconds = NewTime.Seconds;

    /* For 16-bit subseconds, just use the 16 MSBs of the CFE_TIME value */
    if (sizeof(TlmPtr->Sec.Subseconds) == sizeof(uint16))
    {
        TlmPtr->Sec.Subseconds = (NewTime.Subseconds >> 16) & 0xFFFF;
    }
    else
    {
        TlmPtr->Sec.Subseconds = NewTime.Subseconds;
    }

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetMsgTime
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetMsgTime(const CFE_MSG_Message_t *MsgPtr, CFE_TIME_SysTime_t *Time)
{
    const CFE_HDR_TelemetryHeader_t *TlmPtr;
    const CCSDS_CommonHdr_t *        CommonHdr;

    if (MsgPtr == NULL || Time == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CommonHdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    /* This confirms if it is OK to access the pointer as a "TelemetryHeader" type */
    if (CommonHdr->SecHdrFlags != CCSDS_SecHdrFlags_Tlm)
    {
        return CFE_MSG_WRONG_MSG_TYPE;
    }

    TlmPtr = (const CFE_HDR_TelemetryHeader_t *)(const void *)MsgPtr;

    Time->Seconds = TlmPtr->Sec.Seconds;

    /* For 16-bit subseconds, use the value from the msg as the 16 MSBs of the CFE_TIME value */
    if (sizeof(TlmPtr->Sec.Subseconds) == sizeof(uint16))
    {
        Time->Subseconds = TlmPtr->Sec.Subseconds << 16;
    }
    else
    {
        Time->Subseconds = TlmPtr->Sec.Subseconds;
    }

    return CFE_SUCCESS;
}
