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
 * \file     edslib_basic_test.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
 * Basic testing of the "data type" EDS structures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "utassert.h"
#include "uttest.h"

#include "edslib_test_common.h"
#include "edslib_datatypedb.h"
#include "edslib_intfdb.h"

void Test_EdsLib_IntfDB_Initialize(void)
{
    /* Test case for:
     * void EdsLib_IntfDB_Initialize(void);
     */
    UtAssert_VOIDCALL(EdsLib_IntfDB_Initialize());
}

void Test_EdsLib_IntfDB_FindComponentByLocalName(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindComponentByLocalName(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx, const char
     * *CompName, EdsLib_Id_t *IdBuffer);
     */
    EdsLib_Id_t IdBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_FindComponentByLocalName(&UT_DATABASE, UT_INDEX_UTAPP, "UnitTest", &IdBuffer),
                      EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(IdBuffer, UT_EDS_COMPONENT_ID);
}

void Test_EdsLib_IntfDB_FindDeclaredInterfaceByLocalName(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindDeclaredInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx, const
     * char *IntfName, EdsLib_Id_t *IdBuffer);
     */
    EdsLib_Id_t IdBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_FindDeclaredInterfaceByLocalName(&UT_DATABASE, UT_INDEX_UTHDR, "UtCmd", &IdBuffer),
                      EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(IdBuffer, UT_EDS_CMDDECL_ID);
}

void Test_EdsLib_IntfDB_FindDeclaredInterfaceByFullName(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindDeclaredInterfaceByFullName(const EdsLib_DatabaseObject_t *GD, const char *IntfName,
     * EdsLib_Id_t *IdBuffer);
     */
    EdsLib_Id_t IdBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_FindDeclaredInterfaceByFullName(&UT_DATABASE, "UtHdr/UtCmd", &IdBuffer),
                      EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(IdBuffer, UT_EDS_CMDDECL_ID);
}

void Test_EdsLib_IntfDB_FindComponentInterfaceByLocalName(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindComponentInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t
     * ComponentId, const char *IntfName, EdsLib_Id_t *IdBuffer);
     */
    EdsLib_Id_t IdBuffer;

    UtAssert_INT32_EQ(
        EdsLib_IntfDB_FindComponentInterfaceByLocalName(&UT_DATABASE, UT_EDS_COMPONENT_ID, "TestCmd", &IdBuffer),
        EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(IdBuffer, UT_EDS_CMDINTF_ID);
}

void Test_EdsLib_IntfDB_FindComponentInterfaceByFullName(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindComponentInterfaceByFullName(const EdsLib_DatabaseObject_t *GD, const char *IntfName,
     * EdsLib_Id_t *IdBuffer);
     */
    EdsLib_Id_t IdBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_FindComponentInterfaceByFullName(&UT_DATABASE, "UtApp/UnitTest/TestCmd", &IdBuffer),
                      EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(IdBuffer, UT_EDS_CMDINTF_ID);
}

void Test_EdsLib_IntfDB_FindCommandByLocalName(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindCommandByLocalName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t IntfId, const char
     * *CmdName, EdsLib_Id_t *IdBuffer);
     */
    EdsLib_Id_t IdBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_FindCommandByLocalName(&UT_DATABASE, UT_EDS_CMDDECL_ID, "recv", &IdBuffer),
                      EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(IdBuffer, UT_EDS_RECVCMD_ID);
}

void Test_EdsLib_IntfDB_FindAllCommands(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindAllCommands(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t IntfId, EdsLib_Id_t
     * *IdBuffer, size_t NumIdBufs);
     */
    EdsLib_Id_t IdBuffer[4];

    memset(IdBuffer, 0, sizeof(IdBuffer));

    UtAssert_INT32_EQ(EdsLib_IntfDB_FindAllCommands(&UT_DATABASE, UT_EDS_CMDDECL_ID, IdBuffer, 4), EDSLIB_SUCCESS);

    UtAssert_UINT32_EQ(IdBuffer[0], UT_EDS_RECVCMD_ID);
    UtAssert_BOOL_FALSE(EdsLib_Is_Valid(IdBuffer[1]));
    UtAssert_BOOL_FALSE(EdsLib_Is_Valid(IdBuffer[2]));
    UtAssert_BOOL_FALSE(EdsLib_Is_Valid(IdBuffer[3]));
}

void Test_EdsLib_IntfDB_FindAllArgumentTypes(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_FindAllArgumentTypes(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t CmdEdsId, EdsLib_Id_t
     * CompIntfEdsId, EdsLib_Id_t *IdBuffer, size_t NumIdBufs);
     */
    EdsLib_Id_t IdBuffer[4];

    memset(IdBuffer, 0, sizeof(IdBuffer));

    UtAssert_INT32_EQ(
        EdsLib_IntfDB_FindAllArgumentTypes(&UT_DATABASE, UT_EDS_RECVCMD_ID, UT_EDS_CMDINTF_ID, IdBuffer, 4),
        EDSLIB_SUCCESS);

    UtAssert_UINT32_EQ(IdBuffer[0], UT_EDS_CMDHDR_ID);
    UtAssert_BOOL_FALSE(EdsLib_Is_Valid(IdBuffer[1]));
    UtAssert_BOOL_FALSE(EdsLib_Is_Valid(IdBuffer[2]));
    UtAssert_BOOL_FALSE(EdsLib_Is_Valid(IdBuffer[3]));
}

