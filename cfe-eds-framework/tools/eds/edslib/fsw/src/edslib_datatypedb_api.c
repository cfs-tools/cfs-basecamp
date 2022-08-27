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
 * \file     edslib_datatypedb_api.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of public EDS API functions.
 * Linked as part of the "basic" data type EDS runtime library
 *
 * This file implements the external facing user API calls.  In general these are just small
 * wrapper functions around the internal implementation function(s) that put everything into
 * the appropriate types for the external API call.
 *
 * The primary difference is that the internal functions generally use direct pointers to
 * the database objects, whereas the external/public API calls use an integer index, which
 * of course is much safer and immune to corruption as it can be validated before use.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "edslib_internal.h"

/*
 * *************************************************************************************
 * Register/Unregister API calls
 * *************************************************************************************
 */

void EdsLib_DataTypeDB_Initialize(void)
{
    EdsLib_ErrorControl_Initialize();
}

uint16_t EdsLib_DataTypeDB_GetAppIdx(const EdsLib_DataTypeDB_t AppDict)
{
    return AppDict->MissionIdx;
}

int32_t EdsLib_DataTypeDB_Register(EdsLib_DatabaseObject_t *GD, EdsLib_DataTypeDB_t AppDict)
{
    if (GD == NULL ||
            GD->DataTypeDB_Table == NULL ||
            AppDict->MissionIdx >= GD->AppTableSize ||
            GD->DataTypeDB_Table[AppDict->MissionIdx] != NULL)
    {
        return EDSLIB_FAILURE;
    }

    GD->DataTypeDB_Table[AppDict->MissionIdx] = AppDict;
    return EDSLIB_SUCCESS;
}

int32_t EdsLib_DataTypeDB_Unregister(EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
{
    if (GD == NULL ||
            GD->DataTypeDB_Table == NULL ||
            AppIdx >= GD->AppTableSize ||
            GD->DataTypeDB_Table[AppIdx] == NULL)
    {
        return EDSLIB_FAILURE;
    }

    GD->DataTypeDB_Table[AppIdx] = NULL;
    return EDSLIB_SUCCESS;
}


/*
 * *************************************************************************************
 * Datatype DB object lookup API calls
 * *************************************************************************************
 */

int32_t EdsLib_DataTypeDB_GetTypeInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_DataTypeDB_TypeInfo_t *TypeInfo)
{
    EdsLib_DatabaseRef_t TempRef;
    const EdsLib_DataTypeDB_Entry_t *DataDictEntry;
    int32_t Status;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDictEntry = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    /* If no storage buffer was specified, then just return
     * whether or not the data type exists.
     */
    if (DataDictEntry == NULL)
    {
        Status = EDSLIB_FAILURE;
    }
    else
    {
        Status = EDSLIB_SUCCESS;
    }

    if (TypeInfo != NULL)
    {
        EdsLib_DataTypeDB_CopyTypeInfo(DataDictEntry, TypeInfo);
    }

    return Status;
}

