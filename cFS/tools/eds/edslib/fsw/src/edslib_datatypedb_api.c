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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void EdsLib_DataTypeDB_Initialize(void)
{
    EdsLib_ErrorControl_Initialize();
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
uint16_t EdsLib_DataTypeDB_GetAppIdx(const EdsLib_DataTypeDB_t AppDict)
{
    return AppDict->MissionIdx;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_Register(EdsLib_DatabaseObject_t *GD, EdsLib_DataTypeDB_t AppDict)
{
    if (GD == NULL || GD->DataTypeDB_Table == NULL || AppDict->MissionIdx >= GD->AppTableSize
        || GD->DataTypeDB_Table[AppDict->MissionIdx] != NULL)
    {
        return EDSLIB_FAILURE;
    }

    GD->DataTypeDB_Table[AppDict->MissionIdx] = AppDict;
    return EDSLIB_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_Unregister(EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
{
    if (GD == NULL || GD->DataTypeDB_Table == NULL || AppIdx >= GD->AppTableSize
        || GD->DataTypeDB_Table[AppIdx] == NULL)
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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_GetTypeInfo(const EdsLib_DatabaseObject_t *GD,
                                      EdsLib_Id_t                    EdsId,
                                      EdsLib_DataTypeDB_TypeInfo_t  *TypeInfo)
{
    EdsLib_DatabaseRef_t             TempRef;
    const EdsLib_DataTypeDB_Entry_t *DataDictEntry;
    int32_t                          Status;

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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_GetMemberByIndex(const EdsLib_DatabaseObject_t  *GD,
                                           EdsLib_Id_t                     EdsId,
                                           uint16_t                        SubIndex,
                                           EdsLib_DataTypeDB_EntityInfo_t *CompInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDict;
    const EdsLib_FieldDetailEntry_t *MemberDetail;
    const EdsLib_DataTypeDB_Entry_t *MemberDict;
    const EdsLib_DatabaseRef_t      *RefObj;
    EdsLib_DatabaseRef_t             TempRef;
    uint16_t                         Idx;
    int32_t                          Result;

    RefObj = NULL;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDict = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (CompInfo != NULL)
    {
        memset(CompInfo, 0, sizeof(*CompInfo));
    }

    if (DataDict != NULL && SubIndex < DataDict->NumSubElements)
    {
        switch (DataDict->BasicType)
        {
            case EDSLIB_BASICTYPE_CONTAINER:
            case EDSLIB_BASICTYPE_COMPONENT:
            {
                RefObj = &DataDict->Detail.Container->EntryList[SubIndex].RefObj;

                if (CompInfo != NULL)
                {
                    const EdsLib_SizeInfo_t *EndOffset;

                    /* check if the item could be at a variable location */
                    if ((DataDict->Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
                    {
                        for (Idx = 0; Idx < SubIndex; ++Idx)
                        {
                            MemberDetail = &DataDict->Detail.Container->EntryList[Idx];
                            MemberDict   = EdsLib_DataTypeDB_GetEntry(GD, &MemberDetail->RefObj);
                            if (MemberDict != NULL && (MemberDict->Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
                            {
                                CompInfo->Flags |= EDSLIB_DATATYPE_FLAG_VARIABLE_LOCATION;
                            }
                        }
                    }

                    MemberDetail = &DataDict->Detail.Container->EntryList[SubIndex];

                    if (SubIndex < (DataDict->NumSubElements - 1))
                    {
                        /* not the last entry in the container - peek to the next */
                        EndOffset = &MemberDetail[1].Offset;
                    }
                    else
                    {
                        /* last element -- use the parent size */
                        EndOffset = &DataDict->SizeInfo;
                    }

                    CompInfo->Offset        = MemberDetail->Offset;
                    CompInfo->MaxSize.Bytes = (EndOffset->Bytes - CompInfo->Offset.Bytes);
                    CompInfo->MaxSize.Bits  = (EndOffset->Bits - CompInfo->Offset.Bits);
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
                    CompInfo->MaxSize.Bits  = (DataDict->SizeInfo.Bits / DataDict->NumSubElements);
                    CompInfo->Offset.Bytes  = CompInfo->MaxSize.Bytes * SubIndex;
                    CompInfo->Offset.Bits   = CompInfo->MaxSize.Bits * SubIndex;
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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_GetMemberByNativeOffset(const EdsLib_DatabaseObject_t  *GD,
                                                  EdsLib_Id_t                     EdsId,
                                                  uint32_t                        ByteOffset,
                                                  EdsLib_DataTypeDB_EntityInfo_t *CompInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDict;
    EdsLib_DatabaseRef_t             TempRef;
    int32_t                          Result;

    memset(CompInfo, 0, sizeof(*CompInfo));
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDict = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (DataDict == NULL || ByteOffset > DataDict->SizeInfo.Bytes)
    {
        Result = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else if (ByteOffset == DataDict->SizeInfo.Bytes)
    {
        /* special case when it is the exact tail end of the container,
         * return successfully but indicate there is no real member here. */
        CompInfo->Offset = DataDict->SizeInfo;
        if ((DataDict->Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
        {
            CompInfo->Flags |= EDSLIB_DATATYPE_FLAG_VARIABLE_LOCATION;
        }
        Result = EDSLIB_SUCCESS;
    }
    else
        switch (DataDict->BasicType)
        {
            case EDSLIB_BASICTYPE_CONTAINER:
            case EDSLIB_BASICTYPE_COMPONENT:
            {
                Result = EdsLib_DataTypeDB_FindContainerMember_Impl(GD,
                                                                    DataDict,
                                                                    &ByteOffset,
                                                                    EdsLib_OffsetCompareBytes,
                                                                    CompInfo);
                break;
            }
            case EDSLIB_BASICTYPE_ARRAY:
            {
                Result = EdsLib_DataTypeDB_FindArrayMember_Impl(GD,
                                                                DataDict,
                                                                &ByteOffset,
                                                                EdsLib_GetArrayIdxFromBytes,
                                                                CompInfo);
                break;
            }
            default:
                Result = EDSLIB_INVALID_SIZE_OR_TYPE;
                break;
        }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_GetMemberByPackedOffset(const EdsLib_DatabaseObject_t  *GD,
                                                  EdsLib_Id_t                     EdsId,
                                                  uint32_t                        BitOffset,
                                                  EdsLib_DataTypeDB_EntityInfo_t *CompInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDict;
    EdsLib_DatabaseRef_t             TempRef;
    int32_t                          Result;

    memset(CompInfo, 0, sizeof(*CompInfo));
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDict = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (DataDict == NULL || BitOffset > DataDict->SizeInfo.Bits)
    {
        Result = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else if (BitOffset == DataDict->SizeInfo.Bits)
    {
        /* special case when it is the exact tail end of the container,
         * return successfully but indicate there is no real member here. */
        CompInfo->Offset = DataDict->SizeInfo;
        if ((DataDict->Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
        {
            CompInfo->Flags |= EDSLIB_DATATYPE_FLAG_VARIABLE_LOCATION;
        }
        Result = EDSLIB_SUCCESS;
    }
    else
        switch (DataDict->BasicType)
        {
            case EDSLIB_BASICTYPE_CONTAINER:
            case EDSLIB_BASICTYPE_COMPONENT:
            {
                Result = EdsLib_DataTypeDB_FindContainerMember_Impl(GD,
                                                                    DataDict,
                                                                    &BitOffset,
                                                                    EdsLib_OffsetCompareBits,
                                                                    CompInfo);
                break;
            }
            case EDSLIB_BASICTYPE_ARRAY:
            {
                Result = EdsLib_DataTypeDB_FindArrayMember_Impl(GD,
                                                                DataDict,
                                                                &BitOffset,
                                                                EdsLib_GetArrayIdxFromBits,
                                                                CompInfo);
                break;
            }
            default:
                Result = EDSLIB_INVALID_SIZE_OR_TYPE;
                break;
        }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_GetDerivedTypeById(const EdsLib_DatabaseObject_t *GD,
                                             EdsLib_Id_t                    EdsId,
                                             uint16_t                       DerivId,
                                             EdsLib_Id_t                   *DerivedEdsId)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_DatabaseRef_t             TempRef;

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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_GetConstraintEntity(const EdsLib_DatabaseObject_t  *GD,
                                              EdsLib_Id_t                     EdsId,
                                              uint16_t                        ConstraintIdx,
                                              EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
{
    EdsLib_DatabaseRef_t                  ParentObjRef;
    const EdsLib_DataTypeDB_Entry_t      *DataDictPtr;
    const EdsLib_ConstraintEntity_t      *ConstraintEntityPtr;
    const EdsLib_IdentityCheckSequence_t *DerivativeCheck;

    memset(MemberInfo, 0, sizeof(*MemberInfo));
    EdsLib_Decode_StructId(&ParentObjRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &ParentObjRef);
    if (DataDictPtr == NULL || DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER
        || DataDictPtr->Detail.Container == NULL)
    {
        return EDSLIB_FAILURE;
    }

    DerivativeCheck = DataDictPtr->Detail.Container->CheckSequence;

    if (ConstraintIdx >= DerivativeCheck->ConstraintEntityListSize)
    {
        return EDSLIB_FAILURE;
    }

    ConstraintEntityPtr = &DerivativeCheck->ConstraintEntityList[ConstraintIdx];
    DataDictPtr         = EdsLib_DataTypeDB_GetEntry(GD, &ConstraintEntityPtr->RefObj);
    if (DataDictPtr == NULL)
    {
        return EDSLIB_FAILURE;
    }

    MemberInfo->EdsId   = EdsLib_Encode_StructId(&ConstraintEntityPtr->RefObj);
    MemberInfo->Offset  = ConstraintEntityPtr->Offset;
    MemberInfo->MaxSize = DataDictPtr->SizeInfo;

    return EDSLIB_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_GetDerivedInfo(const EdsLib_DatabaseObject_t       *GD,
                                         EdsLib_Id_t                          EdsId,
                                         EdsLib_DataTypeDB_DerivedTypeInfo_t *DerivInfo)
{
    const EdsLib_DataTypeDB_Entry_t      *DataDictPtr;
    const EdsLib_IdentityCheckSequence_t *DerivativeCheck;
    EdsLib_DatabaseRef_t                  TempRef;

    memset(DerivInfo, 0, sizeof(*DerivInfo));
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (DataDictPtr == NULL)
    {
        return EDSLIB_FAILURE;
    }

    if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_CONTAINER && DataDictPtr->Detail.Container != NULL)
    {
        DerivInfo->NumDerivatives = DataDictPtr->Detail.Container->DerivativeListSize;
        if (DataDictPtr->Detail.Container->DerivativeListSize > 0)
        {
            DerivInfo->MaxSize = DataDictPtr->Detail.Container->MaxSize;
        }
        else
        {
            DerivInfo->MaxSize = DataDictPtr->SizeInfo;
        }

        DerivativeCheck = DataDictPtr->Detail.Container->CheckSequence;
        if (DerivativeCheck != NULL)
        {
            DerivInfo->NumConstraints = DerivativeCheck->ConstraintEntityListSize;
        }
    }
    else
    {
        DerivInfo->MaxSize = DataDictPtr->SizeInfo;
    }

    return EDSLIB_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_ConstraintIterator(const EdsLib_DatabaseObject_t *GD,
                                             EdsLib_Id_t                    BaseId,
                                             EdsLib_Id_t                    DerivedId,
                                             EdsLib_ConstraintCallback_t    Callback,
                                             void                          *CbArg)
{
    EdsLib_DataTypeDB_EntityInfo_t           BaseInfo;
    EdsLib_ConstraintIterator_ControlBlock_t CtlBlock;
    EdsLib_DatabaseRef_t                     BaseRef;
    uint32_t                                 NumDerivs = 0;
    int32_t                                  Status    = EDSLIB_FAILURE;

    memset(&CtlBlock, 0, sizeof(CtlBlock));
    CtlBlock.UserCallback = Callback;
    CtlBlock.CbArg        = CbArg;
    memset(&BaseInfo, 0, sizeof(BaseInfo));
    BaseInfo.EdsId = DerivedId;

    while (BaseInfo.EdsId != BaseId)
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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_PackPartialObjectVarSize(const EdsLib_DatabaseObject_t *GD,
                                                   EdsLib_Id_t                   *EdsId,
                                                   void                          *DestBuffer,
                                                   const void                    *SourceBuffer,
                                                   const EdsLib_SizeInfo_t       *MaxSize,
                                                   EdsLib_SizeInfo_t             *ProcessedSize)
{
    EdsLib_DataTypePack_State_t PackState;
    int32_t                     Status;

    memset(&PackState, 0, sizeof(PackState));

    PackState.Common.HandleMember    = EdsLib_DataTypePack_HandleMember;
    PackState.Common.NativeBufferPtr = SourceBuffer;
    PackState.Common.MaxSize         = *MaxSize;
    EdsLib_Decode_StructId(&PackState.Common.RefObj, *EdsId);
    PackState.PackedDstPtr = DestBuffer;
    PackState.NativeSrcPtr = SourceBuffer;

    Status = EdsLib_DataTypeDB_PackUnpackFindTail(GD, *EdsId, ProcessedSize, &PackState.Common);

    if (Status == EDSLIB_SUCCESS)
    {
        EdsLib_DataTypePackUnpack_Impl(GD, &PackState.Common);
        Status = PackState.Common.Status;
    }

    if (Status == EDSLIB_SUCCESS)
    {
        *EdsId = EdsLib_Encode_StructId(&PackState.Common.RefObj);
    }

    if (ProcessedSize != NULL)
    {
        ProcessedSize->Bits  = PackState.Common.LastActualTailBitPos;
        ProcessedSize->Bytes = PackState.Common.LastNominalTail.Bytes;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_PackPartialObject(const EdsLib_DatabaseObject_t *GD,
                                            EdsLib_Id_t                   *EdsId,
                                            void                          *DestBuffer,
                                            const void                    *SourceBuffer,
                                            uint32_t                       MaxPackedBitSize,
                                            uint32_t                       SourceByteSize,
                                            uint32_t                       StartingBit)
{
    EdsLib_SizeInfo_t              MaxSize;
    EdsLib_SizeInfo_t              ProcessedSize;
    EdsLib_DataTypeDB_EntityInfo_t EntityInfo;
    int32_t                        Status;

    MaxSize.Bits  = MaxPackedBitSize;
    MaxSize.Bytes = SourceByteSize;

    memset(&ProcessedSize, 0, sizeof(ProcessedSize));

    if (StartingBit == 0)
    {
        Status = EDSLIB_SUCCESS;
    }
    else
    {
        ProcessedSize.Bits = StartingBit;

        Status = EdsLib_DataTypeDB_PackUnpackFindTailEntity(GD, *EdsId, &ProcessedSize, &EntityInfo);
        if (Status == EDSLIB_SUCCESS)
        {
            if ((EntityInfo.Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_LOCATION) != 0)
            {
                /* this only works if the item is at a fixed location */
                Status = EDSLIB_NOT_FIXED_SIZE;
            }
            else
            {
                ProcessedSize = EntityInfo.Offset;
            }
        }
    }

    if (Status == EDSLIB_SUCCESS)
    {
        Status =
            EdsLib_DataTypeDB_PackPartialObjectVarSize(GD, EdsId, DestBuffer, SourceBuffer, &MaxSize, &ProcessedSize);
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_FinalizePackedObjectVarSize(const EdsLib_DatabaseObject_t *GD,
                                                      EdsLib_Id_t                    EdsId,
                                                      const void                    *NativeBuffer,
                                                      void                          *PackedBuffer,
                                                      const EdsLib_SizeInfo_t       *ProcessedSize)
{
    EdsLib_PackedPostProc_ControlBlock_t CtlBlock;

    memset(&CtlBlock, 0, sizeof(CtlBlock));

    CtlBlock.Pack.Common.HandleMember    = EdsLib_PackedObject_PostProc_Callback;
    CtlBlock.Pack.Common.NativeBufferPtr = NativeBuffer;
    CtlBlock.Pack.Common.MaxSize         = *ProcessedSize;
    CtlBlock.Pack.Common.MaxPasses       = 1;
    EdsLib_Decode_StructId(&CtlBlock.Pack.Common.RefObj, EdsId);

    CtlBlock.Pack.NativeSrcPtr = NativeBuffer;
    CtlBlock.Pack.PackedDstPtr = PackedBuffer;

    EdsLib_DataTypePackUnpack_Impl(GD, &CtlBlock.Pack.Common);
    if (CtlBlock.Pack.Common.Status == EDSLIB_SUCCESS)
    {
        EdsLib_PackedObject_PostProc_Deferred(&CtlBlock);
    }

    return CtlBlock.Pack.Common.Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_FinalizePackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *PackedData)
{
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    int32_t                      Status;

    /*
     * This API gets the size from the DB, it is not passed via arguments.  Thus
     * it cannot work with variably-sized objects.
     */
    Status = EdsLib_DataTypeDB_GetTypeInfo(GD, EdsId, &TypeInfo);
    if (Status == EDSLIB_SUCCESS && (TypeInfo.Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
    {
        /* Cannot use this API, must use the VarSize variant */
        Status = EDSLIB_NOT_FIXED_SIZE;
    }
    else
    {
        Status = EdsLib_DataTypeDB_FinalizePackedObjectVarSize(GD, EdsId, NULL, PackedData, &TypeInfo.Size);
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_PackCompleteObjectVarSize(const EdsLib_DatabaseObject_t *GD,
                                                    EdsLib_Id_t                   *EdsId,
                                                    void                          *DestBuffer,
                                                    const void                    *SourceBuffer,
                                                    EdsLib_SizeInfo_t             *PackedSize)
{
    int32_t           Status;
    EdsLib_SizeInfo_t ProcessedSize;

    memset(&ProcessedSize, 0, sizeof(ProcessedSize));

    Status =
        EdsLib_DataTypeDB_PackPartialObjectVarSize(GD, EdsId, DestBuffer, SourceBuffer, PackedSize, &ProcessedSize);
    if (Status == EDSLIB_SUCCESS)
    {
        Status = EdsLib_DataTypeDB_FinalizePackedObjectVarSize(GD, *EdsId, SourceBuffer, DestBuffer, &ProcessedSize);
    }
    if (Status == EDSLIB_SUCCESS)
    {
        *PackedSize = ProcessedSize;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_PackCompleteObject(const EdsLib_DatabaseObject_t *GD,
                                             EdsLib_Id_t                   *EdsId,
                                             void                          *DestBuffer,
                                             const void                    *SourceBuffer,
                                             uint32_t                       MaxPackedBitSize,
                                             uint32_t                       SourceByteSize)
{
    int32_t                      Status;
    EdsLib_SizeInfo_t            TotalPackedSize;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;

    TotalPackedSize.Bits  = MaxPackedBitSize;
    TotalPackedSize.Bytes = SourceByteSize;

    Status = EdsLib_DataTypeDB_PackCompleteObjectVarSize(GD, EdsId, DestBuffer, SourceBuffer, &TotalPackedSize);

    /* Check for variable size output.  This has to be done after the encode, because
     * the encode also includes identification and may update the EdsId.  This API has
     * no way to return the packed size to the caller, so if the output was variably-sized
     * it means needed information - the size of the result - is irrecoverably lost. */
    if (Status == EDSLIB_SUCCESS)
    {
        Status = EdsLib_DataTypeDB_GetTypeInfo(GD, *EdsId, &TypeInfo);
        if (Status == EDSLIB_SUCCESS && (TypeInfo.Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
        {
            /* This means the caller should not be using this API - need to use the VarSize variant! */
            Status = EDSLIB_NOT_FIXED_SIZE;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_UnpackPartialObjectVarSize(const EdsLib_DatabaseObject_t *GD,
                                                     EdsLib_Id_t                   *EdsId,
                                                     void                          *DestBuffer,
                                                     const void                    *SourceBuffer,
                                                     const EdsLib_SizeInfo_t       *MaxSize,
                                                     EdsLib_SizeInfo_t             *ProcessedSize)
{
    EdsLib_DataTypeUnpack_State_t UnpackState;
    int32_t                       Status;

    memset(&UnpackState, 0, sizeof(UnpackState));

    UnpackState.Common.HandleMember    = EdsLib_DataTypeUnpack_HandleMember;
    UnpackState.Common.NativeBufferPtr = DestBuffer;
    UnpackState.Common.MaxSize         = *MaxSize;
    EdsLib_Decode_StructId(&UnpackState.Common.RefObj, *EdsId);

    UnpackState.NativeDstPtr = DestBuffer;
    UnpackState.PackedSrcPtr = SourceBuffer;

    Status = EdsLib_DataTypeDB_PackUnpackFindTail(GD, *EdsId, ProcessedSize, &UnpackState.Common);

    if (Status == EDSLIB_SUCCESS)
    {
        EdsLib_DataTypePackUnpack_Impl(GD, &UnpackState.Common);
        Status = UnpackState.Common.Status;
    }

    if (Status == EDSLIB_SUCCESS)
    {
        *EdsId = EdsLib_Encode_StructId(&UnpackState.Common.RefObj);
    }

    if (ProcessedSize != NULL)
    {
        ProcessedSize->Bits  = UnpackState.Common.LastActualTailBitPos;
        ProcessedSize->Bytes = UnpackState.Common.LastNominalTail.Bytes;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_UnpackPartialObject(const EdsLib_DatabaseObject_t *GD,
                                              EdsLib_Id_t                   *EdsId,
                                              void                          *DestBuffer,
                                              const void                    *SourceBuffer,
                                              uint32_t                       MaxNativeByteSize,
                                              uint32_t                       SourceBitSize,
                                              uint32_t                       StartingByte)
{
    EdsLib_SizeInfo_t              MaxSize;
    EdsLib_SizeInfo_t              ProcessedSize;
    EdsLib_DataTypeDB_EntityInfo_t EntityInfo;
    int32_t                        Status;

    MaxSize.Bits  = SourceBitSize;
    MaxSize.Bytes = MaxNativeByteSize;

    memset(&ProcessedSize, 0, sizeof(ProcessedSize));

    if (StartingByte == 0)
    {
        Status = EDSLIB_SUCCESS;
    }
    else
    {
        ProcessedSize.Bytes = StartingByte;

        Status = EdsLib_DataTypeDB_PackUnpackFindTailEntity(GD, *EdsId, &ProcessedSize, &EntityInfo);
        if (Status == EDSLIB_SUCCESS)
        {
            if ((EntityInfo.Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_LOCATION) != 0)
            {
                /* this only works if the item is at a fixed location */
                Status = EDSLIB_NOT_FIXED_SIZE;
            }
            else
            {
                ProcessedSize = EntityInfo.Offset;
            }
        }
    }

    if (Status == EDSLIB_SUCCESS)
    {
        Status =
            EdsLib_DataTypeDB_UnpackPartialObjectVarSize(GD, EdsId, DestBuffer, SourceBuffer, &MaxSize, &ProcessedSize);
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(const EdsLib_DatabaseObject_t *GD,
                                                      EdsLib_Id_t                    EdsId,
                                                      void                          *NativeBuffer,
                                                      const void                    *PackedBuffer,
                                                      uint32_t                       RecomputeFields,
                                                      const EdsLib_SizeInfo_t       *ProcessedSize)
{
    EdsLib_NativePostProc_ControlBlock_t CtlBlock;

    memset(&CtlBlock, 0, sizeof(CtlBlock));

    CtlBlock.Unpack.Common.HandleMember    = EdsLib_NativeObject_PostProc_Callback;
    CtlBlock.Unpack.Common.NativeBufferPtr = NativeBuffer;
    CtlBlock.Unpack.Common.MaxSize         = *ProcessedSize;
    CtlBlock.Unpack.Common.MaxPasses       = 1;
    EdsLib_Decode_StructId(&CtlBlock.Unpack.Common.RefObj, EdsId);

    CtlBlock.Unpack.NativeDstPtr = NativeBuffer;
    CtlBlock.Unpack.PackedSrcPtr = PackedBuffer;
    CtlBlock.RecomputeFields     = RecomputeFields;
    CtlBlock.CheckFields         = EDSLIB_DATATYPEDB_RECOMPUTE_ALL;

    EdsLib_DataTypePackUnpack_Impl(GD, &CtlBlock.Unpack.Common);

    return CtlBlock.Unpack.Common.Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_VerifyUnpackedObject(const EdsLib_DatabaseObject_t *GD,
                                               EdsLib_Id_t                    EdsId,
                                               void                          *UnpackedObj,
                                               const void                    *PackedObj,
                                               uint32_t                       RecomputeFields)
{
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    int32_t                      Status;

    /*
     * This API gets the size from the DB, it is not passed via arguments.  Thus
     * it cannot work with variably-sized objects.
     */
    Status = EdsLib_DataTypeDB_GetTypeInfo(GD, EdsId, &TypeInfo);
    if (Status == EDSLIB_SUCCESS && (TypeInfo.Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
    {
        /* Cannot use this API, must use the VarSize variant */
        Status = EDSLIB_NOT_FIXED_SIZE;
    }
    else
    {
        Status = EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(GD,
                                                               EdsId,
                                                               UnpackedObj,
                                                               PackedObj,
                                                               RecomputeFields,
                                                               &TypeInfo.Size);
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
static void EdsLib_NativeObject_ConstraintInitCallback(const EdsLib_DatabaseObject_t        *GD,
                                                       const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
                                                       EdsLib_GenericValueBuffer_t          *ConstraintValue,
                                                       void                                 *Arg)
{
    uint8_t *DataBuf = Arg;

    EdsLib_DataTypeDB_StoreValue(GD, MemberInfo->EdsId, &DataBuf[MemberInfo->Offset.Bytes], ConstraintValue);
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t
EdsLib_DataTypeDB_InitializeNativeObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *UnpackedObj)
{
    EdsLib_NativePostProc_ControlBlock_t CtlBlock;
    EdsLib_DataTypeDB_TypeInfo_t         TypeInfo;
    int32_t                              Status;

    memset(&CtlBlock, 0, sizeof(CtlBlock));
    EdsLib_DataTypeDB_GetTypeInfo(GD, EdsId, &TypeInfo);

    CtlBlock.Unpack.Common.HandleMember    = EdsLib_NativeObject_PostProc_Callback;
    CtlBlock.Unpack.Common.NativeBufferPtr = UnpackedObj;
    CtlBlock.Unpack.Common.MaxSize         = TypeInfo.Size;
    CtlBlock.Unpack.Common.MaxPasses       = 1;
    EdsLib_Decode_StructId(&CtlBlock.Unpack.Common.RefObj, EdsId);

    CtlBlock.Unpack.NativeDstPtr = UnpackedObj;
    CtlBlock.RecomputeFields     = EDSLIB_DATATYPEDB_RECOMPUTE_FIXEDVALUE | EDSLIB_DATATYPEDB_RECOMPUTE_LENGTH;

    EdsLib_DataTypePackUnpack_Impl(GD, &CtlBlock.Unpack.Common);

    Status = CtlBlock.Unpack.Common.Status;

    if (Status == EDSLIB_SUCCESS)
    {
        Status = EdsLib_DataTypeDB_ConstraintIterator(GD,
                                                      EDSLIB_ID_INVALID,
                                                      EdsId,
                                                      EdsLib_NativeObject_ConstraintInitCallback,
                                                      UnpackedObj);
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_UnpackCompleteObject(const EdsLib_DatabaseObject_t *GD,
                                               EdsLib_Id_t                   *EdsId,
                                               void                          *DestBuffer,
                                               const void                    *SourceBuffer,
                                               uint32_t                       MaxNativeByteSize,
                                               uint32_t                       SourceBitSize)
{
    int32_t           Status;
    EdsLib_SizeInfo_t MaxSize;
    EdsLib_SizeInfo_t ProcessedSize;

    MaxSize.Bits  = SourceBitSize;
    MaxSize.Bytes = MaxNativeByteSize;
    memset(&ProcessedSize, 0, sizeof(ProcessedSize));

    Status =
        EdsLib_DataTypeDB_UnpackPartialObjectVarSize(GD, EdsId, DestBuffer, SourceBuffer, &MaxSize, &ProcessedSize);
    if (Status == EDSLIB_SUCCESS)
    {
        Status = EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(GD,
                                                               *EdsId,
                                                               DestBuffer,
                                                               SourceBuffer,
                                                               EDSLIB_DATATYPEDB_RECOMPUTE_NONE,
                                                               &ProcessedSize);
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_LoadValue(const EdsLib_DatabaseObject_t *GD,
                                    EdsLib_Id_t                    EdsId,
                                    EdsLib_GenericValueBuffer_t   *DestBuffer,
                                    const void                    *SrcPtr)
{
    EdsLib_DatabaseRef_t             TempRef;
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_ConstPtr_t                Src = { .Ptr = SrcPtr };

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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_StoreValue(const EdsLib_DatabaseObject_t *GD,
                                     EdsLib_Id_t                    EdsId,
                                     void                          *DestPtr,
                                     EdsLib_GenericValueBuffer_t   *SrcBuffer)
{
    EdsLib_DatabaseRef_t             TempRef;
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_Ptr_t                     Dest = { .Ptr = DestPtr };

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

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_IdentifyBufferWithSize(const EdsLib_DatabaseObject_t            *GD,
                                                 EdsLib_Id_t                               EdsId,
                                                 const void                               *BufferPtr,
                                                 size_t                                    BufferSize,
                                                 EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_DatabaseRef_t             ActualRef;
    int32_t                          Status;

    EdsLib_Decode_StructId(&ActualRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &ActualRef);

    if (DataDictPtr == NULL)
    {
        Status = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else
    {
        Status = EdsLib_DataTypeIdentifyBuffer_Impl(GD,
                                                    DataDictPtr,
                                                    BufferPtr,
                                                    BufferSize,
                                                    &DerivObjInfo->DerivativeTableIndex,
                                                    &ActualRef);
    }

    if (Status == EDSLIB_SUCCESS)
    {
        DerivObjInfo->EdsId = EdsLib_Encode_StructId(&ActualRef);
    }
    else
    {
        /* do not return an undefined value */
        memset(DerivObjInfo, 0, sizeof(*DerivObjInfo));
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_IdentifyBuffer(const EdsLib_DatabaseObject_t            *GD,
                                         EdsLib_Id_t                               EdsId,
                                         const void                               *BufferPtr,
                                         EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    EdsLib_DatabaseRef_t             ActualRef;
    int32_t                          Status;

    EdsLib_Decode_StructId(&ActualRef, EdsId);
    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &ActualRef);

    if (DataDictPtr == NULL)
    {
        Status = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else
    {
        Status = EdsLib_DataTypeIdentifyBuffer_Impl(GD,
                                                    DataDictPtr,
                                                    BufferPtr,
                                                    DataDictPtr->SizeInfo.Bytes,
                                                    &DerivObjInfo->DerivativeTableIndex,
                                                    &ActualRef);
    }

    if (Status == EDSLIB_SUCCESS)
    {
        DerivObjInfo->EdsId = EdsLib_Encode_StructId(&ActualRef);
    }
    else
    {
        /* do not return an undefined value */
        memset(DerivObjInfo, 0, sizeof(*DerivObjInfo));
    }

    return Status;
}
