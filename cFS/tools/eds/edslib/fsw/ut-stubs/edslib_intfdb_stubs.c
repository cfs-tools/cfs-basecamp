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
 * Auto-Generated stub implementations for functions defined in edslib_intfdb header
 */

#include "edslib_intfdb.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindAllArgumentTypes()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindAllArgumentTypes(const EdsLib_DatabaseObject_t *GD,
                                           EdsLib_Id_t                    CmdEdsId,
                                           EdsLib_Id_t                    CompIntfEdsId,
                                           EdsLib_Id_t                   *IdBuffer,
                                           size_t                         NumIdBufs)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindAllArgumentTypes, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllArgumentTypes, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllArgumentTypes, EdsLib_Id_t, CmdEdsId);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllArgumentTypes, EdsLib_Id_t, CompIntfEdsId);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllArgumentTypes, EdsLib_Id_t *, IdBuffer);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllArgumentTypes, size_t, NumIdBufs);

    UT_GenStub_Execute(EdsLib_IntfDB_FindAllArgumentTypes, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindAllArgumentTypes, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindAllCommands()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindAllCommands(const EdsLib_DatabaseObject_t *GD,
                                      EdsLib_Id_t                    IntfId,
                                      EdsLib_Id_t                   *IdBuffer,
                                      size_t                         NumIdBufs)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindAllCommands, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllCommands, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllCommands, EdsLib_Id_t, IntfId);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllCommands, EdsLib_Id_t *, IdBuffer);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindAllCommands, size_t, NumIdBufs);

    UT_GenStub_Execute(EdsLib_IntfDB_FindAllCommands, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindAllCommands, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindCommandByLocalName()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindCommandByLocalName(const EdsLib_DatabaseObject_t *GD,
                                             EdsLib_Id_t                    IntfId,
                                             const char                    *CmdName,
                                             EdsLib_Id_t                   *IdBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindCommandByLocalName, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindCommandByLocalName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindCommandByLocalName, EdsLib_Id_t, IntfId);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindCommandByLocalName, const char *, CmdName);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindCommandByLocalName, EdsLib_Id_t *, IdBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_FindCommandByLocalName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindCommandByLocalName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindComponentByLocalName()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindComponentByLocalName(const EdsLib_DatabaseObject_t *GD,
                                               uint16_t                       AppIdx,
                                               const char                    *CompName,
                                               EdsLib_Id_t                   *IdBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindComponentByLocalName, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentByLocalName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentByLocalName, uint16_t, AppIdx);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentByLocalName, const char *, CompName);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentByLocalName, EdsLib_Id_t *, IdBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_FindComponentByLocalName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindComponentByLocalName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindComponentInterfaceByFullName()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindComponentInterfaceByFullName(const EdsLib_DatabaseObject_t *GD,
                                                       const char                    *IntfName,
                                                       EdsLib_Id_t                   *IdBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindComponentInterfaceByFullName, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentInterfaceByFullName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentInterfaceByFullName, const char *, IntfName);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentInterfaceByFullName, EdsLib_Id_t *, IdBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_FindComponentInterfaceByFullName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindComponentInterfaceByFullName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindComponentInterfaceByLocalName()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindComponentInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                        EdsLib_Id_t                    ComponentId,
                                                        const char                    *IntfName,
                                                        EdsLib_Id_t                   *IdBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindComponentInterfaceByLocalName, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentInterfaceByLocalName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentInterfaceByLocalName, EdsLib_Id_t, ComponentId);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentInterfaceByLocalName, const char *, IntfName);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindComponentInterfaceByLocalName, EdsLib_Id_t *, IdBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_FindComponentInterfaceByLocalName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindComponentInterfaceByLocalName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindDeclaredInterfaceByFullName()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindDeclaredInterfaceByFullName(const EdsLib_DatabaseObject_t *GD,
                                                      const char                    *IntfName,
                                                      EdsLib_Id_t                   *IdBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindDeclaredInterfaceByFullName, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindDeclaredInterfaceByFullName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindDeclaredInterfaceByFullName, const char *, IntfName);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindDeclaredInterfaceByFullName, EdsLib_Id_t *, IdBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_FindDeclaredInterfaceByFullName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindDeclaredInterfaceByFullName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_FindDeclaredInterfaceByLocalName()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_FindDeclaredInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                       uint16_t                       AppIdx,
                                                       const char                    *IntfName,
                                                       EdsLib_Id_t                   *IdBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName, uint16_t, AppIdx);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName, const char *, IntfName);
    UT_GenStub_AddParam(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName, EdsLib_Id_t *, IdBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_GetCommandInfo()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_GetCommandInfo(const EdsLib_DatabaseObject_t *GD,
                                     EdsLib_Id_t                    CommandId,
                                     EdsLib_IntfDB_CommandInfo_t   *CommandInfoBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_GetCommandInfo, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_GetCommandInfo, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetCommandInfo, EdsLib_Id_t, CommandId);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetCommandInfo, EdsLib_IntfDB_CommandInfo_t *, CommandInfoBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_GetCommandInfo, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_GetCommandInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_GetComponentInfo()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_GetComponentInfo(const EdsLib_DatabaseObject_t *GD,
                                       EdsLib_Id_t                    ComponentId,
                                       EdsLib_IntfDB_ComponentInfo_t *CompInfoBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_GetComponentInfo, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_GetComponentInfo, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetComponentInfo, EdsLib_Id_t, ComponentId);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetComponentInfo, EdsLib_IntfDB_ComponentInfo_t *, CompInfoBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_GetComponentInfo, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_GetComponentInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_GetComponentInterfaceInfo()
 * ----------------------------------------------------
 */
int32_t EdsLib_IntfDB_GetComponentInterfaceInfo(const EdsLib_DatabaseObject_t *GD,
                                                EdsLib_Id_t                    IntfId,
                                                EdsLib_IntfDB_InterfaceInfo_t *IntfInfoBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_GetComponentInterfaceInfo, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_GetComponentInterfaceInfo, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetComponentInterfaceInfo, EdsLib_Id_t, IntfId);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetComponentInterfaceInfo, EdsLib_IntfDB_InterfaceInfo_t *, IntfInfoBuffer);

    UT_GenStub_Execute(EdsLib_IntfDB_GetComponentInterfaceInfo, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_GetComponentInterfaceInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_GetFullName()
 * ----------------------------------------------------
 */
int32_t
EdsLib_IntfDB_GetFullName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *BufferPtr, size_t BufferLen)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_IntfDB_GetFullName, int32_t);

    UT_GenStub_AddParam(EdsLib_IntfDB_GetFullName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetFullName, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetFullName, char *, BufferPtr);
    UT_GenStub_AddParam(EdsLib_IntfDB_GetFullName, size_t, BufferLen);

    UT_GenStub_Execute(EdsLib_IntfDB_GetFullName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_IntfDB_GetFullName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_IntfDB_Initialize()
 * ----------------------------------------------------
 */
void EdsLib_IntfDB_Initialize(void)
{
    UT_GenStub_Execute(EdsLib_IntfDB_Initialize, Basic, NULL);
}