int32_t EdsLib_DataTypeDB_GetMemberByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t SubIndex, EdsLib_DataTypeDB_EntityInfo_t *CompInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDict;
    const EdsLib_DatabaseRef_t *RefObj;
    EdsLib_DatabaseRef_t TempRef;
    int32_t Result;

    RefObj = NULL;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDict = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (CompInfo != NULL)
    {
        memset(CompInfo, 0, sizeof(*CompInfo));
    }

    if (DataDict != NULL && SubIndex < DataDict->NumSubElements)
    {
        switch(DataDict->BasicType)
        {
        case EDSLIB_BASICTYPE_CONTAINER:
        {
            RefObj = &DataDict->Detail.Container->EntryList[SubIndex].RefObj;

            if (CompInfo != NULL)
            {
                const EdsLib_SizeInfo_t *EndOffset;

                if (SubIndex < (DataDict->NumSubElements - 1))
                {
                    /* not the last entry in the container - peek to the next */
                    EndOffset = &DataDict->Detail.Container->EntryList[SubIndex + 1].Offset;
                }
                else
                {
                    /* last element -- use the parent size */
                    EndOffset = &DataDict->SizeInfo;
                }

                CompInfo->Offset = DataDict->Detail.Container->EntryList[SubIndex].Offset;
                CompInfo->MaxSize.Bytes = (EndOffset->Bytes - CompInfo->Offset.Bytes);
                CompInfo->MaxSize.Bits = (EndOffset->Bits - CompInfo->Offset.Bits);
            }
            break;
        }
        case EDSLIB_BASICTYPE_ARRAY:
        {
            RefObj = &DataDict->Detail.Array->ElementRefObj;

            if (CompInfo != NULL)
            {
                /*
                 * Calculating the start offset this way, although it involves division,
                 * does not require that we actually _have_ the definition of the array
                 * element type loaded.  The total byte size of the entire array _must_
                 * be an integer multiple of the array length, by definition.
                 */

                CompInfo->MaxSize.Bytes = (DataDict->SizeInfo.Bytes / DataDict->NumSubElements);
                CompInfo->MaxSize.Bits = (DataDict->SizeInfo.Bits / DataDict->NumSubElements);
                CompInfo->Offset.Bytes = CompInfo->MaxSize.Bytes * SubIndex;
                CompInfo->Offset.Bits = CompInfo->MaxSize.Bits * SubIndex;
            }
            break;
        }
        default:
            break;
        }

    }

    if (RefObj != NULL)
    {
        if (CompInfo != NULL)
        {
            CompInfo->EdsId = EdsLib_Encode_StructId(RefObj);
        }
        Result = EDSLIB_SUCCESS;
    }
    else
    {
        Result = EDSLIB_INVALID_INDEX;
    }

    return Result;
}

int32_t EdsLib_DataTypeDB_GetMemberByNativeOffset(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint32_t ByteOffset, EdsLib_DataTypeDB_EntityInfo_t *CompInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDict;
    EdsLib_DatabaseRef_t TempRef;
    int32_t Result;
    uint32_t SubIndex;

    memset(CompInfo, 0, sizeof(*CompInfo));
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDict = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (DataDict == NULL || ByteOffset >= DataDict->SizeInfo.Bytes)
    {
        Result = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else switch(DataDict->BasicType)
    {
    case EDSLIB_BASICTYPE_CONTAINER:
    {
        const EdsLib_FieldDetailEntry_t *ContEntry;

        /*
         * Locate the entry _prior_ to the one where the offset exceeds
         * the specified offset
         */
        ContEntry = DataDict->Detail.Container->EntryList;
        for (SubIndex=1; SubIndex < DataDict->NumSubElements; ++SubIndex)
        {
            if (ByteOffset < DataDict->Detail.Container->EntryList[SubIndex].Offset.Bytes)
            {
                break;
            }
            ++ContEntry;
        }

        CompInfo->EdsId = EdsLib_Encode_StructId(&ContEntry->RefObj);
        CompInfo->Offset = ContEntry->Offset;
        if (SubIndex < DataDict->NumSubElements)
        {
            /* Not last element, peek at offset of _next_ element to get max size */
            CompInfo->MaxSize = DataDict->Detail.Container->EntryList[SubIndex].Offset;
        }
        else
        {
            /* is last element - so use size of parent as max size */
            CompInfo->MaxSize = DataDict->SizeInfo;
        }
        CompInfo->MaxSize.Bytes -= CompInfo->Offset.Bytes;
        CompInfo->MaxSize.Bits -= CompInfo->Offset.Bits;
        Result = EDSLIB_SUCCESS;
        break;
    }
    case EDSLIB_BASICTYPE_ARRAY:
    {
        /*
         * Calculating the start offset this way, although it involves division,
         * does not require that we actually _have_ the definition of the array
         * element type loaded.  The total byte size of the entire array _must_
         * be an integer multiple of the array length, by definition.
         */
        CompInfo->EdsId = EdsLib_Encode_StructId(&DataDict->Detail.Array->ElementRefObj);
        CompInfo->MaxSize.Bytes = DataDict->SizeInfo.Bytes / DataDict->NumSubElements;
        CompInfo->MaxSize.Bits = DataDict->SizeInfo.Bits / DataDict->NumSubElements;
        SubIndex = ByteOffset / CompInfo->MaxSize.Bytes;
        CompInfo->Offset.Bytes = CompInfo->MaxSize.Bytes * SubIndex;
        CompInfo->Offset.Bits = CompInfo->MaxSize.Bits * SubIndex;
        Result = EDSLIB_SUCCESS;
        break;
    }
    default:
        Result = EDSLIB_INVALID_SIZE_OR_TYPE;
        break;
    }

    return Result;
}


int32_t EdsLib_DataTypeDB_GetDerivedTypeById(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t DerivId, EdsLib_Id_t *DerivedEdsId)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_DatabaseRef_t TempRef;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (DataDictPtr == NULL || DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER)
    {
        return EDSLIB_INVALID_SIZE_OR_TYPE;
    }

    if (DerivId >= DataDictPtr->Detail.Container->DerivativeListSize)
    {
        return EDSLIB_INVALID_INDEX;
    }

    *DerivedEdsId = EdsLib_Encode_StructId(&DataDictPtr->Detail.Container->DerivativeList[DerivId].RefObj);

    return EDSLIB_SUCCESS;
}


