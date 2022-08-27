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
** File: cfe_sb_util.c
**
** Purpose:
**      This file contains 'access' macros and functions for reading and
**      writing message header fields.
**
** Author:   R.McGraw/SSI
**
******************************************************************************/

/*
** Include Files
*/

#include "cfe_sb_module_all.h"

#include "cfe_config.h"
#include "edslib_datatypedb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include <string.h>

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_MsgHdrSize
 *
 * Application-scope internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
size_t CFE_SB_MsgHdrSize(const CFE_MSG_Message_t *MsgPtr)
{
    size_t         size      = 0;
    bool           hassechdr = false;
    CFE_MSG_Type_t type      = CFE_MSG_Type_Invalid;

    if (MsgPtr == NULL)
    {
        return size;
    }

    CFE_MSG_GetHasSecondaryHeader(MsgPtr, &hassechdr);
    CFE_MSG_GetType(MsgPtr, &type);

    /* if secondary hdr is not present... */
    /* Since all cFE messages must have a secondary hdr this check is not needed */
    if (!hassechdr)
    {
        size = sizeof(CFE_MSG_Message_t);
    }
    else if (type == CFE_MSG_Type_Cmd)
    {
        size = sizeof(CFE_MSG_CommandHeader_t);
    }
    else if (type == CFE_MSG_Type_Tlm)
    {
        size = sizeof(CFE_MSG_TelemetryHeader_t);
    }

    return size;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_GetUserPayloadInfo
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_SB_GetUserPayloadInfo(const CFE_MSG_Message_t *MsgPtr, EdsLib_DataTypeDB_EntityInfo_t *PayloadInfo)
{
    union
    {
        CFE_SB_Listener_Component_t  Listener;
        CFE_SB_Publisher_Component_t Publisher;

    } FuncParams;
    int32                                    EdsStatus;
    EdsLib_Id_t                              EdsId;
    EdsLib_DataTypeDB_DerivativeObjectInfo_t DerivObjInfo;
    CFE_SB_SoftwareBus_PubSub_Interface_t    PubSubParams;

    const EdsLib_DatabaseObject_t *               EDS_DB    = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);
    const CFE_MissionLib_SoftwareBus_Interface_t *SBINTF_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_SBINTF_DB);

    EdsStatus = CFE_MISSIONLIB_FAILURE;
    if (MsgPtr != NULL)
    {
        CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &MsgPtr->BaseMsg);

        if (CFE_MissionLib_PubSub_IsListenerComponent(&PubSubParams))
        {
            CFE_MissionLib_UnmapListenerComponent(&FuncParams.Listener, &PubSubParams);

            EdsStatus = CFE_MissionLib_GetArgumentType(SBINTF_DB, CFE_SB_Telecommand_Interface_ID,
                                                       FuncParams.Listener.Telecommand.TopicId, 1, 1, &EdsId);
        }
        else if (CFE_MissionLib_PubSub_IsPublisherComponent(&PubSubParams))
        {
            CFE_MissionLib_UnmapPublisherComponent(&FuncParams.Publisher, &PubSubParams);

            EdsStatus = CFE_MissionLib_GetArgumentType(SBINTF_DB, CFE_SB_Telemetry_Interface_ID,
                                                       FuncParams.Publisher.Telemetry.TopicId, 1, 1, &EdsId);
        }
    }

    /*
     * The above code yields an interface base type.  Need to potentially interpret
     * value constraints within the headers to determine final/real type.
     */
    if (EdsStatus == CFE_MISSIONLIB_SUCCESS)
    {
        EdsStatus = EdsLib_DataTypeDB_IdentifyBuffer(EDS_DB, EdsId, MsgPtr, &DerivObjInfo);
        if (EdsStatus == EDSLIB_SUCCESS)
        {
            /* Use the derived type as the actual EdsId */
            EdsId = DerivObjInfo.EdsId;
        }
        else if (EdsStatus == EDSLIB_NO_MATCHING_VALUE)
        {
            /* This is OK if the struct is not derived or has no additional value constraints */
            EdsStatus = EDSLIB_SUCCESS;
        }
    }

    /*
     * Index 0 is always the header, and index 1 should always be the first element of real data
     */
    if (EdsStatus == EDSLIB_SUCCESS)
    {
        EdsStatus = EdsLib_DataTypeDB_GetMemberByIndex(EDS_DB, EdsId, 1, PayloadInfo);
    }

    if (EdsStatus != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_GetUserData
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void *CFE_SB_GetUserData(CFE_MSG_Message_t *MsgPtr)
{
    CFE_Status_t                   Status;
    EdsLib_DataTypeDB_EntityInfo_t PayloadInfo;

    Status = CFE_SB_GetUserPayloadInfo(MsgPtr, &PayloadInfo);
    if (Status != CFE_SUCCESS)
    {
        return NULL;
    }

    return &MsgPtr->Byte[PayloadInfo.Offset.Bytes];
}

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_GetUserDataLength
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
size_t CFE_SB_GetUserDataLength(const CFE_MSG_Message_t *MsgPtr)
{
    CFE_Status_t                   Status;
    CFE_MSG_Size_t                 TotalSize;
    EdsLib_DataTypeDB_EntityInfo_t PayloadInfo;

    Status = CFE_SB_GetUserPayloadInfo(MsgPtr, &PayloadInfo);
    if (Status != CFE_SUCCESS)
    {
        return 0;
    }

    Status = CFE_MSG_GetSize(MsgPtr, &TotalSize);
    if (Status != CFE_SUCCESS || TotalSize < PayloadInfo.Offset.Bytes)
    {
        return 0;
    }

    return TotalSize - PayloadInfo.Offset.Bytes;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_SetUserDataLength
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void CFE_SB_SetUserDataLength(CFE_MSG_Message_t *MsgPtr, size_t DataLength)
{
    EdsLib_DataTypeDB_EntityInfo_t PayloadInfo;
    CFE_Status_t                   Status;

    if (MsgPtr == NULL)
    {
        CFE_ES_WriteToSysLog("%s: Failed invalid arguments\n", __func__);
    }
    else
    {
        Status = CFE_SB_GetUserPayloadInfo(MsgPtr, &PayloadInfo);
        if (Status != CFE_SUCCESS)
        {
            CFE_ES_WriteToSysLog("%s: Failed unknown payload location\n", __func__);
        }
        else
        {
            CFE_MSG_SetSize(MsgPtr, DataLength + PayloadInfo.Offset.Bytes);
        }
    }
}

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_TimeStampMsg
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void CFE_SB_TimeStampMsg(CFE_MSG_Message_t *MsgPtr)
{
    CFE_MSG_SetMsgTime(MsgPtr, CFE_TIME_GetTime());
}

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_MessageStringGet
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_SB_MessageStringGet(char *DestStringPtr, const char *SourceStringPtr, const char *DefaultString,
                              size_t DestMaxSize, size_t SourceMaxSize)
{
    int32 Result;

    /*
     * Error in caller if DestMaxSize == 0.
     * Cannot terminate the string, since there is no place for the NUL
     * In this case, do nothing
     */
    if (DestMaxSize == 0 || DestStringPtr == NULL)
    {
        Result = CFE_SB_BAD_ARGUMENT;
    }
    else
    {
        Result = 0;

        /*
         * Check if should use the default, which is if
         * the source string has zero length (first char is NUL).
         */
        if (DefaultString != NULL && (SourceMaxSize == 0 || *SourceStringPtr == 0))
        {
            SourceStringPtr = DefaultString;
            SourceMaxSize   = DestMaxSize;
        }

        /* Reserve 1 character for the required NUL */
        --DestMaxSize;

        while (SourceMaxSize > 0 && *SourceStringPtr != 0 && DestMaxSize > 0)
        {
            *DestStringPtr = *SourceStringPtr;
            ++DestStringPtr;
            ++SourceStringPtr;
            --SourceMaxSize;
            --DestMaxSize;

            ++Result;
        }

        /* Put the NUL in the last character */
        *DestStringPtr = 0;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * Function: CFE_SB_MessageStringSet
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_SB_MessageStringSet(char *DestStringPtr, const char *SourceStringPtr, size_t DestMaxSize,
                              size_t SourceMaxSize)
{
    int32 Result;

    if (SourceStringPtr == NULL || DestStringPtr == NULL)
    {
        Result = CFE_SB_BAD_ARGUMENT;
    }
    else
    {
        Result = 0;

        while (SourceMaxSize > 0 && *SourceStringPtr != 0 && DestMaxSize > 0)
        {
            *DestStringPtr = *SourceStringPtr;
            ++DestStringPtr;
            ++SourceStringPtr;
            ++Result;
            --DestMaxSize;
            --SourceMaxSize;
        }

        /*
         * Pad the remaining space with NUL chars,
         * but this should NOT be included in the final size
         */
        while (DestMaxSize > 0)
        {
            /* Put the NUL in the last character */
            *DestStringPtr = 0;
            ++DestStringPtr;
            --DestMaxSize;
        }
    }

    return Result;
}
