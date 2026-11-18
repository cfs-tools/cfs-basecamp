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
 * \file     edslib_full_test.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
 * Unit testing of the "full" (name-based) EDS API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "utassert.h"
#include "uttest.h"

#include "edslib_test_common.h"

#include "edslib_displaydb.h"

void Test_EdsLib_DisplayDB_Initialize(void)
{
    /* Test Case for:
     * void EdsLib_DisplayDB_Initialize(void);
     */
    UtAssert_VOIDCALL(EdsLib_DisplayDB_Initialize());
}

void Test_EdsLib_DisplayDB_GetEdsName(void)
{
    /* Test Case for:
     * const char *EdsLib_DisplayDB_GetEdsName(const EdsLib_DatabaseObject_t *GD, uint16_t AppId);
     */
    const char *Name;

    UtAssert_NOT_NULL(Name = EdsLib_DisplayDB_GetEdsName(&UT_DATABASE, UT_INDEX_UTAPP));
    UtAssert_STRINGBUF_EQ(Name, -1, "utapp", -1);
}

void Test_EdsLib_DisplayDB_GetBaseName(void)
{
    /* Test Case for:
     * const char *EdsLib_DisplayDB_GetBaseName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId);
     */
    const char *Name;

    UtAssert_NOT_NULL(Name = EdsLib_DisplayDB_GetBaseName(&UT_DATABASE, UT_EDS_UINT32_ID));
    UtAssert_STRINGBUF_EQ(Name, -1, "UINT32", -1);
}

void Test_EdsLib_DisplayDB_GetNamespace(void)
{
    /* Test Case for:
     * const char *EdsLib_DisplayDB_GetNamespace(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId);
     */
    const char *Name;

    UtAssert_NOT_NULL(Name = EdsLib_DisplayDB_GetNamespace(&UT_DATABASE, UT_EDS_CMDHDR_ID));
    UtAssert_STRINGBUF_EQ(Name, -1, "UtHdr", -1);
    UtAssert_NOT_NULL(Name = EdsLib_DisplayDB_GetNamespace(&UT_DATABASE, UT_EDS_UINT32_ID));
    UtAssert_STRINGBUF_EQ(Name, -1, "CCSDS/SOIS/SEDS", -1);
}

void Test_EdsLib_DisplayDB_GetTypeName(void)
{
    /* Test Case for:
     * const char *EdsLib_DisplayDB_GetTypeName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *Buffer,
     * uint32_t BufferSize);
     */
    char        StrBuffer[32];
    const char *Name;

    UtAssert_NOT_NULL(Name =
                          EdsLib_DisplayDB_GetTypeName(&UT_DATABASE, UT_EDS_UINT32_ID, StrBuffer, sizeof(StrBuffer)));
    UtAssert_STRINGBUF_EQ(Name, -1, "CCSDS/SOIS/SEDS/UINT32", -1);
}

void Test_EdsLib_DisplayDB_GetDisplayHint(void)
{
    /* Test Case for:
     * EdsLib_DisplayHint_t EdsLib_DisplayDB_GetDisplayHint(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId);
     */
    UtAssert_INT32_EQ(EdsLib_DisplayDB_GetDisplayHint(&UT_DATABASE, UT_EDS_UINT32_ID), EDSLIB_DISPLAYHINT_NONE);
}

void Test_EdsLib_DisplayDB_LookupTypeName(void)
{
    /* Test Case for:
     * EdsLib_Id_t EdsLib_DisplayDB_LookupTypeName(const EdsLib_DatabaseObject_t *GD, const char *String);
     */
    UtAssert_UINT32_EQ(EdsLib_DisplayDB_LookupTypeName(&UT_DATABASE, "CCSDS/SOIS/SEDS/UINT32"), UT_EDS_UINT32_ID);
}

