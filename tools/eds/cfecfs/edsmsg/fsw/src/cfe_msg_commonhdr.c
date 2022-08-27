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
 * Message CCSDS Primary header implementations
 */
#include "cfe_msg.h"
#include "cfe_error.h"

#include "ccsds_spacepacket_eds_typedefs.h"
#include "cfe_sb_eds_typedefs.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_missionlib_runtime.h"

#define CFE_MSG_SHDR_PRESENT_MASK_BIT (CCSDS_SecHdrFlags_Tlm & CCSDS_SecHdrFlags_Cmd)
#define CFE_MSG_SHDR_TYPE_MASK_BIT    (CCSDS_SecHdrFlags_Tlm ^ CCSDS_SecHdrFlags_Cmd)
#define CFE_MSG_SHDR_TYPE_TLM_BIT     (CCSDS_SecHdrFlags_BareTlm & CCSDS_SecHdrFlags_Tlm)
#define CFE_MSG_SHDR_TYPE_CMD_BIT     (CCSDS_SecHdrFlags_BareCmd & CCSDS_SecHdrFlags_Cmd)

#ifdef jphfix
CFE_MSG_Message_t *CFE_MSG_ConvertPtr(CFE_MSG_BaseMsg_t *BaseMsg)
{
    void *Msg = BaseMsg;
    return Msg;
}
#endif

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetHeaderVersion
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetHeaderVersion(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_HeaderVersion_t *Version)
{
    const CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || Version == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    *Version = Hdr->VersionId;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetHeaderVersion
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetHeaderVersion(CFE_MSG_Message_t *MsgPtr, CFE_MSG_HeaderVersion_t Version)
{
    CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || Version > 7)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (CCSDS_CommonHdr_t *)MsgPtr;

    Hdr->VersionId = Version;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetType
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetType(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_Type_t *Type)
{
    const CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || Type == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    switch (Hdr->SecHdrFlags)
    {
        case CCSDS_SecHdrFlags_BareTlm:
        case CCSDS_SecHdrFlags_Tlm:
            *Type = CFE_MSG_Type_Tlm;
            break;
        case CCSDS_SecHdrFlags_BareCmd:
        case CCSDS_SecHdrFlags_Cmd:
            *Type = CFE_MSG_Type_Cmd;
            break;
        default:
            *Type = CFE_MSG_Type_Invalid;
            break;
    }

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetType
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetType(CFE_MSG_Message_t *MsgPtr, CFE_MSG_Type_t Type)
{
    CCSDS_CommonHdr_t *      Hdr;
    CCSDS_SecHdrFlags_Enum_t SetVal;

    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr    = (CCSDS_CommonHdr_t *)MsgPtr;
    SetVal = 0;

    if (Type == CFE_MSG_Type_Tlm)
    {
        SetVal = CFE_MSG_SHDR_TYPE_TLM_BIT;
    }
    else if (Type == CFE_MSG_Type_Cmd)
    {
        SetVal = CFE_MSG_SHDR_TYPE_CMD_BIT;
    }
    else
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    /* preserve header presence bit */
    Hdr->SecHdrFlags = (Hdr->SecHdrFlags & ~CFE_MSG_SHDR_TYPE_MASK_BIT) | SetVal;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetHasSecondaryHeader
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetHasSecondaryHeader(const CFE_MSG_Message_t *MsgPtr, bool *HasSecondary)
{
    const CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || HasSecondary == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    *HasSecondary = (Hdr->SecHdrFlags & CFE_MSG_SHDR_PRESENT_MASK_BIT) != 0;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetHasSecondaryHeader
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetHasSecondaryHeader(CFE_MSG_Message_t *MsgPtr, bool HasSecondary)
{
    CCSDS_CommonHdr_t *      Hdr;
    CCSDS_SecHdrFlags_Enum_t SetVal;

    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (CCSDS_CommonHdr_t *)MsgPtr;

    if (HasSecondary)
    {
        SetVal = CFE_MSG_SHDR_PRESENT_MASK_BIT;
    }
    else
    {
        SetVal = 0;
    }

    /* preserve header type bit */
    Hdr->SecHdrFlags = (Hdr->SecHdrFlags & ~CFE_MSG_SHDR_PRESENT_MASK_BIT) | SetVal;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetApId
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetApId(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_ApId_t *ApId)
{
    const CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || ApId == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    *ApId = Hdr->AppId;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetApId
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetApId(CFE_MSG_Message_t *MsgPtr, CFE_MSG_ApId_t ApId)
{
    CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || ApId > 0x3FF)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (CCSDS_CommonHdr_t *)MsgPtr;

    Hdr->AppId = ApId;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetSegmentationFlag
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetSegmentationFlag(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_SegmentationFlag_t *SegFlag)
{
    const CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || SegFlag == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    /* JPHFIX: should be EDS enum? */
    switch (Hdr->SeqFlag)
    {
        case 0:
            *SegFlag = CFE_MSG_SegFlag_Continue;
            break;
        case 1:
            *SegFlag = CFE_MSG_SegFlag_First;
            break;
        case 2:
            *SegFlag = CFE_MSG_SegFlag_Last;
            break;
        case 3:
            *SegFlag = CFE_MSG_SegFlag_Unsegmented;
            break;
        default:
            *SegFlag = CFE_MSG_SegFlag_Invalid;
            break;
    }

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetSegmentationFlag
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetSegmentationFlag(CFE_MSG_Message_t *MsgPtr, CFE_MSG_SegmentationFlag_t SegFlag)
{
    CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (CCSDS_CommonHdr_t *)MsgPtr;

    /* JPHFIX: should be EDS enum? */
    switch (SegFlag)
    {
        case CFE_MSG_SegFlag_Continue:
            Hdr->SeqFlag = 0;
            break;
        case CFE_MSG_SegFlag_First:
            Hdr->SeqFlag = 1;
            break;
        case CFE_MSG_SegFlag_Last:
            Hdr->SeqFlag = 2;
            break;
        case CFE_MSG_SegFlag_Unsegmented:
            Hdr->SeqFlag = 3;
            break;
        default:
            return CFE_MSG_BAD_ARGUMENT;
    }

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetSequenceCount
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetSequenceCount(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_SequenceCount_t *SeqCnt)
{
    const CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || SeqCnt == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    *SeqCnt = Hdr->Sequence;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetSequenceCount
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetSequenceCount(CFE_MSG_Message_t *MsgPtr, CFE_MSG_SequenceCount_t SeqCnt)
{
    CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || SeqCnt > 0x3FFF)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (CCSDS_CommonHdr_t *)MsgPtr;

    Hdr->Sequence = SeqCnt;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetNextSequenceCount
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_MSG_SequenceCount_t CFE_MSG_GetNextSequenceCount(CFE_MSG_SequenceCount_t SeqCnt)
{
    return (SeqCnt + 1) & 0x3FFF;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_GetSize
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_GetSize(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_Size_t *Size)
{
    const CCSDS_CommonHdr_t *Hdr;

    if (MsgPtr == NULL || Size == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (const CCSDS_CommonHdr_t *)MsgPtr;

    /* note that EDS pack/unpack reapplies the CCSDS length calibration */
    *Size = Hdr->Length + 7;

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_MSG_SetSize
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_MSG_SetSize(CFE_MSG_Message_t *MsgPtr, CFE_MSG_Size_t Size)
{
    CCSDS_CommonHdr_t *Hdr;

    /*
     * note that EDS pack/unpack reapplies the traditional CCSDS length calibration (+/- 7)
     * so this needs to maintain this difference here.  The usable range of message
     * lengths is therefore 7-65542 bytes.
     */
    if (MsgPtr == NULL || Size < 7 || Size > 65542)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    Hdr = (CCSDS_CommonHdr_t *)MsgPtr;

    Hdr->Length = Size - 7;

    return CFE_SUCCESS;
}
