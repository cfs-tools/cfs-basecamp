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
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_database_types.h"

/*
 * ***********************************************************************
 *  LOOKUP HELPER FUNCTIONS (static, internal only)
 * ***********************************************************************
 */

static const CFE_MissionLib_InterfaceId_Entry_t *
CFE_MissionLib_Lookup_SubIntf(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType)
{
    if (InterfaceType == 0 || InterfaceType > Intf->NumInterfaces)
    {
        return NULL;
    }

    return &Intf->InterfaceList[InterfaceType - 1];
}

static const CFE_MissionLib_TopicId_Entry_t *
CFE_MissionLib_Lookup_Topic(const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr, uint16_t TopicId)
{
    if (IntfPtr == NULL || TopicId == 0 || TopicId > IntfPtr->NumTopics)
    {
        return NULL;
    }

    return &IntfPtr->TopicList[TopicId - 1];
}

static const CFE_MissionLib_Command_Prototype_Entry_t *
CFE_MissionLib_Lookup_Command_Prototype(const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr, uint16_t IndicationId)
{
    if (IntfPtr == NULL || IndicationId == 0 || IndicationId > IntfPtr->NumCommands)
    {
        return NULL;
    }

    return &IntfPtr->CommandList[IndicationId - 1];
}

static const CFE_MissionLib_Command_Definition_Entry_t *
CFE_MissionLib_Lookup_Command_Definition(const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr,
                                         const CFE_MissionLib_TopicId_Entry_t *TopicPtr, uint16_t IndicationId)
{
    if (IntfPtr == NULL || TopicPtr == NULL || IndicationId == 0 || IndicationId > IntfPtr->NumCommands)
    {
        return NULL;
    }

    return &TopicPtr->CommandList[IndicationId - 1];
}

static const CFE_MissionLib_Argument_Entry_t *
CFE_MissionLib_Lookup_Command_Argument(const CFE_MissionLib_Command_Definition_Entry_t *CmdPtr,
                                       const CFE_MissionLib_Command_Prototype_Entry_t * PrototypePtr,
                                       uint16_t                                         ArgumentId)
{
    if (CmdPtr == NULL || PrototypePtr == NULL || ArgumentId == 0 || ArgumentId > PrototypePtr->NumArguments)
    {
        return NULL;
    }

    return &CmdPtr->ArgumentList[ArgumentId - 1];
}

static const CFE_MissionLib_Subcommand_Entry_t *
CFE_MissionLib_Lookup_Subcommand(const CFE_MissionLib_Command_Definition_Entry_t *CmdPtr, uint16_t SubcommandId)
{
    if (CmdPtr == NULL || SubcommandId == 0 || SubcommandId > CmdPtr->SubcommandCount)
    {
        return NULL;
    }

    return &CmdPtr->SubcommandList[SubcommandId - 1];
}

/*
 * ***********************************************************************
 *  PUBLIC API FUNCTIONS (non-static)
 * ***********************************************************************
 */

int32_t CFE_MissionLib_GetTopicInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                    uint16_t TopicId, CFE_MissionLib_TopicInfo_t *TopicInfo)
{
    const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr;
    const CFE_MissionLib_TopicId_Entry_t *    TopicPtr;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    TopicPtr = CFE_MissionLib_Lookup_Topic(IntfPtr, TopicId);
    if (TopicPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_TOPIC;
    }

    if (TopicPtr->InterfaceId != InterfaceType)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    TopicInfo->DispatchTableId     = TopicPtr->DispatchTableId;
    TopicInfo->DispatchStartOffset = TopicPtr->DispatchStartOffset;

    return CFE_MISSIONLIB_SUCCESS;
}