void Test_EdsLib_DisplayDB_GetIndexByName(void)
{
    /* Test Case for:
     * int32_t EdsLib_DisplayDB_GetIndexByName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name,
     * uint16_t *SubIndex);
     */
    uint16_t SubIndex = 0xFFFF;

    UtAssert_INT32_EQ(EdsLib_DisplayDB_GetIndexByName(&UT_DATABASE, UT_EDS_CMD1_ID, "Cmd1Data1", &SubIndex),
                      EDSLIB_SUCCESS);
    UtAssert_UINT16_EQ(SubIndex, 1);
}

void Test_EdsLib_DisplayDB_GetNameByIndex(void)
{
    /* Test Case for:
     * const char *EdsLib_DisplayDB_GetNameByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t
     * SubIndex);
     */
    const char *Name;

    UtAssert_NOT_NULL(Name = EdsLib_DisplayDB_GetNameByIndex(&UT_DATABASE, UT_EDS_CMD1_ID, 1));
    UtAssert_STRINGBUF_EQ(Name, -1, "Cmd1Data1", -1);
}

static void Test_EntityCallback(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc)
{
}

void Test_EdsLib_DisplayDB_IterateBaseEntities(void)
{
    /* Test Case for:
     * void EdsLib_DisplayDB_IterateBaseEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     *      EdsLib_EntityCallback_t Callback, void *Arg);
     */
    UtAssert_VOIDCALL(EdsLib_DisplayDB_IterateBaseEntities(&UT_DATABASE, UT_EDS_CMD1_ID, Test_EntityCallback, NULL));
}

void Test_EdsLib_DisplayDB_IterateAllEntities(void)
{
    /* Test Case for:
     * void EdsLib_DisplayDB_IterateAllEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     * EdsLib_EntityCallback_t Callback, void *Arg);
     */
    UtAssert_VOIDCALL(EdsLib_DisplayDB_IterateAllEntities(&UT_DATABASE, UT_EDS_CMD1_ID, Test_EntityCallback, NULL));
}

