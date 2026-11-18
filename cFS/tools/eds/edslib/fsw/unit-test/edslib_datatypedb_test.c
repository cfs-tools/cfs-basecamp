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

const char         *UT_DYNAMIC_APPNAME_TABLE[UT_MAX_INDEX]    = { NULL };
EdsLib_DataTypeDB_t UT_DYNAMIC_DATATYPEDB_TABLE[UT_MAX_INDEX] = { NULL };

EdsLib_DatabaseObject_t UT_DYNAMIC_DB = { .AppTableSize     = UT_MAX_INDEX,
                                          .AppName_Table    = UT_DYNAMIC_APPNAME_TABLE,
                                          .DataTypeDB_Table = UT_DYNAMIC_DATATYPEDB_TABLE,
                                          .DisplayDB_Table  = NULL,
                                          .IntfDB_Table     = NULL };

void Test_EdsLib_DataTypeDB_Initialize(void)
{
    /* Test Case For:
     * void EdsLib_DataTypeDB_Initialize(void);
     */
    UtAssert_VOIDCALL(EdsLib_DataTypeDB_Initialize());
}

void Test_EdsLib_DataTypeDB_GetAppIdx(void)
{
    /* Test Case For:
     * uint16_t EdsLib_DataTypeDB_GetAppIdx(const EdsLib_DataTypeDB_t AppDict);
     */
    UtAssert_UINT16_EQ(EdsLib_DataTypeDB_GetAppIdx(&EDS_PACKAGE_CCSDS_SOIS_SEDS_DATATYPE_DB), UT_INDEX_CCSDS_SOIS_SEDS);
    UtAssert_UINT16_EQ(EdsLib_DataTypeDB_GetAppIdx(&EDS_PACKAGE_UTHDR_DATATYPE_DB), UT_INDEX_UTHDR);
    UtAssert_UINT16_EQ(EdsLib_DataTypeDB_GetAppIdx(&EDS_PACKAGE_UTAPP_DATATYPE_DB), UT_INDEX_UTAPP);
}

void Test_EdsLib_DataTypeDB_Register(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_Register(EdsLib_DatabaseObject_t *GD, EdsLib_DataTypeDB_t AppDict);
     */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_Register(&UT_DYNAMIC_DB, &EDS_PACKAGE_CCSDS_SOIS_SEDS_DATATYPE_DB),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_Register(&UT_DYNAMIC_DB, &EDS_PACKAGE_UTHDR_DATATYPE_DB), EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_Unregister(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_Unregister(EdsLib_DatabaseObject_t *GD, uint16_t AppIdx);
     */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_Unregister(&UT_DYNAMIC_DB, UT_INDEX_CCSDS_SOIS_SEDS), EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_Unregister(&UT_DYNAMIC_DB, UT_INDEX_UTHDR), EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_Unregister(&UT_DYNAMIC_DB, UT_INDEX_UTAPP), EDSLIB_FAILURE);
}

void Test_EdsLib_DataTypeDB_GetTypeInfo(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_GetTypeInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     * EdsLib_DataTypeDB_TypeInfo_t *TypeInfo);
     */
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetTypeInfo(&UT_DATABASE, UT_EDS_UINT32_ID, &TypeInfo), EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(TypeInfo.ElemType, EDSLIB_BASICTYPE_UNSIGNED_INT);
    UtAssert_UINT32_EQ(TypeInfo.Size.Bits, 32);
    UtAssert_UINT32_EQ(TypeInfo.Size.Bytes, 4);

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetTypeInfo(&UT_DATABASE, UT_EDS_INT8_ID, &TypeInfo), EDSLIB_SUCCESS);
    UtAssert_UINT32_EQ(TypeInfo.ElemType, EDSLIB_BASICTYPE_SIGNED_INT);
    UtAssert_UINT32_EQ(TypeInfo.Size.Bits, 8);
    UtAssert_UINT32_EQ(TypeInfo.Size.Bytes, 1);

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetTypeInfo(&UT_DATABASE, EDSLIB_ID_INVALID, &TypeInfo), EDSLIB_FAILURE);
}

