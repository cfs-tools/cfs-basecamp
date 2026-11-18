/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 *
 * Copyright (c) 2020 United States Government as represented by
 * the Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file     cfe_missionlib_api.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Provides the conversions between CFE software bus abstract "MsgId" values
 * and the actual values used inside message headers.  This is actually compiled
 * a small self contained library such that it can be linked easily into both
 * CFE flight software as well as ground tools and utilities that also need to
 * interpret or generate software bus messages.
 *
 * Provided that everything links to this library and therefore uses the same
 * conversion logic, everything should stay synchronized all the time even if
 * the logic needs to change at some point.
 *
 * This also allows the logic to be customized per-mission.
 *
 * The default logic is to perform a bitwise shift and mapping, but this is
 * not required.  It can be arbitrary, e.g. a table lookup if necessary.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "cfe_missionlib_internal.h"

/*
 * ***********************************************************************
 *  PUBLIC API FUNCTIONS (non-static)
 * ***********************************************************************
 */

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
const EdsLib_DatabaseObject_t *CFE_MissionLib_GetParent(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB)
{
    return SBDB->ParentGD;
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
uint16_t CFE_MissionLib_GetNumTopics(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB)
{
    uint16_t Result = 0;

    if (SBDB != NULL)
    {
        Result = SBDB->NumTopics;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
uint16_t CFE_MissionLib_GetNumInstances(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB)
{
    uint16_t Result = 0;

    if (SBDB != NULL)
    {
        Result = SBDB->NumInstances;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
int32_t CFE_MissionLib_GetTopicInfo(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                    uint16_t                                      TopicId,
                                    CFE_MissionLib_TopicInfo_t                   *TopicInfo)
{
    const CFE_MissionLib_TopicId_Entry_t *TopicPtr;
    int32_t                               Status;

    TopicPtr = CFE_MissionLib_Lookup_Topic(SBDB, TopicId);
    if (TopicPtr == NULL)
    {
        Status = CFE_MISSIONLIB_INVALID_TOPIC;
    }
    else
    {
        Status = CFE_MissionLib_ExportTopicInfo(TopicPtr, TopicInfo);
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
const char *CFE_MissionLib_GetInstanceNameOrNull(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                 uint16_t                                      InstanceNum)
{
    uint16_t    NameNum;
    const char *Result;

    /* The lookup table is 0-based while runtime numbers are 1-based */
    NameNum = InstanceNum - 1;
    if (NameNum >= SBDB->NumInstances)
    {
        Result = NULL;
    }
    else
    {
        Result = SBDB->InstanceList[NameNum];
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
const char *CFE_MissionLib_GetInstanceNameNonNull(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                  uint16_t                                      InstanceNum)
{
    static const char UNDEFINED_INSTANCE_NAME[] = "[undefined]";
    const char       *Result;

    Result = CFE_MissionLib_GetInstanceNameOrNull(SBDB, InstanceNum);
    if (Result == NULL)
    {
        Result = UNDEFINED_INSTANCE_NAME;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
uint16_t CFE_MissionLib_GetInstanceNumber(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB, const char *String)
{
    uint16_t    InstanceNum;
    const char *NamePtr;

    InstanceNum = 0;
    if (SBDB->InstanceList != NULL && SBDB->NumInstances > 0)
    {
        InstanceNum = SBDB->NumInstances;
        while (InstanceNum > 0)
        {
            NamePtr = SBDB->InstanceList[InstanceNum - 1];
            if (NamePtr != NULL && strcmp(NamePtr, String) == 0)
            {
                break;
            }

            --InstanceNum;
        }
    }

    return InstanceNum;
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
void CFE_MissionLib_EnumerateTopics(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                    CFE_MissionLib_TopicInfo_Callback_t           Callback,
                                    void                                         *OpaqueArg)
{
    const CFE_MissionLib_TopicId_Entry_t *TopicPtr;
    uint16_t                              TopicId;

    for (TopicId = 0; TopicId < SBDB->NumTopics; ++TopicId)
    {
        TopicPtr = &SBDB->TopicList[TopicId];

        Callback(OpaqueArg, TopicId, EdsLib_Encode_StructId(&TopicPtr->InterfaceRefObj));
    }
}

/*----------------------------------------------------------------
 *
 * Public API function
 *
 *-----------------------------------------------------------------*/
int32_t CFE_MissionLib_FindTopicIdFromIntfId(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                             EdsLib_Id_t                                   IntfEdsId,
                                             uint16_t                                     *TopicIdBuffer)
{
    const CFE_MissionLib_TopicId_Entry_t *TopicPtr;
    uint16_t                              TopicId;
    int32_t                               Status;
    EdsLib_DatabaseRef_t                  RefObj;

    Status = CFE_MISSIONLIB_INVALID_INTERFACE;
    EdsLib_Decode_StructId(&RefObj, IntfEdsId);

    for (TopicId = 1; TopicId < SBDB->NumTopics; ++TopicId)
    {
        TopicPtr = &SBDB->TopicList[TopicId];

        if (EdsLib_DatabaseRef_IsEqual(&TopicPtr->InterfaceRefObj, &RefObj))
        {
            Status = CFE_MISSIONLIB_SUCCESS;
            break;
        }
    }

    if (TopicIdBuffer != NULL)
    {
        if (Status == CFE_MISSIONLIB_SUCCESS)
        {
            *TopicIdBuffer = TopicId;
        }
        else
        {
            *TopicIdBuffer = 0;
        }
    }

    return Status;
}