int32_t CFE_MissionLib_GetIndicationInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                         uint16_t TopicId, uint16_t IndicationId,
                                         CFE_MissionLib_IndicationInfo_t *IndInfo)
{
    const CFE_MissionLib_InterfaceId_Entry_t *       IntfPtr;
    const CFE_MissionLib_TopicId_Entry_t *           TopicPtr;
    const CFE_MissionLib_Command_Prototype_Entry_t * CmdProtoPtr;
    const CFE_MissionLib_Command_Definition_Entry_t *CmdDefPtr;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    TopicPtr = CFE_MissionLib_Lookup_Topic(IntfPtr, TopicId);
    if (TopicPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_TOPIC;
    }

    if (TopicPtr->InterfaceId != InterfaceType)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    CmdProtoPtr = CFE_MissionLib_Lookup_Command_Prototype(IntfPtr, IndicationId);
    CmdDefPtr   = CFE_MissionLib_Lookup_Command_Definition(IntfPtr, TopicPtr, IndicationId);
    if (CmdProtoPtr == NULL || CmdDefPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_INDICATION;
    }

    memset(IndInfo, 0, sizeof(*IndInfo));
    IndInfo->NumArguments         = CmdProtoPtr->NumArguments;
    IndInfo->NumSubcommands       = CmdDefPtr->SubcommandCount;
    IndInfo->SubcommandArgumentId = CmdDefPtr->SubcommandArg;

    return CFE_MISSIONLIB_SUCCESS;
}

int32_t CFE_MissionLib_GetArgumentType(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                       uint16_t TopicId, uint16_t IndicationId, uint16_t ArgumentId, EdsLib_Id_t *Id)
{
    const CFE_MissionLib_InterfaceId_Entry_t *       IntfPtr;
    const CFE_MissionLib_TopicId_Entry_t *           TopicPtr;
    const CFE_MissionLib_Command_Prototype_Entry_t * CmdProtoPtr;
    const CFE_MissionLib_Command_Definition_Entry_t *CmdDefPtr;
    const CFE_MissionLib_Argument_Entry_t *          ArgPtr;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    TopicPtr = CFE_MissionLib_Lookup_Topic(IntfPtr, TopicId);
    if (TopicPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_TOPIC;
    }

    if (TopicPtr->InterfaceId != InterfaceType)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    CmdProtoPtr = CFE_MissionLib_Lookup_Command_Prototype(IntfPtr, IndicationId);
    CmdDefPtr   = CFE_MissionLib_Lookup_Command_Definition(IntfPtr, TopicPtr, IndicationId);
    if (CmdProtoPtr == NULL || CmdDefPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_INDICATION;
    }

    ArgPtr = CFE_MissionLib_Lookup_Command_Argument(CmdDefPtr, CmdProtoPtr, ArgumentId);
    if (ArgPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_ARGUMENT;
    }

    *Id = EDSLIB_MAKE_ID(ArgPtr->AppIndex, ArgPtr->TypeIndex);

    return CFE_MISSIONLIB_SUCCESS;
}

int32_t CFE_MissionLib_GetInterfaceInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceId,
                                        CFE_MissionLib_InterfaceInfo_t *IntfInfo)
{
    int32_t Status;

    if (InterfaceId == 0 || InterfaceId > Intf->NumInterfaces)
    {
        Status = CFE_MISSIONLIB_INVALID_INTERFACE;
    }
    else
    {
        Status                = CFE_MISSIONLIB_SUCCESS;
        IntfInfo->NumCommands = Intf->InterfaceList[InterfaceId - 1].NumCommands;
        IntfInfo->NumTopics   = Intf->InterfaceList[InterfaceId - 1].NumTopics;
    }

    return Status;
}