void Test_EdsLib_DataTypeDB_GetMemberByIndex(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_GetMemberByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t
     * SubIndex, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo);
     */
    EdsLib_DataTypeDB_EntityInfo_t EntityInfo;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetMemberByIndex(&UT_DATABASE, UT_EDS_CMD1_ID, 1, &EntityInfo), EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_GetMemberByNativeOffset(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_GetMemberByNativeOffset(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint32_t
     * ByteOffset, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo);
     */
    EdsLib_DataTypeDB_EntityInfo_t MemberInfo;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetMemberByNativeOffset(&UT_DATABASE, UT_EDS_CMD1_ID, 0, &MemberInfo),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_GetDerivedTypeById(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_GetDerivedTypeById(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t
     * DerivId, EdsLib_Id_t *DerivedEdsId);
     */
    EdsLib_Id_t DerivedEdsId;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetDerivedTypeById(&UT_DATABASE, UT_EDS_CMDHDR_ID, 0, &DerivedEdsId),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_GetConstraintEntity(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_GetConstraintEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t
     * ConstraintIdx, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo);
     */
    EdsLib_DataTypeDB_EntityInfo_t MemberInfo;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetConstraintEntity(&UT_DATABASE, UT_EDS_CMDHDR_ID, 0, &MemberInfo),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_GetDerivedInfo(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_GetDerivedInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     * EdsLib_DataTypeDB_DerivedTypeInfo_t *DerivInfo);
     */

    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_GetDerivedInfo(&UT_DATABASE, UT_EDS_CMDHDR_ID, &DerivInfo), EDSLIB_SUCCESS);
}

void UtConstraintCallback(const EdsLib_DatabaseObject_t        *GD,
                          const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
                          EdsLib_GenericValueBuffer_t          *ConstraintValue,
                          void                                 *Arg)
{
}