int32_t EdsLib_DataTypeDB_GetConstraintEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t ConstraintIdx, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
{
    EdsLib_DatabaseRef_t ParentObjRef;
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    const EdsLib_ConstraintEntity_t *ConstraintEntityPtr;

    memset(MemberInfo, 0, sizeof(*MemberInfo));
    EdsLib_Decode_StructId(&ParentObjRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &ParentObjRef);
    if (DataDictPtr == NULL || DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER)
    {
        return EDSLIB_FAILURE;
    }

    if (ConstraintIdx >= DataDictPtr->Detail.Container->ConstraintEntityListSize)
    {
        return EDSLIB_FAILURE;
    }

    ConstraintEntityPtr = &DataDictPtr->Detail.Container->ConstraintEntityList[ConstraintIdx];
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &ConstraintEntityPtr->RefObj);
    if (DataDictPtr == NULL)
    {
        return EDSLIB_FAILURE;
    }

    MemberInfo->EdsId = EdsLib_Encode_StructId(&ConstraintEntityPtr->RefObj);
    MemberInfo->Offset = ConstraintEntityPtr->Offset;
    MemberInfo->MaxSize = DataDictPtr->SizeInfo;

    return EDSLIB_SUCCESS;
}

int32_t EdsLib_DataTypeDB_GetDerivedInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_DataTypeDB_DerivedTypeInfo_t *DerivInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_DatabaseRef_t TempRef;

    memset(DerivInfo,0,sizeof(*DerivInfo));
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (DataDictPtr == NULL)
    {
        return EDSLIB_FAILURE;
    }

    if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_CONTAINER)
    {
        DerivInfo->NumConstraints = DataDictPtr->Detail.Container->ConstraintEntityListSize;
        DerivInfo->NumDerivatives = DataDictPtr->Detail.Container->DerivativeListSize;
        if (DataDictPtr->Detail.Container->DerivativeListSize > 0)
        {
            DerivInfo->MaxSize = DataDictPtr->Detail.Container->MaxSize;
        }
        else
        {
            DerivInfo->MaxSize = DataDictPtr->SizeInfo;
        }
    }
    else
    {
        DerivInfo->MaxSize = DataDictPtr->SizeInfo;
    }

    return EDSLIB_SUCCESS;
}