int32_t CFE_MissionLib_GetSubcommandOffset(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                           uint16_t TopicId, uint16_t IndicationId, uint16_t SubcommandId,
                                           uint16_t *Offset)
{
    const CFE_MissionLib_InterfaceId_Entry_t *       IntfPtr;
    const CFE_MissionLib_TopicId_Entry_t *           TopicPtr;
    const CFE_MissionLib_Command_Prototype_Entry_t * CmdProtoPtr;
    const CFE_MissionLib_Command_Definition_Entry_t *CmdDefPtr;
    const CFE_MissionLib_Subcommand_Entry_t *        SubCmdPtr;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    TopicPtr = CFE_MissionLib_Lookup_Topic(IntfPtr, TopicId);
    if (TopicPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_TOPIC;
    }

    if (TopicPtr->InterfaceId != InterfaceType)
    {
        return CFE_MISSIONLIB_INVALID_INTERFACE;
    }

    CmdProtoPtr = CFE_MissionLib_Lookup_Command_Prototype(IntfPtr, IndicationId);
    CmdDefPtr   = CFE_MissionLib_Lookup_Command_Definition(IntfPtr, TopicPtr, IndicationId);
    if (CmdProtoPtr == NULL || CmdDefPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_INDICATION;
    }

    SubCmdPtr = CFE_MissionLib_Lookup_Subcommand(CmdDefPtr, SubcommandId);
    if (SubCmdPtr == NULL)
    {
        return CFE_MISSIONLIB_INVALID_SUBCOMMAND;
    }

    *Offset = SubCmdPtr->DispatchOffset;

    return CFE_MISSIONLIB_SUCCESS;
}

int32_t CFE_MissionLib_FindInterfaceByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, const char *IntfName,
                                           uint16_t *InterfaceIdBuffer)
{
    uint16_t InterfaceId;
    int32_t  Status = CFE_MISSIONLIB_INVALID_INTERFACE;

    for (InterfaceId = 0; InterfaceId < Intf->NumInterfaces; ++InterfaceId)
    {
        if (strcmp(Intf->InterfaceList[InterfaceId].InterfaceName, IntfName) == 0)
        {
            Status             = CFE_MISSIONLIB_SUCCESS;
            *InterfaceIdBuffer = 1 + InterfaceId;
            break;
        }
    }

    return Status;
}

int32_t CFE_MissionLib_FindTopicByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                       const char *TopicName, uint16_t *TopicIdBuffer)
{
    const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr;
    const CFE_MissionLib_TopicId_Entry_t *    TopicPtr;
    uint16_t                                  TopicId;
    int32_t                                   Status;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        Status = CFE_MISSIONLIB_INVALID_INTERFACE;
    }
    else
    {
        Status = CFE_MISSIONLIB_INVALID_TOPIC;
        for (TopicId = 0; TopicId < IntfPtr->NumTopics; ++TopicId)
        {
            TopicPtr = &IntfPtr->TopicList[TopicId];
            if (TopicPtr->InterfaceId == InterfaceType && TopicPtr->TopicName != NULL &&
                strcmp(TopicPtr->TopicName, TopicName) == 0)
            {
                Status         = CFE_MISSIONLIB_SUCCESS;
                *TopicIdBuffer = 1 + TopicId;
                break;
            }
        }
    }

    return Status;
}

int32_t CFE_MissionLib_FindCommandByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                         const char *CommandName, uint16_t *CommandIdBuffer)
{
    const CFE_MissionLib_InterfaceId_Entry_t *      IntfPtr;
    const CFE_MissionLib_Command_Prototype_Entry_t *CmdPtr;
    uint16_t                                        CommandId;
    int32_t                                         Status;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        Status = CFE_MISSIONLIB_INVALID_INTERFACE;
    }
    else
    {
        Status = CFE_MISSIONLIB_INVALID_TOPIC;
        for (CommandId = 0; CommandId < IntfPtr->NumCommands; ++CommandId)
        {
            CmdPtr = &IntfPtr->CommandList[CommandId];
            if (CmdPtr->CommandName != NULL && strcmp(CmdPtr->CommandName, CommandName) == 0)
            {
                Status           = CFE_MISSIONLIB_SUCCESS;
                *CommandIdBuffer = 1 + CommandId;
                break;
            }
        }
    }

    return Status;
}