void Test_EdsLib_IntfDB_GetComponentInterfaceInfo(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_GetComponentInterfaceInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t IntfId,
     * EdsLib_IntfDB_InterfaceInfo_t *IntfInfoBuffer);
     */
    EdsLib_IntfDB_InterfaceInfo_t IntfInfoBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_GetComponentInterfaceInfo(&UT_DATABASE, UT_EDS_CMDINTF_ID, &IntfInfoBuffer),
                      EDSLIB_SUCCESS);

    UtAssert_STRINGBUF_EQ(IntfInfoBuffer.InterfaceName, -1, "TestCmd", -1);
    UtAssert_UINT32_EQ(IntfInfoBuffer.IntfTypeEdsId, UT_EDS_CMDDECL_ID);
    UtAssert_UINT32_EQ(IntfInfoBuffer.ParentCompEdsId, UT_EDS_COMPONENT_ID);
    UtAssert_UINT16_EQ(IntfInfoBuffer.GenericTypeMapCount, 0);
}

void Test_EdsLib_IntfDB_GetComponentInfo(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_GetComponentInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t ComponentId,
     * EdsLib_IntfDB_ComponentInfo_t *CompInfoBuffer);
     */
    EdsLib_IntfDB_ComponentInfo_t CompInfoBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_GetComponentInfo(&UT_DATABASE, UT_EDS_COMPONENT_ID, &CompInfoBuffer),
                      EDSLIB_SUCCESS);

    UtAssert_STRINGBUF_EQ(CompInfoBuffer.ComponentName, -1, "UnitTest", -1);
    UtAssert_UINT16_EQ(CompInfoBuffer.RequiredIntfCount, 2);
}

void Test_EdsLib_IntfDB_GetFullName(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_GetFullName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *BufferPtr, size_t
     * BufferLen);
     */
    char Buffer[32];

    UtAssert_INT32_EQ(EdsLib_IntfDB_GetFullName(&UT_DATABASE, UT_EDS_COMPONENT_ID, Buffer, sizeof(Buffer)),
                      EDSLIB_SUCCESS);
    UtAssert_STRINGBUF_EQ(Buffer, sizeof(Buffer), "UtApp/UnitTest", -1);
}

void Test_EdsLib_IntfDB_GetCommandInfo(void)
{
    /* Test case for:
     * int32_t EdsLib_IntfDB_GetCommandInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t CommandId,
     * EdsLib_IntfDB_CommandInfo_t *CommandInfoBuffer);
     */
    EdsLib_IntfDB_CommandInfo_t CommandInfoBuffer;

    UtAssert_INT32_EQ(EdsLib_IntfDB_GetCommandInfo(&UT_DATABASE, UT_EDS_RECVCMD_ID, &CommandInfoBuffer),
                      EDSLIB_SUCCESS);

    UtAssert_STRINGBUF_EQ(CommandInfoBuffer.CommandName, -1, "recv", -1);

    UtAssert_UINT32_EQ(CommandInfoBuffer.ParentDeclIntfId, UT_EDS_CMDDECL_ID);
    UtAssert_UINT16_EQ(CommandInfoBuffer.ArgumentCount, 1);
}

void UtTest_Setup(void)
{
    UtTest_Add(Test_EdsLib_IntfDB_Initialize, NULL, NULL, "EdsLib_IntfDB_Initialize");
    UtTest_Add(Test_EdsLib_IntfDB_FindComponentByLocalName, NULL, NULL, "EdsLib_IntfDB_FindComponentByLocalName");
    UtTest_Add(Test_EdsLib_IntfDB_FindDeclaredInterfaceByLocalName,
               NULL,
               NULL,
               "EdsLib_IntfDB_FindDeclaredInterfaceByLocalName");
    UtTest_Add(Test_EdsLib_IntfDB_FindDeclaredInterfaceByFullName,
               NULL,
               NULL,
               "EdsLib_IntfDB_FindDeclaredInterfaceByFullName");
    UtTest_Add(Test_EdsLib_IntfDB_FindComponentInterfaceByLocalName,
               NULL,
               NULL,
               "EdsLib_IntfDB_FindComponentInterfaceByLocalName");
    UtTest_Add(Test_EdsLib_IntfDB_FindComponentInterfaceByFullName,
               NULL,
               NULL,
               "EdsLib_IntfDB_FindComponentInterfaceByFullName");
    UtTest_Add(Test_EdsLib_IntfDB_FindCommandByLocalName, NULL, NULL, "EdsLib_IntfDB_FindCommandByLocalName");
    UtTest_Add(Test_EdsLib_IntfDB_FindAllCommands, NULL, NULL, "EdsLib_IntfDB_FindAllCommands");
    UtTest_Add(Test_EdsLib_IntfDB_FindAllArgumentTypes, NULL, NULL, "EdsLib_IntfDB_FindAllArgumentTypes");
    UtTest_Add(Test_EdsLib_IntfDB_GetComponentInterfaceInfo, NULL, NULL, "EdsLib_IntfDB_GetComponentInterfaceInfo");
    UtTest_Add(Test_EdsLib_IntfDB_GetComponentInfo, NULL, NULL, "EdsLib_IntfDB_GetComponentInfo");
    UtTest_Add(Test_EdsLib_IntfDB_GetFullName, NULL, NULL, "EdsLib_IntfDB_GetFullName");
    UtTest_Add(Test_EdsLib_IntfDB_GetCommandInfo, NULL, NULL, "EdsLib_IntfDB_GetCommandInfo");
}
