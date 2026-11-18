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

#include "edslib_database_types.h"
#include "edslib_database_ops.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_database_types.h"

/*----------------------------------------------------------------
 *
 * CFE Missionlib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const CFE_MissionLib_TopicId_Entry_t *CFE_MissionLib_Lookup_Topic(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                                  uint16_t                                      TopicId)
{
    if (SBDB == NULL)
    {
        return NULL;
    }

    if (TopicId >= SBDB->NumTopics)
    {
        return NULL;
    }

    return &SBDB->TopicList[TopicId];
}

/*----------------------------------------------------------------
 *
 * CFE Missionlib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t CFE_MissionLib_ExportTopicInfo(const CFE_MissionLib_TopicId_Entry_t *TopicPtr,
                                       CFE_MissionLib_TopicInfo_t           *TopicInfo)
{
    if (TopicInfo == NULL)
    {
        return CFE_MISSIONLIB_INVALID_BUFFER;
    }

    TopicInfo->DispatchStartOffset = TopicPtr->DispatchStartOffset;
    TopicInfo->NumSubcommands      = TopicPtr->LocalDispatchSize;
    TopicInfo->ParentIntfId        = EdsLib_Encode_StructId(&TopicPtr->InterfaceRefObj);

    return CFE_MISSIONLIB_SUCCESS;
}
