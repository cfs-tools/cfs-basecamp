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

#ifndef _CFE_MISSIONLIB_DATABASE_TYPES_H_
#define _CFE_MISSIONLIB_DATABASE_TYPES_H_

#include <stdint.h>
#include <stddef.h>

/*
 * Caution about #include'ing additional files here ---
 * This is compiled _without_ the full include path from the rest of the CFE build
 * System headers are OK (as above) but locally-defined files should be avoided.
 */

struct CFE_MissionLib_Argument_Entry
{
    uint16_t AppIndex;
    uint16_t TypeIndex;
};

typedef struct CFE_MissionLib_Argument_Entry CFE_MissionLib_Argument_Entry_t;

struct CFE_MissionLib_Subcommand_Entry
{
    uint16_t DispatchOffset;
};

typedef struct CFE_MissionLib_Subcommand_Entry CFE_MissionLib_Subcommand_Entry_t;

struct CFE_MissionLib_Command_Definition_Entry
{
    uint16_t                                      SubcommandArg;
    uint16_t                                      SubcommandCount;
    const struct CFE_MissionLib_Argument_Entry *  ArgumentList;
    const struct CFE_MissionLib_Subcommand_Entry *SubcommandList;
};

typedef struct CFE_MissionLib_Command_Definition_Entry CFE_MissionLib_Command_Definition_Entry_t;

struct CFE_MissionLib_TopicId_Entry
{
    uint16_t                                         DispatchTableId;
    uint16_t                                         DispatchStartOffset;
    uint16_t                                         InterfaceId;
    const char *                                     TopicName;
    const CFE_MissionLib_Command_Definition_Entry_t *CommandList;
};

typedef struct CFE_MissionLib_TopicId_Entry CFE_MissionLib_TopicId_Entry_t;

struct CFE_MissionLib_Command_Prototype_Entry
{
    uint16_t    NumArguments;
    const char *CommandName;
};

typedef struct CFE_MissionLib_Command_Prototype_Entry CFE_MissionLib_Command_Prototype_Entry_t;

struct CFE_MissionLib_InterfaceId_Entry
{
    uint16_t                                        NumCommands;
    uint16_t                                        NumTopics;
    const char *                                    InterfaceName;
    const CFE_MissionLib_Command_Prototype_Entry_t *CommandList;
    const CFE_MissionLib_TopicId_Entry_t *          TopicList;
};

typedef struct CFE_MissionLib_InterfaceId_Entry CFE_MissionLib_InterfaceId_Entry_t;

struct CFE_MissionLib_SoftwareBus_Interface
{
    uint16_t                                  NumInterfaces;
    const CFE_MissionLib_InterfaceId_Entry_t *InterfaceList;
    const char *const *                       InstanceList;
};

#endif /* _CFE_MISSIONLIB_DATABASE_TYPES_H_ */