void Test_EdsLib_DisplayDB_LocateSubEntity(void)
{
    /* Test Case for:
     * int32_t EdsLib_DisplayDB_LocateSubEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name,
     * EdsLib_DataTypeDB_EntityInfo_t *CompInfo);
     */
    EdsLib_DataTypeDB_EntityInfo_t CompInfo;

    UtAssert_INT32_EQ(EdsLib_DisplayDB_LocateSubEntity(&UT_DATABASE, UT_EDS_CMD1_ID, "Cmd1Data1", &CompInfo),
                      EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(CompInfo.EdsId, UT_EDS_UINT8_ID);
    UtAssert_UINT32_EQ(CompInfo.Offset.Bits, 64);
    UtAssert_UINT32_EQ(CompInfo.Offset.Bytes, 8);
    UtAssert_UINT32_EQ(CompInfo.MaxSize.Bits, 8);
    UtAssert_UINT32_EQ(CompInfo.MaxSize.Bytes, 4);
}

void Test_EdsLib_Generate_Hexdump(void)
{
    /* Test Case for:
     * void EdsLib_Generate_Hexdump(void *output, const uint8_t *DataPtr, uint16_t DisplayOffset, uint16_t Count);
     */
    const uint8_t Data[] = { 0x0A, 0x0B, 0x0C, 0x0D };
    UtAssert_VOIDCALL(EdsLib_Generate_Hexdump(stdout, Data, 0, sizeof(Data)));
}

void Test_EdsLib_Scalar_ToString(void)
{
    /* Test Case for:
     * int32_t EdsLib_Scalar_ToString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *OutputBuffer, uint32_t
     * BufferSize, const void *SrcPtr);
     */
    char                                 Buffer[32];
    EdsDataType_CCSDS_SOIS_SEDS_UINT32_t Val;

    Val = 6543;
    UtAssert_INT32_EQ(EdsLib_Scalar_ToString(&UT_DATABASE, UT_EDS_UINT32_ID, Buffer, sizeof(Buffer), &Val),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_Scalar_FromString(void)
{
    /* Test Case for:
     * int32_t EdsLib_Scalar_FromString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr, const char
     * *SrcString);
     */
    EdsDataType_CCSDS_SOIS_SEDS_UINT32_t Val;

    Val = 0;
    UtAssert_INT32_EQ(EdsLib_Scalar_FromString(&UT_DATABASE, UT_EDS_UINT32_ID, &Val, "2345"), EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(Val, 2345);
}

void Test_EdsLib_DisplayDB_GetEnumLabel(void)
{
    /* Test Case for:
     * const char *EdsLib_DisplayDB_GetEnumLabel(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const
     * EdsLib_GenericValueBuffer_t *ValueBuffer);
     */
    EdsLib_GenericValueBuffer_t ValueBuffer;
    const char                 *Label;

    memset(&ValueBuffer, 0, sizeof(ValueBuffer));
    ValueBuffer.ValueType             = EDSLIB_BASICTYPE_UNSIGNED_INT;
    ValueBuffer.Value.UnsignedInteger = 140;
    UtAssert_NOT_NULL(Label = EdsLib_DisplayDB_GetEnumLabel(&UT_DATABASE, UT_EDS_ENUM_ID, &ValueBuffer));
    UtAssert_STRINGBUF_EQ(Label, -1, "C140", -1);
}

void Test_EdsLib_DisplayDB_GetEnumValue(void)
{
    /* Test Case for:
     * void EdsLib_DisplayDB_GetEnumValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *String,
     * EdsLib_GenericValueBuffer_t *ValueBuffer);
     */
    EdsLib_GenericValueBuffer_t ValueBuffer;

    memset(&ValueBuffer, 0, sizeof(ValueBuffer));
    UtAssert_VOIDCALL(EdsLib_DisplayDB_GetEnumValue(&UT_DATABASE, UT_EDS_ENUM_ID, "C142", &ValueBuffer));
    UtAssert_UINT32_EQ(ValueBuffer.Value.UnsignedInteger, 142);
}

static void Test_SymbolCallback(void *Arg, const char *SymbolName, int32_t SymbolValue)
{
}

void Test_EdsLib_DisplayDB_IterateEnumValues(void)
{
    /* Test Case for:
     * void EdsLib_DisplayDB_IterateEnumValues(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     * EdsLib_SymbolCallback_t Callback, void *Arg);
     */
    UtAssert_VOIDCALL(EdsLib_DisplayDB_IterateEnumValues(&UT_DATABASE, UT_EDS_ENUM_ID, Test_SymbolCallback, NULL));
}

void Test_EdsLib_DisplayDB_GetEnumValueByIndex(void)
{
    /* Test Case for:
     * intmax_t EdsLib_DisplayDB_GetEnumValueByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t
     * Index);
     */
    UtAssert_INT32_EQ(EdsLib_DisplayDB_GetEnumValueByIndex(&UT_DATABASE, UT_EDS_ENUM_ID, 3), 143);
}

void Test_EdsLib_DisplayDB_GetEnumLabelByIndex(void)
{
    /* Test Case for:
     * const char *EdsLib_DisplayDB_GetEnumLabelByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t
     * Index, char *Buffer, uint32_t BufferSize);
     */
    char        Buffer[32];
    const char *Label;

    UtAssert_NOT_NULL(
        Label = EdsLib_DisplayDB_GetEnumLabelByIndex(&UT_DATABASE, UT_EDS_ENUM_ID, 1, Buffer, sizeof(Buffer)));
    UtAssert_STRINGBUF_EQ(Label, -1, "C141", -1);
}

void Test_EdsLib_DisplayDB_Base64Encode(void)
{
    /* Test Case for:
     * void EdsLib_DisplayDB_Base64Encode(char *Out, uint32_t OutputLenBytes, const uint8_t *In, uint32_t InputLenBits);
     */
    char          Buffer[32];
    const uint8_t Message[] = "hello, world";

    UtAssert_VOIDCALL(EdsLib_DisplayDB_Base64Encode(Buffer, sizeof(Buffer), Message, 8 * sizeof(Message)));
    UtAssert_STRINGBUF_EQ(Buffer, sizeof(Buffer), "aGVsbG8sIHdvcmxk", -1);
}

void Test_EdsLib_DisplayDB_Base64Decode(void)
{
    /* Test Case for:
     * void EdsLib_DisplayDB_Base64Decode(uint8_t *Out, uint32_t OutputLenBits, const char *In);
     */
    const char Buffer[32] = "aGVsbG8sIHdvcmxk";
    uint8_t    Message[32];

    UtAssert_VOIDCALL(EdsLib_DisplayDB_Base64Decode(Message, 8 * sizeof(Message), Buffer));
    UtAssert_STRINGBUF_EQ((const char *)Message, sizeof(Message), "hello, world", 12);
}

void UtTest_Setup(void)
{
    UtTest_Add(Test_EdsLib_DisplayDB_Initialize, NULL, NULL, "EdsLib_DisplayDB_Initialize");
    UtTest_Add(Test_EdsLib_DisplayDB_GetEdsName, NULL, NULL, "EdsLib_DisplayDB_GetEdsName");
    UtTest_Add(Test_EdsLib_DisplayDB_GetBaseName, NULL, NULL, "EdsLib_DisplayDB_GetBaseName");
    UtTest_Add(Test_EdsLib_DisplayDB_GetNamespace, NULL, NULL, "EdsLib_DisplayDB_GetNamespace");
    UtTest_Add(Test_EdsLib_DisplayDB_GetTypeName, NULL, NULL, "EdsLib_DisplayDB_GetTypeName");
    UtTest_Add(Test_EdsLib_DisplayDB_GetDisplayHint, NULL, NULL, "EdsLib_DisplayDB_GetDisplayHint");
    UtTest_Add(Test_EdsLib_DisplayDB_LookupTypeName, NULL, NULL, "EdsLib_DisplayDB_LookupTypeName");
    UtTest_Add(Test_EdsLib_DisplayDB_GetIndexByName, NULL, NULL, "EdsLib_DisplayDB_GetIndexByName");
    UtTest_Add(Test_EdsLib_DisplayDB_GetNameByIndex, NULL, NULL, "EdsLib_DisplayDB_GetNameByIndex");
    UtTest_Add(Test_EdsLib_DisplayDB_IterateBaseEntities, NULL, NULL, "EdsLib_DisplayDB_IterateBaseEntities");
    UtTest_Add(Test_EdsLib_DisplayDB_IterateAllEntities, NULL, NULL, "EdsLib_DisplayDB_IterateAllEntities");
    UtTest_Add(Test_EdsLib_DisplayDB_LocateSubEntity, NULL, NULL, "EdsLib_DisplayDB_LocateSubEntity");
    UtTest_Add(Test_EdsLib_Generate_Hexdump, NULL, NULL, "EdsLib_Generate_Hexdump");
    UtTest_Add(Test_EdsLib_Scalar_ToString, NULL, NULL, "EdsLib_Scalar_ToString");
    UtTest_Add(Test_EdsLib_Scalar_FromString, NULL, NULL, "EdsLib_Scalar_FromString");
    UtTest_Add(Test_EdsLib_DisplayDB_GetEnumLabel, NULL, NULL, "EdsLib_DisplayDB_GetEnumLabel");
    UtTest_Add(Test_EdsLib_DisplayDB_GetEnumValue, NULL, NULL, "EdsLib_DisplayDB_GetEnumValue");
    UtTest_Add(Test_EdsLib_DisplayDB_IterateEnumValues, NULL, NULL, "EdsLib_DisplayDB_IterateEnumValues");
    UtTest_Add(Test_EdsLib_DisplayDB_GetEnumValueByIndex, NULL, NULL, "EdsLib_DisplayDB_GetEnumValueByIndex");
    UtTest_Add(Test_EdsLib_DisplayDB_GetEnumLabelByIndex, NULL, NULL, "EdsLib_DisplayDB_GetEnumLabelByIndex");
    UtTest_Add(Test_EdsLib_DisplayDB_Base64Encode, NULL, NULL, "EdsLib_DisplayDB_Base64Encode");
    UtTest_Add(Test_EdsLib_DisplayDB_Base64Decode, NULL, NULL, "EdsLib_DisplayDB_Base64Decode");
}
