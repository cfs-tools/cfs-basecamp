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
 * \file     cfe_missionlib_database_types.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 */

#ifndef CFE_MISSIONLIB_DATABASE_TYPES_H
#define CFE_MISSIONLIB_DATABASE_TYPES_H

#include <stdint.h>
#include <stddef.h>

#include "edslib_database_types.h"

/*
 * Caution about #include'ing additional files here ---
 * This is compiled _without_ the full include path from the rest of the CFE build
 * System headers are OK (as above) but locally-defined files should be avoided.
 */

struct CFE_MissionLib_DispatchTable_Entry
{
    uint16_t DispatchOffset;
};

typedef struct CFE_MissionLib_DispatchTable_Entry CFE_MissionLib_DispatchTable_Entry_t;

struct CFE_MissionLib_TopicId_Entry
{
    uint16_t             LocalDispatchSize;
    uint16_t             DispatchStartOffset;
    EdsLib_DatabaseRef_t InterfaceRefObj;

    const struct CFE_MissionLib_DispatchTable_Entry *LocalDispatchTable;
};

typedef struct CFE_MissionLib_TopicId_Entry CFE_MissionLib_TopicId_Entry_t;

struct CFE_MissionLib_SoftwareBus_Interface
{
    const EdsLib_DatabaseObject_t        *ParentGD;
    const CFE_MissionLib_TopicId_Entry_t *TopicList;
    const char *const                    *InstanceList;

    uint16_t NumInstances;
    uint16_t NumTopics;
};

#endif /* _CFE_MISSIONLIB_DATABASE_TYPES_H_ */