void Test_EdsLib_DataTypeDB_ConstraintIterator(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_ConstraintIterator(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t
     * DerivedId, EdsLib_ConstraintCallback_t Callback, void *CbArg);
     */

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_ConstraintIterator(&UT_DATABASE,
                                                           UT_EDS_CMDHDR_ID,
                                                           UT_EDS_CMD1_ID,
                                                           UtConstraintCallback,
                                                           NULL),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_BaseCheck(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_BaseCheck(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t
     * DerivedId);
     */

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_BaseCheck(&UT_DATABASE, UT_EDS_CMDHDR_ID, UT_EDS_CMD1_ID), EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_PackCompleteObject(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_PackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
     *     void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsLib_Id_t                      EdsId;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&PackBuf, 0xAA, sizeof(PackBuf));
    EdsId = UT_EDS_CMD1_ID;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId,
                                                           &PackBuf,
                                                           &CmdBuf,
                                                           sizeof(PackBuf) * 8,
                                                           sizeof(CmdBuf)),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_PackPartialObjectVarSize(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_PackPartialObjectVarSize(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
     *    void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize, uint32_t
     * StartingBit);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsLib_Id_t                      EdsId;
    EdsLib_SizeInfo_t                MaxSize;
    EdsLib_SizeInfo_t                ProcessedSize;

    memset(&CmdBuf, 0x1, sizeof(CmdBuf));
    memset(&PackBuf, 0xAA, sizeof(PackBuf));
    memset(&ProcessedSize, 0, sizeof(ProcessedSize));
    EdsId = UT_EDS_CMD1_ID;

    MaxSize.Bits  = EdsLib_OCTETS_TO_BITS(sizeof(PackBuf));
    MaxSize.Bytes = sizeof(CmdBuf);

    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackPartialObjectVarSize(&UT_DATABASE, &EdsId, &PackBuf, &CmdBuf, &MaxSize, &ProcessedSize),
        EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_PackPartialObject(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_PackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
     *    void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize, uint32_t
     * StartingBit);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsLib_Id_t                      EdsId;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&PackBuf, 0xAA, sizeof(PackBuf));
    EdsId = UT_EDS_CMD1_ID;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackPartialObject(&UT_DATABASE,
                                                          &EdsId,
                                                          &PackBuf,
                                                          &CmdBuf,
                                                          sizeof(PackBuf) * 8,
                                                          sizeof(CmdBuf),
                                                          0),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_UnpackCompleteObject(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_UnpackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
     *     void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsLib_Id_t                      EdsId;

    memset(&CmdBuf, 0xAA, sizeof(CmdBuf));
    memset(&PackBuf, 0, sizeof(PackBuf));
    EdsId = UT_EDS_CMDHDR_ID;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_UnpackCompleteObject(&UT_DATABASE,
                                                             &EdsId,
                                                             &CmdBuf,
                                                             &PackBuf,
                                                             sizeof(CmdBuf),
                                                             sizeof(PackBuf) * 8),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_UnpackPartialObjectVarSize(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_UnpackPartialObjectVarSize(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
     *     void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize, uint32_t
     * StartingByte);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsLib_Id_t                      EdsId;
    EdsLib_SizeInfo_t                MaxSize;
    EdsLib_SizeInfo_t                ProcessedSize;

    memset(&CmdBuf, 0xAA, sizeof(CmdBuf));
    memset(&PackBuf, 0, sizeof(PackBuf));
    memset(&ProcessedSize, 0, sizeof(ProcessedSize));
    EdsId = UT_EDS_CMDHDR_ID;

    MaxSize.Bits  = EdsLib_OCTETS_TO_BITS(sizeof(PackBuf));
    MaxSize.Bytes = sizeof(CmdBuf);

    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_UnpackPartialObjectVarSize(&UT_DATABASE, &EdsId, &CmdBuf, &PackBuf, &MaxSize, &ProcessedSize),
        EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_UnpackPartialObject(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_UnpackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
     *     void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize, uint32_t
     * StartingByte);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsLib_Id_t                      EdsId;

    memset(&CmdBuf, 0xAA, sizeof(CmdBuf));
    memset(&PackBuf, 0, sizeof(PackBuf));
    EdsId = UT_EDS_CMDHDR_ID;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_UnpackPartialObject(&UT_DATABASE,
                                                            &EdsId,
                                                            &CmdBuf,
                                                            &PackBuf,
                                                            sizeof(CmdBuf),
                                                            sizeof(PackBuf) * 8,
                                                            0),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_FinalizePackedObjectVarSize(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_FinalizePackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void
     * *PackedData);
     */
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsLib_SizeInfo_t                ProcessedSize;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&PackBuf, 0, sizeof(PackBuf));

    ProcessedSize.Bits  = EdsLib_OCTETS_TO_BITS(sizeof(PackBuf));
    ProcessedSize.Bytes = sizeof(CmdBuf);

    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_FinalizePackedObjectVarSize(&UT_DATABASE, UT_EDS_CMD1_ID, &CmdBuf, &PackBuf, &ProcessedSize),
        EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_FinalizePackedObject(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_FinalizePackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void
     * *PackedData);
     */
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;

    memset(&PackBuf, 0, sizeof(PackBuf));

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_FinalizePackedObject(&UT_DATABASE, UT_EDS_CMD1_ID, &PackBuf), EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     *     void *UnpackedObj, const void *PackedObj, uint32_t RecomputeFields);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;
    EdsLib_SizeInfo_t                ProcessedSize;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&PackBuf, 0, sizeof(PackBuf));

    ProcessedSize.Bits  = EdsLib_OCTETS_TO_BITS(sizeof(PackBuf));
    ProcessedSize.Bytes = sizeof(CmdBuf);

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(&UT_DATABASE,
                                                                    UT_EDS_CMD1_ID,
                                                                    &CmdBuf,
                                                                    &PackBuf,
                                                                    0,
                                                                    &ProcessedSize),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_VerifyUnpackedObject(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_VerifyUnpackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     *     void *UnpackedObj, const void *PackedObj, uint32_t RecomputeFields);
     */
    EdsDataType_UtApp_Cmd1_t         CmdBuf;
    EdsPackedBuffer_UtHdr_UtCmdHdr_t PackBuf;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&PackBuf, 0, sizeof(PackBuf));

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_VerifyUnpackedObject(&UT_DATABASE, UT_EDS_CMD1_ID, &CmdBuf, &PackBuf, 0),
                      EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_InitializeNativeObject(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_InitializeNativeObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void
     * *UnpackedObj);
     */
    EdsDataType_UtApp_Cmd1_t CmdBuf;

    memset(&CmdBuf, 0, sizeof(CmdBuf));

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_InitializeNativeObject(&UT_DATABASE, UT_EDS_CMD1_ID, &CmdBuf), EDSLIB_SUCCESS);
}