int32_t EdsLib_DataTypeDB_ConstraintIterator(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId,
        EdsLib_Id_t DerivedId, EdsLib_ConstraintCallback_t Callback, void *CbArg)
{
    EdsLib_DataTypeDB_EntityInfo_t BaseInfo;
    EdsLib_ConstraintIterator_ControlBlock_t CtlBlock;
    EdsLib_DatabaseRef_t BaseRef;
    uint32_t NumDerivs = 0;
    int32_t Status = EDSLIB_FAILURE;

    memset(&CtlBlock, 0, sizeof(CtlBlock));
    CtlBlock.UserCallback = Callback;
    CtlBlock.CbArg = CbArg;
    memset(&BaseInfo, 0, sizeof(BaseInfo));
    BaseInfo.EdsId = DerivedId;

    while(BaseInfo.EdsId != BaseId)
    {
        EdsLib_Decode_StructId(&CtlBlock.TargetRef, BaseInfo.EdsId);

        /*
         * Note that any base object that this derives from must, by definition, be the first
         * subcomponent in this object.  Therefore it shouldn't really be necessary to check anything
         * beyond index 0.
         */
        Status = EdsLib_DataTypeDB_GetMemberByIndex(GD, BaseInfo.EdsId, 0, &BaseInfo);
        if (Status != EDSLIB_SUCCESS)
        {
            break;
        }

        EdsLib_Decode_StructId(&BaseRef, BaseInfo.EdsId);

        Status = EdsLib_DataTypeDB_ConstraintIterator_Impl(GD, &CtlBlock, &BaseRef);
        if (Status != EDSLIB_SUCCESS)
        {
            /*
             * The ConstraintIterator will return an error if the BaseRef is not
             * actually a container or the TargetRef is not in its container derivative
             * list.  (this will be the case, for instance, if the member 0 was just
             * an ordinary container member, not a base type).
             *
             * If a specific BaseId was specified, then this is an error because it
             * means that the BaseId is _not_ a base type for the DerivedId given.
             *
             * If the BaseId was _not_ specified, then this is simply the
             * end of the loop, because we reached the lowest base type in EDS.
             * This is not an error so long as there was at least 1 successful
             * pass through this loop.
             *
             */
            if (!EdsLib_Is_Valid(BaseId) && NumDerivs > 0)
            {
                Status = EDSLIB_SUCCESS;
            }
            break;
        }

        ++NumDerivs;
    }

    return Status;
}

int32_t EdsLib_DataTypeDB_BaseCheck(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t DerivedId)
{
    /*
     * This needs a specific base type indicated.
     * (Makes no sense to call this otherwise)
     */
    if (!EdsLib_Is_Valid(BaseId))
    {
        return EDSLIB_FAILURE;
    }

    /*
     * Invoke the iterator with a NULL callback.
     *
     * This will go through the process of validating the basetype relationship
     * without actually going through the constraint tables or generating any callbacks.
     */
    return EdsLib_DataTypeDB_ConstraintIterator(GD, BaseId, DerivedId, NULL, NULL);
}


int32_t EdsLib_DataTypeDB_PackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize, uint32_t StartingBit)
{
    EdsLib_DataTypePackUnpack_ControlBlock_t PackState;

    memset(&PackState, 0, sizeof(PackState));
    PackState.DestBasePtr = DestBuffer;
    PackState.SourceBasePtr = SourceBuffer;
    PackState.OperMode = EDSLIB_BITPACK_OPERMODE_PACK;
    PackState.MaxSize.Bits = MaxPackedBitSize;
    PackState.MaxSize.Bytes = SourceByteSize;
    PackState.ProcessedSize.Bits = StartingBit;
    EdsLib_Decode_StructId(&PackState.RefObj, *EdsId);

    EdsLib_DataTypePackUnpack_Impl(GD, &PackState);

    if (PackState.Status == EDSLIB_SUCCESS)
    {
        *EdsId = EdsLib_Encode_StructId(&PackState.RefObj);
    }

    return PackState.Status;
}

