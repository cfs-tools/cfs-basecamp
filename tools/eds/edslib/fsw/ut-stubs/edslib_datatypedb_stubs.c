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
 * Auto-Generated stub implementations for functions defined in edslib_datatypedb header
 */

#include "edslib_datatypedb.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeConvert()
 * ----------------------------------------------------
 */
void EdsLib_DataTypeConvert(EdsLib_GenericValueBuffer_t *ValueBuff, EdsLib_BasicType_t DesiredType)
{
    UT_GenStub_AddParam(EdsLib_DataTypeConvert, EdsLib_GenericValueBuffer_t *, ValueBuff);
    UT_GenStub_AddParam(EdsLib_DataTypeConvert, EdsLib_BasicType_t, DesiredType);

    UT_GenStub_Execute(EdsLib_DataTypeConvert, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_BaseCheck()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_BaseCheck(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t DerivedId)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_BaseCheck, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_BaseCheck, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_BaseCheck, EdsLib_Id_t, BaseId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_BaseCheck, EdsLib_Id_t, DerivedId);

    UT_GenStub_Execute(EdsLib_DataTypeDB_BaseCheck, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_BaseCheck, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_ConstraintIterator()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_ConstraintIterator(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId,
                                             EdsLib_Id_t DerivedId, EdsLib_ConstraintCallback_t Callback, void *CbArg)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_ConstraintIterator, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_ConstraintIterator, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_ConstraintIterator, EdsLib_Id_t, BaseId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_ConstraintIterator, EdsLib_Id_t, DerivedId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_ConstraintIterator, EdsLib_ConstraintCallback_t, Callback);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_ConstraintIterator, void *, CbArg);

    UT_GenStub_Execute(EdsLib_DataTypeDB_ConstraintIterator, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_ConstraintIterator, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_FinalizePackedObject()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_FinalizePackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *PackedData)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_FinalizePackedObject, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_FinalizePackedObject, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_FinalizePackedObject, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_FinalizePackedObject, void *, PackedData);

    UT_GenStub_Execute(EdsLib_DataTypeDB_FinalizePackedObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_FinalizePackedObject, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_GetAppIdx()
 * ----------------------------------------------------
 */
uint16_t EdsLib_DataTypeDB_GetAppIdx(const EdsLib_DataTypeDB_t AppDict)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_GetAppIdx, uint16_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetAppIdx, const EdsLib_DataTypeDB_t, AppDict);

    UT_GenStub_Execute(EdsLib_DataTypeDB_GetAppIdx, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_GetAppIdx, uint16_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_GetConstraintEntity()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_GetConstraintEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                              uint16_t ConstraintIdx, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_GetConstraintEntity, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetConstraintEntity, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetConstraintEntity, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetConstraintEntity, uint16_t, ConstraintIdx);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetConstraintEntity, EdsLib_DataTypeDB_EntityInfo_t *, MemberInfo);

    UT_GenStub_Execute(EdsLib_DataTypeDB_GetConstraintEntity, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_GetConstraintEntity, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_GetDerivedInfo()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_GetDerivedInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                         EdsLib_DataTypeDB_DerivedTypeInfo_t *DerivInfo)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_GetDerivedInfo, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetDerivedInfo, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetDerivedInfo, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetDerivedInfo, EdsLib_DataTypeDB_DerivedTypeInfo_t *, DerivInfo);

    UT_GenStub_Execute(EdsLib_DataTypeDB_GetDerivedInfo, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_GetDerivedInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_GetDerivedTypeById()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_GetDerivedTypeById(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t DerivId,
                                             EdsLib_Id_t *DerivedEdsId)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_GetDerivedTypeById, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetDerivedTypeById, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetDerivedTypeById, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetDerivedTypeById, uint16_t, DerivId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetDerivedTypeById, EdsLib_Id_t *, DerivedEdsId);

    UT_GenStub_Execute(EdsLib_DataTypeDB_GetDerivedTypeById, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_GetDerivedTypeById, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_GetMemberByIndex()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_GetMemberByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t SubIndex,
                                           EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_GetMemberByIndex, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByIndex, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByIndex, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByIndex, uint16_t, SubIndex);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByIndex, EdsLib_DataTypeDB_EntityInfo_t *, MemberInfo);

    UT_GenStub_Execute(EdsLib_DataTypeDB_GetMemberByIndex, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_GetMemberByIndex, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_GetMemberByNativeOffset()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_GetMemberByNativeOffset(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                                  uint32_t ByteOffset, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_GetMemberByNativeOffset, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByNativeOffset, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByNativeOffset, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByNativeOffset, uint32_t, ByteOffset);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetMemberByNativeOffset, EdsLib_DataTypeDB_EntityInfo_t *, MemberInfo);

    UT_GenStub_Execute(EdsLib_DataTypeDB_GetMemberByNativeOffset, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_GetMemberByNativeOffset, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_GetTypeInfo()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_GetTypeInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                      EdsLib_DataTypeDB_TypeInfo_t *TypeInfo)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_GetTypeInfo, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetTypeInfo, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetTypeInfo, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_GetTypeInfo, EdsLib_DataTypeDB_TypeInfo_t *, TypeInfo);

    UT_GenStub_Execute(EdsLib_DataTypeDB_GetTypeInfo, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_GetTypeInfo, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_IdentifyBuffer()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_IdentifyBuffer(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                         const void *                              MessageBuffer,
                                         EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_IdentifyBuffer, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_IdentifyBuffer, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_IdentifyBuffer, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_IdentifyBuffer, const void *, MessageBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_IdentifyBuffer, EdsLib_DataTypeDB_DerivativeObjectInfo_t *, DerivObjInfo);

    UT_GenStub_Execute(EdsLib_DataTypeDB_IdentifyBuffer, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_IdentifyBuffer, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_Initialize()
 * ----------------------------------------------------
 */
void EdsLib_DataTypeDB_Initialize(void)
{

    UT_GenStub_Execute(EdsLib_DataTypeDB_Initialize, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_InitializeNativeObject()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_InitializeNativeObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                                 void *UnpackedObj)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_InitializeNativeObject, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_InitializeNativeObject, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_InitializeNativeObject, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_InitializeNativeObject, void *, UnpackedObj);

    UT_GenStub_Execute(EdsLib_DataTypeDB_InitializeNativeObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_InitializeNativeObject, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_LoadValue()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_LoadValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                    EdsLib_GenericValueBuffer_t *DestBuffer, const void *SrcPtr)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_LoadValue, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_LoadValue, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_LoadValue, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_LoadValue, EdsLib_GenericValueBuffer_t *, DestBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_LoadValue, const void *, SrcPtr);

    UT_GenStub_Execute(EdsLib_DataTypeDB_LoadValue, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_LoadValue, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_PackCompleteObject()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_PackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId, void *DestBuffer,
                                             const void *SourceBuffer, uint32_t MaxPackedBitSize,
                                             uint32_t SourceByteSize)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_PackCompleteObject, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackCompleteObject, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackCompleteObject, EdsLib_Id_t *, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackCompleteObject, void *, DestBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackCompleteObject, const void *, SourceBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackCompleteObject, uint32_t, MaxPackedBitSize);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackCompleteObject, uint32_t, SourceByteSize);

    UT_GenStub_Execute(EdsLib_DataTypeDB_PackCompleteObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_PackCompleteObject, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_PackPartialObject()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_PackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId, void *DestBuffer,
                                            const void *SourceBuffer, uint32_t MaxPackedBitSize,
                                            uint32_t SourceByteSize, uint32_t StartingBit)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_PackPartialObject, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackPartialObject, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackPartialObject, EdsLib_Id_t *, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackPartialObject, void *, DestBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackPartialObject, const void *, SourceBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackPartialObject, uint32_t, MaxPackedBitSize);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackPartialObject, uint32_t, SourceByteSize);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_PackPartialObject, uint32_t, StartingBit);

    UT_GenStub_Execute(EdsLib_DataTypeDB_PackPartialObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_PackPartialObject, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_Register()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_Register(EdsLib_DatabaseObject_t *GD, EdsLib_DataTypeDB_t AppDict)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_Register, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_Register, EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_Register, EdsLib_DataTypeDB_t, AppDict);

    UT_GenStub_Execute(EdsLib_DataTypeDB_Register, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_Register, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_StoreValue()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_StoreValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr,
                                     EdsLib_GenericValueBuffer_t *SrcBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_StoreValue, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_StoreValue, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_StoreValue, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_StoreValue, void *, DestPtr);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_StoreValue, EdsLib_GenericValueBuffer_t *, SrcBuffer);

    UT_GenStub_Execute(EdsLib_DataTypeDB_StoreValue, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_StoreValue, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_UnpackCompleteObject()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_UnpackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId, void *DestBuffer,
                                               const void *SourceBuffer, uint32_t MaxNativeByteSize,
                                               uint32_t SourceBitSize)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_UnpackCompleteObject, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackCompleteObject, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackCompleteObject, EdsLib_Id_t *, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackCompleteObject, void *, DestBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackCompleteObject, const void *, SourceBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackCompleteObject, uint32_t, MaxNativeByteSize);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackCompleteObject, uint32_t, SourceBitSize);

    UT_GenStub_Execute(EdsLib_DataTypeDB_UnpackCompleteObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_UnpackCompleteObject, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_UnpackPartialObject()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_UnpackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId, void *DestBuffer,
                                              const void *SourceBuffer, uint32_t MaxNativeByteSize,
                                              uint32_t SourceBitSize, uint32_t StartingByte)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_UnpackPartialObject, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackPartialObject, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackPartialObject, EdsLib_Id_t *, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackPartialObject, void *, DestBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackPartialObject, const void *, SourceBuffer);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackPartialObject, uint32_t, MaxNativeByteSize);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackPartialObject, uint32_t, SourceBitSize);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_UnpackPartialObject, uint32_t, StartingByte);

    UT_GenStub_Execute(EdsLib_DataTypeDB_UnpackPartialObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_UnpackPartialObject, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_Unregister()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_Unregister(EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_Unregister, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_Unregister, EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_Unregister, uint16_t, AppIdx);

    UT_GenStub_Execute(EdsLib_DataTypeDB_Unregister, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_Unregister, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DataTypeDB_VerifyUnpackedObject()
 * ----------------------------------------------------
 */
int32_t EdsLib_DataTypeDB_VerifyUnpackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *UnpackedObj,
                                               const void *PackedObj, uint32_t RecomputeFields)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DataTypeDB_VerifyUnpackedObject, int32_t);

    UT_GenStub_AddParam(EdsLib_DataTypeDB_VerifyUnpackedObject, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_VerifyUnpackedObject, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_VerifyUnpackedObject, void *, UnpackedObj);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_VerifyUnpackedObject, const void *, PackedObj);
    UT_GenStub_AddParam(EdsLib_DataTypeDB_VerifyUnpackedObject, uint32_t, RecomputeFields);

    UT_GenStub_Execute(EdsLib_DataTypeDB_VerifyUnpackedObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DataTypeDB_VerifyUnpackedObject, int32_t);
}
