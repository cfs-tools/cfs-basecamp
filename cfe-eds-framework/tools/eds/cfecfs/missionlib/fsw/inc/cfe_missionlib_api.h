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
 * \file     cfe_missionlib_api.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Defines a runtime API for CFE to obtain information about
 * the software bus intefaces/routing and correlate with EDS.
 */

#ifndef _CFE_MISSIONLIB_API_H_
#define _CFE_MISSIONLIB_API_H_

/*
 * The EDS library uses the fixed-width types from the C99 stdint.h header file,
 * rather than the OSAL types.  This allows use in applications that are not
 * based on OSAL.
 */
#include <stdint.h>
#include <stddef.h>

#include <edslib_id.h>

/******************************
 * MACROS
 ******************************/

/******************************
 * TYPEDEFS
 ******************************/

enum
{
    CFE_MISSIONLIB_NOT_IMPLEMENTED    = -10,
    CFE_MISSIONLIB_INVALID_SUBCOMMAND = -6,
    CFE_MISSIONLIB_INVALID_ARGUMENT   = -6,
    CFE_MISSIONLIB_INVALID_INDICATION = -5,
    CFE_MISSIONLIB_INVALID_TOPIC      = -4,
    CFE_MISSIONLIB_INVALID_MESSAGE    = -3,
    CFE_MISSIONLIB_INVALID_INTERFACE  = -2,
    CFE_MISSIONLIB_FAILURE            = -1,
    CFE_MISSIONLIB_SUCCESS            = 0
};

typedef struct CFE_MissionLib_SoftwareBus_Interface CFE_MissionLib_SoftwareBus_Interface_t;

/**
 * Argument to the basic data dictionary walk callback function.
 * Indicates basic Type, Size and Offset information about a data element.
 */
struct CFE_MissionLib_InterfaceInfo
{
    uint16_t NumCommands;
    uint16_t NumTopics;
};

typedef struct CFE_MissionLib_InterfaceInfo CFE_MissionLib_InterfaceInfo_t;

struct CFE_MissionLib_IndicationInfo
{
    uint16_t NumArguments;
    uint16_t SubcommandArgumentId;
    uint16_t NumSubcommands;
};

typedef struct CFE_MissionLib_IndicationInfo CFE_MissionLib_IndicationInfo_t;

struct CFE_MissionLib_TopicInfo
{
    uint16_t DispatchTableId;
    uint16_t DispatchStartOffset;
};

typedef struct CFE_MissionLib_TopicInfo CFE_MissionLib_TopicInfo_t;

typedef void (*CFE_MissionLib_TopicInfo_Callback_t)(void *Arg, uint16_t TopicId, const char *TopicName);

/******************************
 * API CALLS
 ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

    const char *CFE_MissionLib_GetInstanceName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InstanceNum,
                                               char *Buffer, uint32_t BufferSize);
    uint16_t CFE_MissionLib_GetInstanceNumber(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, const char *String);

    int32_t CFE_MissionLib_GetIndicationInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                             uint16_t TopicId, uint16_t IndicationId,
                                             CFE_MissionLib_IndicationInfo_t *IndInfo);
    int32_t CFE_MissionLib_GetInterfaceInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                            CFE_MissionLib_InterfaceInfo_t *IntfInfo);
    int32_t CFE_MissionLib_GetTopicInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                        uint16_t TopicId, CFE_MissionLib_TopicInfo_t *TopicInfo);
    int32_t CFE_MissionLib_GetArgumentType(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                           uint16_t TopicId, uint16_t IndicationId, uint16_t ArgumentId,
                                           EdsLib_Id_t *Id);
    int32_t CFE_MissionLib_GetSubcommandOffset(const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                                               uint16_t InterfaceType, uint16_t TopicId, uint16_t IndicationId,
                                               uint16_t SubcommandId, uint16_t *Offset);

    int32_t CFE_MissionLib_FindInterfaceByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, const char *IntfName,
                                               uint16_t *InterfaceIdBuffer);
    int32_t CFE_MissionLib_FindTopicByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                           const char *TopicName, uint16_t *TopicIdBuffer);
    int32_t CFE_MissionLib_FindCommandByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                             const char *CommandName, uint16_t *CommandIdBuffer);
    const char *CFE_MissionLib_GetTopicName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                            uint16_t TopicId);
    const char *CFE_MissionLib_GetInterfaceName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                                                uint16_t                                      InterfaceType);
    const char *CFE_MissionLib_GetCommandName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                                              uint16_t InterfaceType, uint16_t CommandId);
    void CFE_MissionLib_EnumerateTopics(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                        CFE_MissionLib_TopicInfo_Callback_t Callback, void *OpaqueArg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CFE_MISSIONLIB_API_H_ */