void Test_EdsLib_DataTypeDB_LoadValue(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_LoadValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
     * EdsLib_GenericValueBuffer_t *DestBuffer, const void *SrcPtr);
     */
    EdsDataType_UtApp_Cmd1_t    CmdBuf;
    EdsLib_GenericValueBuffer_t DestBuffer;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&DestBuffer, 0, sizeof(DestBuffer));

    CmdBuf.Cmd1Data2 = 5678;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_LoadValue(&UT_DATABASE, UT_EDS_UINT32_ID, &DestBuffer, &CmdBuf.Cmd1Data2),
                      EDSLIB_SUCCESS);

    UtAssert_EQ(EdsLib_BasicType_t, DestBuffer.ValueType, EDSLIB_BASICTYPE_UNSIGNED_INT);
    UtAssert_EQ(unsigned int, DestBuffer.Value.UnsignedInteger, 5678);
}

void Test_EdsLib_DataTypeDB_StoreValue(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_StoreValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr,
     * EdsLib_GenericValueBuffer_t *SrcBuffer);
     */
    EdsDataType_UtApp_Cmd1_t    CmdBuf;
    EdsLib_GenericValueBuffer_t SrcBuffer;

    memset(&CmdBuf, 0xCC, sizeof(CmdBuf));
    memset(&SrcBuffer, 0, sizeof(SrcBuffer));

    SrcBuffer.ValueType             = EDSLIB_BASICTYPE_UNSIGNED_INT;
    SrcBuffer.Value.UnsignedInteger = 1234;

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_StoreValue(&UT_DATABASE, UT_EDS_UINT32_ID, &CmdBuf.Cmd1Data2, &SrcBuffer),
                      EDSLIB_SUCCESS);

    UtAssert_UINT32_EQ(CmdBuf.Cmd1Data2, 1234);
}

void Test_EdsLib_DataTypeDB_IdentifyBufferWithSize(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_IdentifyBufferWithSize(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const void
     * *BufferPtr, size_t BufferSize, EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo);
     */
    EdsNativeBuffer_UtHdr_UtCmdHdr_t         CmdBuf;
    EdsLib_DataTypeDB_DerivativeObjectInfo_t DerivObjInfo;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&DerivObjInfo, 0, sizeof(DerivObjInfo));

    /* Setup */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_InitializeNativeObject(&UT_DATABASE, UT_EDS_CMD1_ID, &CmdBuf), EDSLIB_SUCCESS);

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_IdentifyBufferWithSize(&UT_DATABASE,
                                                               UT_EDS_CMDHDR_ID,
                                                               &CmdBuf,
                                                               sizeof(CmdBuf),
                                                               &DerivObjInfo),
                      EDSLIB_SUCCESS);

    UtAssert_UINT32_EQ(DerivObjInfo.DerivativeTableIndex, 0);
}

void Test_EdsLib_DataTypeDB_IdentifyBuffer(void)
{
    /* Test Case For:
     * int32_t EdsLib_DataTypeDB_IdentifyBuffer(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const void
     * *MessageBuffer, EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo);
     */
    EdsNativeBuffer_UtHdr_UtCmdHdr_t         CmdBuf;
    EdsLib_DataTypeDB_DerivativeObjectInfo_t DerivObjInfo;

    memset(&CmdBuf, 0, sizeof(CmdBuf));
    memset(&DerivObjInfo, 0, sizeof(DerivObjInfo));

    /* Setup */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_InitializeNativeObject(&UT_DATABASE, UT_EDS_CMD1_ID, &CmdBuf), EDSLIB_SUCCESS);

    UtAssert_INT32_EQ(EdsLib_DataTypeDB_IdentifyBuffer(&UT_DATABASE, UT_EDS_CMDHDR_ID, &CmdBuf, &DerivObjInfo),
                      EDSLIB_SUCCESS);

    UtAssert_UINT32_EQ(DerivObjInfo.DerivativeTableIndex, 0);
}

void Test_EdsLib_DataTypeConvert(void)
{
    /* Test Case For:
     * void EdsLib_DataTypeConvert(EdsLib_GenericValueBuffer_t *ValueBuff, EdsLib_BasicType_t DesiredType);
     */
    EdsLib_GenericValueBuffer_t Buffer;

    memset(&Buffer, 0, sizeof(Buffer));

    Buffer.ValueType           = EDSLIB_BASICTYPE_SIGNED_INT;
    Buffer.Value.SignedInteger = -10;

    UtAssert_VOIDCALL(EdsLib_DataTypeConvert(&Buffer, EDSLIB_BASICTYPE_FLOAT));

    UtAssert_EQ(EdsLib_BasicType_t, Buffer.ValueType, EDSLIB_BASICTYPE_FLOAT);
    UtAssert_EQ(double, Buffer.Value.FloatingPoint, -10.0);
}