int32_t EdsLib_DataTypeDB_FinalizePackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *PackedData)
{
    EdsLib_PackedPostProc_ControlBlock_t CtlBlock;
    int32_t Status;

    EDSLIB_DECLARE_ITERATOR_CB(IteratorState,
            EDSLIB_ITERATOR_MAX_DEEP_DEPTH,
            EdsLib_PackedObject_PostProc_Callback,
            &CtlBlock);

    memset(&CtlBlock, 0, sizeof(CtlBlock));
    CtlBlock.BasePtr = PackedData;

    EDSLIB_RESET_ITERATOR_FROM_EDSID(IteratorState, EdsId);

    Status = EdsLib_DataTypeIterator_Impl(GD, &IteratorState.Cb);

    if (Status == EDSLIB_SUCCESS && CtlBlock.ErrorCtlType != EdsLib_ErrorControlType_INVALID &&
            CtlBlock.BaseDictPtr != NULL && CtlBlock.ErrorCtlDictPtr != NULL)
    {
        EdsLib_UpdateErrorControlField(CtlBlock.ErrorCtlDictPtr, PackedData, CtlBlock.BaseDictPtr->SizeInfo.Bits,
                CtlBlock.ErrorCtlType, CtlBlock.ErrorCtlOffsetBits);
    }

    return Status;
}

int32_t EdsLib_DataTypeDB_PackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize)
{
    int32_t Status;

    Status = EdsLib_DataTypeDB_PackPartialObject(GD, EdsId, DestBuffer, SourceBuffer, MaxPackedBitSize, SourceByteSize, 0);
    if (Status == EDSLIB_SUCCESS)
    {
        Status = EdsLib_DataTypeDB_FinalizePackedObject(GD, *EdsId, DestBuffer);
    }

    return Status;
}

int32_t EdsLib_DataTypeDB_UnpackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize, uint32_t StartingByte)
{
    EdsLib_DataTypePackUnpack_ControlBlock_t PackState;

    memset(&PackState, 0, sizeof(PackState));
    PackState.DestBasePtr = DestBuffer;
    PackState.SourceBasePtr = SourceBuffer;
    PackState.OperMode = EDSLIB_BITPACK_OPERMODE_UNPACK;
    PackState.MaxSize.Bits = SourceBitSize;
    PackState.MaxSize.Bytes = MaxNativeByteSize;
    PackState.ProcessedSize.Bytes = StartingByte;
    EdsLib_Decode_StructId(&PackState.RefObj, *EdsId);

    EdsLib_DataTypePackUnpack_Impl(GD, &PackState);

    if (PackState.Status == EDSLIB_SUCCESS)
    {
        *EdsId = EdsLib_Encode_StructId(&PackState.RefObj);
    }

    return PackState.Status;
}

int32_t EdsLib_DataTypeDB_VerifyUnpackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
        void *UnpackedObj, const void *PackedObj, uint32_t RecomputeFields)
{
    EdsLib_NativePostProc_ControlBlock_t CtlBlock;
    int32_t Status;

    EDSLIB_DECLARE_ITERATOR_CB(IteratorState,
            EDSLIB_ITERATOR_MAX_DEEP_DEPTH,
            EdsLib_NativeObject_PostProc_Callback,
            &CtlBlock);

    memset(&CtlBlock, 0, sizeof(CtlBlock));
    CtlBlock.NativePtr = UnpackedObj;
    CtlBlock.PackedPtr = PackedObj;
    CtlBlock.RecomputeFields = RecomputeFields;

    EDSLIB_RESET_ITERATOR_FROM_EDSID(IteratorState, EdsId);

    Status = EdsLib_DataTypeIterator_Impl(GD, &IteratorState.Cb);
    if (Status == EDSLIB_SUCCESS)
    {
        Status = CtlBlock.Status;
    }

    return Status;
}

