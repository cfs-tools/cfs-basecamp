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
 * @file
 *
 * Auto-Generated stub implementations for functions defined in cfe_missionlib_api header
 */

#include "cfe_missionlib_api.h"
#include "utgenstub.h"

extern void UT_DefaultHandler_CFE_MissionLib_FindCommandByName(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_FindInterfaceByName(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_FindTopicByName(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetArgumentType(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetCommandName(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetIndicationInfo(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetInstanceName(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetInstanceNumber(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetInterfaceInfo(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetInterfaceName(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetSubcommandOffset(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetTopicInfo(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_GetTopicName(void *, UT_EntryKey_t, const UT_StubContext_t *);

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_EnumerateTopics()
 * ----------------------------------------------------
 */
void CFE_MissionLib_EnumerateTopics(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                    CFE_MissionLib_TopicInfo_Callback_t Callback, void *OpaqueArg)
{
    UT_GenStub_AddParam(CFE_MissionLib_EnumerateTopics, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_EnumerateTopics, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_EnumerateTopics, CFE_MissionLib_TopicInfo_Callback_t, Callback);
    UT_GenStub_AddParam(CFE_MissionLib_EnumerateTopics, void *, OpaqueArg);

    UT_GenStub_Execute(CFE_MissionLib_EnumerateTopics, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_FindCommandByName()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_FindCommandByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                         const char *CommandName, uint16_t *CommandIdBuffer)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_FindCommandByName, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_FindCommandByName, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_FindCommandByName, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_FindCommandByName, const char *, CommandName);
    UT_GenStub_AddParam(CFE_MissionLib_FindCommandByName, uint16_t *, CommandIdBuffer);

    UT_GenStub_Execute(CFE_MissionLib_FindCommandByName, Basic, UT_DefaultHandler_CFE_MissionLib_FindCommandByName);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_FindCommandByName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_FindInterfaceByName()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_FindInterfaceByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, const char *IntfName,
                                           uint16_t *InterfaceIdBuffer)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_FindInterfaceByName, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_FindInterfaceByName, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_FindInterfaceByName, const char *, IntfName);
    UT_GenStub_AddParam(CFE_MissionLib_FindInterfaceByName, uint16_t *, InterfaceIdBuffer);

    UT_GenStub_Execute(CFE_MissionLib_FindInterfaceByName, Basic, UT_DefaultHandler_CFE_MissionLib_FindInterfaceByName);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_FindInterfaceByName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_FindTopicByName()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_FindTopicByName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                       const char *TopicName, uint16_t *TopicIdBuffer)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_FindTopicByName, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_FindTopicByName, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_FindTopicByName, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_FindTopicByName, const char *, TopicName);
    UT_GenStub_AddParam(CFE_MissionLib_FindTopicByName, uint16_t *, TopicIdBuffer);

    UT_GenStub_Execute(CFE_MissionLib_FindTopicByName, Basic, UT_DefaultHandler_CFE_MissionLib_FindTopicByName);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_FindTopicByName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetArgumentType()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_GetArgumentType(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                       uint16_t TopicId, uint16_t IndicationId, uint16_t ArgumentId, EdsLib_Id_t *Id)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetArgumentType, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetArgumentType, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetArgumentType, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_GetArgumentType, uint16_t, TopicId);
    UT_GenStub_AddParam(CFE_MissionLib_GetArgumentType, uint16_t, IndicationId);
    UT_GenStub_AddParam(CFE_MissionLib_GetArgumentType, uint16_t, ArgumentId);
    UT_GenStub_AddParam(CFE_MissionLib_GetArgumentType, EdsLib_Id_t *, Id);

    UT_GenStub_Execute(CFE_MissionLib_GetArgumentType, Basic, UT_DefaultHandler_CFE_MissionLib_GetArgumentType);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetArgumentType, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetCommandName()
 * ----------------------------------------------------
 */
const char *CFE_MissionLib_GetCommandName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                          uint16_t CommandId)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetCommandName, const char *);

    UT_GenStub_AddParam(CFE_MissionLib_GetCommandName, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetCommandName, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_GetCommandName, uint16_t, CommandId);

    UT_GenStub_Execute(CFE_MissionLib_GetCommandName, Basic, UT_DefaultHandler_CFE_MissionLib_GetCommandName);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetCommandName, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetIndicationInfo()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_GetIndicationInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                         uint16_t TopicId, uint16_t IndicationId,
                                         CFE_MissionLib_IndicationInfo_t *IndInfo)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetIndicationInfo, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetIndicationInfo, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetIndicationInfo, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_GetIndicationInfo, uint16_t, TopicId);
    UT_GenStub_AddParam(CFE_MissionLib_GetIndicationInfo, uint16_t, IndicationId);
    UT_GenStub_AddParam(CFE_MissionLib_GetIndicationInfo, CFE_MissionLib_IndicationInfo_t *, IndInfo);

    UT_GenStub_Execute(CFE_MissionLib_GetIndicationInfo, Basic, UT_DefaultHandler_CFE_MissionLib_GetIndicationInfo);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetIndicationInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetInstanceName()
 * ----------------------------------------------------
 */
const char *CFE_MissionLib_GetInstanceName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InstanceNum,
                                           char *Buffer, uint32_t BufferSize)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetInstanceName, const char *);

    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceName, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceName, uint16_t, InstanceNum);
    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceName, char *, Buffer);
    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceName, uint32_t, BufferSize);

    UT_GenStub_Execute(CFE_MissionLib_GetInstanceName, Basic, UT_DefaultHandler_CFE_MissionLib_GetInstanceName);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetInstanceName, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetInstanceNumber()
 * ----------------------------------------------------
 */