void UtTest_Setup(void)
{
    UtTest_Add(Test_EdsLib_DataTypeDB_Initialize, NULL, NULL, "EdsLib_DataTypeDB_Initialize");
    UtTest_Add(Test_EdsLib_DataTypeDB_GetAppIdx, NULL, NULL, "EdsLib_DataTypeDB_GetAppIdx");
    UtTest_Add(Test_EdsLib_DataTypeDB_Register, NULL, NULL, "EdsLib_DataTypeDB_Register");
    UtTest_Add(Test_EdsLib_DataTypeDB_Unregister, NULL, NULL, "EdsLib_DataTypeDB_Unregister");
    UtTest_Add(Test_EdsLib_DataTypeDB_GetTypeInfo, NULL, NULL, "EdsLib_DataTypeDB_GetTypeInfo");
    UtTest_Add(Test_EdsLib_DataTypeDB_GetMemberByIndex, NULL, NULL, "EdsLib_DataTypeDB_GetMemberByIndex");
    UtTest_Add(Test_EdsLib_DataTypeDB_GetMemberByNativeOffset, NULL, NULL, "EdsLib_DataTypeDB_GetMemberByNativeOffset");
    UtTest_Add(Test_EdsLib_DataTypeDB_GetDerivedTypeById, NULL, NULL, "EdsLib_DataTypeDB_GetDerivedTypeById");
    UtTest_Add(Test_EdsLib_DataTypeDB_GetConstraintEntity, NULL, NULL, "EdsLib_DataTypeDB_GetConstraintEntity");
    UtTest_Add(Test_EdsLib_DataTypeDB_GetDerivedInfo, NULL, NULL, "EdsLib_DataTypeDB_GetDerivedInfo");
    UtTest_Add(Test_EdsLib_DataTypeDB_ConstraintIterator, NULL, NULL, "EdsLib_DataTypeDB_ConstraintIterator");
    UtTest_Add(Test_EdsLib_DataTypeDB_BaseCheck, NULL, NULL, "EdsLib_DataTypeDB_BaseCheck");
    UtTest_Add(Test_EdsLib_DataTypeDB_PackCompleteObject, NULL, NULL, "EdsLib_DataTypeDB_PackCompleteObject");
    UtTest_Add(Test_EdsLib_DataTypeDB_PackPartialObjectVarSize,
               NULL,
               NULL,
               "EdsLib_DataTypeDB_PackPartialObjectVarSize");
    UtTest_Add(Test_EdsLib_DataTypeDB_UnpackCompleteObject, NULL, NULL, "EdsLib_DataTypeDB_UnpackCompleteObject");
    UtTest_Add(Test_EdsLib_DataTypeDB_UnpackPartialObjectVarSize,
               NULL,
               NULL,
               "EdsLib_DataTypeDB_UnpackPartialObjectVarSize");
    UtTest_Add(Test_EdsLib_DataTypeDB_FinalizePackedObjectVarSize,
               NULL,
               NULL,
               "EdsLib_DataTypeDB_FinalizePackedObjectVarSize");
    UtTest_Add(Test_EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize,
               NULL,
               NULL,
               "EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize");
    UtTest_Add(Test_EdsLib_DataTypeDB_InitializeNativeObject, NULL, NULL, "EdsLib_DataTypeDB_InitializeNativeObject");
    UtTest_Add(Test_EdsLib_DataTypeDB_LoadValue, NULL, NULL, "EdsLib_DataTypeDB_LoadValue");
    UtTest_Add(Test_EdsLib_DataTypeDB_StoreValue, NULL, NULL, "EdsLib_DataTypeDB_StoreValue");
    UtTest_Add(Test_EdsLib_DataTypeDB_IdentifyBufferWithSize, NULL, NULL, "EdsLib_DataTypeDB_IdentifyBufferWithSize");
    UtTest_Add(Test_EdsLib_DataTypeDB_IdentifyBuffer, NULL, NULL, "EdsLib_DataTypeDB_IdentifyBuffer");
    UtTest_Add(Test_EdsLib_DataTypeConvert, NULL, NULL, "EdsLib_DataTypeConvert");
}