const char *CFE_MissionLib_GetCommandName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                          uint16_t CommandId)
{
    const CFE_MissionLib_InterfaceId_Entry_t *      IntfPtr;
    const CFE_MissionLib_Command_Prototype_Entry_t *CmdProtoPtr;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        return NULL;
    }

    CmdProtoPtr = CFE_MissionLib_Lookup_Command_Prototype(IntfPtr, CommandId);
    if (CmdProtoPtr == NULL)
    {
        return NULL;
    }

    return CmdProtoPtr->CommandName;
}

const char *CFE_MissionLib_GetTopicName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                        uint16_t TopicId)
{
    const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr;
    const CFE_MissionLib_TopicId_Entry_t *    TopicPtr;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        return NULL;
    }

    TopicPtr = CFE_MissionLib_Lookup_Topic(IntfPtr, TopicId);
    if ((TopicPtr == NULL) || (TopicPtr->InterfaceId != InterfaceType))
    {
        return NULL;
    }

    return TopicPtr->TopicName;
}

const char *CFE_MissionLib_GetInterfaceName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType)
{
    const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr == NULL)
    {
        return NULL;
    }

    return IntfPtr->InterfaceName;
}

const char *CFE_MissionLib_GetInstanceName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InstanceNum,
                                           char *Buffer, uint32_t BufferSize)
{
    uint16_t           NameNum;
    const char *const *InstanceName;
    const char *       Result;

    if (InstanceNum > 0 && Intf->InstanceList != NULL)
    {
        NameNum      = InstanceNum - 1;
        InstanceName = Intf->InstanceList;
        while (*InstanceName != NULL && NameNum > 0)
        {
            --NameNum;
            ++InstanceName;
        }
        Result = *InstanceName;
    }
    else
    {
        Result = NULL;
    }

    if (Buffer != NULL && BufferSize > 0)
    {
        if (Result != NULL)
        {
            snprintf(Buffer, BufferSize, "%s", Result);
        }
        else
        {
            Result = Buffer;
            if (InstanceNum == 0)
            {
                Buffer[0] = 0;
            }
            else
            {
                snprintf(Buffer, BufferSize, "%u", (unsigned int)InstanceNum);
            }
        }
    }

    return Result;
}

uint16_t CFE_MissionLib_GetInstanceNumber(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, const char *String)
{
    uint16_t    InstanceNum;
    size_t      PartLength;
    const char *EndPtr;

    PartLength = strlen(String);
    if (Intf->InstanceList != NULL)
    {
        for (InstanceNum = 0; Intf->InstanceList[InstanceNum] != NULL; ++InstanceNum)
        {
            if (strncmp(Intf->InstanceList[InstanceNum], String, PartLength) == 0 &&
                Intf->InstanceList[InstanceNum][PartLength] == 0)
            {
                break;
            }
        }
    }

    if (Intf->InstanceList == NULL || Intf->InstanceList[InstanceNum] == NULL)
    {
        InstanceNum = strtoul(String, (char **)&EndPtr, 10);
        if (EndPtr == String)
        {
            InstanceNum = 0;
        }
    }
    else
    {
        /* Name lookup was successful, need to convert to 1-based indexing */
        ++InstanceNum;
    }

    return InstanceNum;
}

void CFE_MissionLib_EnumerateTopics(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                    CFE_MissionLib_TopicInfo_Callback_t Callback, void *OpaqueArg)
{
    const CFE_MissionLib_InterfaceId_Entry_t *IntfPtr;
    const CFE_MissionLib_TopicId_Entry_t *    TopicPtr;
    uint16_t                                  TopicId;

    IntfPtr = CFE_MissionLib_Lookup_SubIntf(Intf, InterfaceType);
    if (IntfPtr != NULL)
    {
        TopicPtr = IntfPtr->TopicList;
        for (TopicId = 0; TopicId < IntfPtr->NumTopics; ++TopicId)
        {
            if (TopicPtr->InterfaceId == InterfaceType)
            {
                Callback(OpaqueArg, 1 + TopicId, TopicPtr->TopicName);
            }
            ++TopicPtr;
        }
    }
}