uint16_t CFE_MissionLib_GetInstanceNumber(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, const char *String)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetInstanceNumber, uint16_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNumber, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNumber, const char *, String);

    UT_GenStub_Execute(CFE_MissionLib_GetInstanceNumber, Basic, UT_DefaultHandler_CFE_MissionLib_GetInstanceNumber);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetInstanceNumber, uint16_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetInterfaceInfo()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_GetInterfaceInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                        CFE_MissionLib_InterfaceInfo_t *IntfInfo)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetInterfaceInfo, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetInterfaceInfo, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetInterfaceInfo, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_GetInterfaceInfo, CFE_MissionLib_InterfaceInfo_t *, IntfInfo);

    UT_GenStub_Execute(CFE_MissionLib_GetInterfaceInfo, Basic, UT_DefaultHandler_CFE_MissionLib_GetInterfaceInfo);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetInterfaceInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetInterfaceName()
 * ----------------------------------------------------
 */
const char *CFE_MissionLib_GetInterfaceName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetInterfaceName, const char *);

    UT_GenStub_AddParam(CFE_MissionLib_GetInterfaceName, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetInterfaceName, uint16_t, InterfaceType);

    UT_GenStub_Execute(CFE_MissionLib_GetInterfaceName, Basic, UT_DefaultHandler_CFE_MissionLib_GetInterfaceName);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetInterfaceName, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetSubcommandOffset()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_GetSubcommandOffset(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                           uint16_t TopicId, uint16_t IndicationId, uint16_t SubcommandId,
                                           uint16_t *Offset)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetSubcommandOffset, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetSubcommandOffset, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetSubcommandOffset, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_GetSubcommandOffset, uint16_t, TopicId);
    UT_GenStub_AddParam(CFE_MissionLib_GetSubcommandOffset, uint16_t, IndicationId);
    UT_GenStub_AddParam(CFE_MissionLib_GetSubcommandOffset, uint16_t, SubcommandId);
    UT_GenStub_AddParam(CFE_MissionLib_GetSubcommandOffset, uint16_t *, Offset);

    UT_GenStub_Execute(CFE_MissionLib_GetSubcommandOffset, Basic, UT_DefaultHandler_CFE_MissionLib_GetSubcommandOffset);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetSubcommandOffset, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetTopicInfo()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_GetTopicInfo(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                    uint16_t TopicId, CFE_MissionLib_TopicInfo_t *TopicInfo)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetTopicInfo, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetTopicInfo, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetTopicInfo, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_GetTopicInfo, uint16_t, TopicId);
    UT_GenStub_AddParam(CFE_MissionLib_GetTopicInfo, CFE_MissionLib_TopicInfo_t *, TopicInfo);

    UT_GenStub_Execute(CFE_MissionLib_GetTopicInfo, Basic, UT_DefaultHandler_CFE_MissionLib_GetTopicInfo);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetTopicInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetTopicName()
 * ----------------------------------------------------
 */
const char *CFE_MissionLib_GetTopicName(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                                        uint16_t TopicId)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetTopicName, const char *);

    UT_GenStub_AddParam(CFE_MissionLib_GetTopicName, const CFE_MissionLib_SoftwareBus_Interface_t *, Intf);
    UT_GenStub_AddParam(CFE_MissionLib_GetTopicName, uint16_t, InterfaceType);
    UT_GenStub_AddParam(CFE_MissionLib_GetTopicName, uint16_t, TopicId);

    UT_GenStub_Execute(CFE_MissionLib_GetTopicName, Basic, UT_DefaultHandler_CFE_MissionLib_GetTopicName);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetTopicName, const char *);
}