static void EdsLib_NativeObject_ConstraintInitCallback(const EdsLib_DatabaseObject_t *GD, const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo, EdsLib_GenericValueBuffer_t *ConstraintValue, void *Arg)
{
    uint8_t *DataBuf = Arg;

    EdsLib_DataTypeDB_StoreValue(GD, MemberInfo->EdsId,
            &DataBuf[MemberInfo->Offset.Bytes],
            ConstraintValue);
}

int32_t EdsLib_DataTypeDB_InitializeNativeObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *UnpackedObj)
{
    EdsLib_NativePostProc_ControlBlock_t CtlBlock;
    int32_t Status;

    EDSLIB_DECLARE_ITERATOR_CB(IteratorState,
            EDSLIB_ITERATOR_MAX_DEEP_DEPTH,
            EdsLib_NativeObject_PostProc_Callback,
            &CtlBlock);

    memset(&CtlBlock, 0, sizeof(CtlBlock));
    CtlBlock.NativePtr = UnpackedObj;

    EDSLIB_RESET_ITERATOR_FROM_EDSID(IteratorState, EdsId);
    Status = EdsLib_DataTypeIterator_Impl(GD, &IteratorState.Cb);
    if (Status == EDSLIB_SUCCESS)
    {
        Status = CtlBlock.Status;
    }

    if (Status != EDSLIB_SUCCESS)
    {
        return Status;
    }

    Status = EdsLib_DataTypeDB_ConstraintIterator(GD, EDSLIB_ID_INVALID, EdsId,
            EdsLib_NativeObject_ConstraintInitCallback, UnpackedObj);

    return Status;
}



int32_t EdsLib_DataTypeDB_UnpackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize)
{
    int32_t Status;

    Status = EdsLib_DataTypeDB_UnpackPartialObject(GD, EdsId, DestBuffer, SourceBuffer,
            MaxNativeByteSize, SourceBitSize, 0);
    if (Status == EDSLIB_SUCCESS)
    {
        Status = EdsLib_DataTypeDB_VerifyUnpackedObject(GD, *EdsId, DestBuffer,
                SourceBuffer, EDSLIB_DATATYPEDB_RECOMPUTE_NONE);
    }

    return Status;
}

int32_t EdsLib_DataTypeDB_LoadValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_GenericValueBuffer_t *DestBuffer, const void *SrcPtr)
{
    EdsLib_DatabaseRef_t TempRef;
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_ConstPtr_t Src = { .Ptr = SrcPtr };

    DestBuffer->ValueType = EDSLIB_BASICTYPE_NONE;
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);
    if (DataDictPtr != NULL)
    {
        EdsLib_DataTypeLoad_Impl(DestBuffer, Src, DataDictPtr);
    }

    if (DestBuffer->ValueType == EDSLIB_BASICTYPE_NONE)
    {
        return EDSLIB_FAILURE;
    }
    return EDSLIB_SUCCESS;
}

int32_t EdsLib_DataTypeDB_StoreValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr, EdsLib_GenericValueBuffer_t *SrcBuffer)
{
    EdsLib_DatabaseRef_t TempRef;
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_Ptr_t Dest = { .Ptr = DestPtr };

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);
    if (DataDictPtr != NULL)
    {
        EdsLib_DataTypeStore_Impl(Dest, SrcBuffer, DataDictPtr);
        if (SrcBuffer->ValueType == DataDictPtr->BasicType)
        {
            return EDSLIB_SUCCESS;
        }
    }

    return EDSLIB_FAILURE;
}


int32_t EdsLib_DataTypeDB_IdentifyBuffer(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const void *MessageBuffer, EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_DatabaseRef_t ActualRef;
    int32_t Status;

    EdsLib_Decode_StructId(&ActualRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &ActualRef);

    Status = EdsLib_DataTypeIdentifyBuffer_Impl(GD, DataDictPtr, MessageBuffer,
            &DerivObjInfo->DerivativeTableIndex, &ActualRef);

    if (Status == EDSLIB_SUCCESS)
    {
        DerivObjInfo->EdsId = EdsLib_Encode_StructId(&ActualRef);
    }

    return Status;
}


