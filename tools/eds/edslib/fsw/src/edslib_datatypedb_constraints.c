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
 * \file     edslib_datatypedb_constraints.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Internal API functions for dealing with the EDS-defined "Mission-Unique"
 * message identifiers.  This includes:
 *  - looking up message size and signature information
 *  - looking up a basic EDS from a message id
 *  - looking up a CCSDS APID from a message id
 *  - helpers for dispatching messages at runtime
 *
 * This is linked as part of the "basic" EDS library
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "edslib_internal.h"

int32_t EdsLib_DataTypeDB_ConstraintIterator_Impl(const EdsLib_DatabaseObject_t *GD,
        EdsLib_ConstraintIterator_ControlBlock_t *CtlBlock, const EdsLib_DatabaseRef_t *BaseRef)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    const EdsLib_ContainerDescriptor_t *DerivDescPtr;
    const EdsLib_IdentSequenceEntry_t *IdentSequencePtr;
    const EdsLib_DerivativeEntry_t *DerivEntryPtr;
    const EdsLib_ValueEntry_t *SelectedEntry;
    const EdsLib_ConstraintEntity_t *SelectedLocation;
    uint16_t DerivIdx;
    int32_t Status;

    SelectedEntry = NULL;
    SelectedLocation = NULL;

    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, BaseRef);
    if (DataDictPtr == NULL || DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER)
    {
        return EDSLIB_INVALID_SIZE_OR_TYPE;
    }

    DerivDescPtr = DataDictPtr->Detail.Container;
    if (DerivDescPtr == NULL ||
            DerivDescPtr->IdentSequenceList == NULL ||
            DerivDescPtr->ValueList == NULL ||
            DerivDescPtr->ConstraintEntityList == NULL)
    {
        return EDSLIB_NO_MATCHING_VALUE;
    }

    Status = EDSLIB_NO_MATCHING_VALUE;
    DerivEntryPtr = DerivDescPtr->DerivativeList;
    for (DerivIdx=0; DerivIdx < DerivDescPtr->DerivativeListSize; ++DerivIdx)
    {
        if ((DerivEntryPtr->RefObj.AppIndex == CtlBlock->TargetRef.AppIndex &&
                DerivEntryPtr->RefObj.TypeIndex == CtlBlock->TargetRef.TypeIndex))
        {
            Status = EDSLIB_SUCCESS;
        }
        else if (CtlBlock->Recursive)
        {
            Status = EdsLib_DataTypeDB_ConstraintIterator_Impl(GD, CtlBlock, &DerivEntryPtr->RefObj);
        }
        if (Status == EDSLIB_SUCCESS)
        {
            break;
        }
        ++DerivEntryPtr;
    }

    if (Status != EDSLIB_SUCCESS || CtlBlock->UserCallback == NULL)
    {
        return Status;
    }

    /*
     * Walk through the identification sequence tree in reverse --
     * start at the result (the derivative) and follow the ParentOperation
     * links, which reveal the conditions required on the base type.
     */
    IdentSequencePtr = &DerivDescPtr->IdentSequenceList[DerivEntryPtr->IdentSeqIdx];
    CtlBlock->TempConstraintValue.ValueType = EDSLIB_BASICTYPE_NONE;
    while (IdentSequencePtr->EntryType != EDSLIB_IDENT_SEQUENCE_INVALID)
    {
        if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_VALUE_CONDITION)
        {
            SelectedEntry = &DerivDescPtr->ValueList[IdentSequencePtr->RefIdx];
        }
        else if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_ENTITY_LOCATION)
        {
            SelectedLocation = &DerivDescPtr->ConstraintEntityList[IdentSequencePtr->RefIdx];
            if (SelectedEntry != NULL)
            {
                DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &SelectedLocation->RefObj);
                if (DataDictPtr != NULL)
                {
                    if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_SIGNED_INT)
                    {
                        CtlBlock->TempConstraintValue.Value.SignedInteger = SelectedEntry->RefValue.Integer;
                        CtlBlock->TempConstraintValue.ValueType = DataDictPtr->BasicType;
                    }
                    else if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_UNSIGNED_INT)
                    {
                        CtlBlock->TempConstraintValue.Value.UnsignedInteger = SelectedEntry->RefValue.Unsigned;
                        CtlBlock->TempConstraintValue.ValueType = DataDictPtr->BasicType;
                    }
                    else if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_BINARY)
                    {
                        strncpy(CtlBlock->TempConstraintValue.Value.StringData, SelectedEntry->RefValue.String,
                                sizeof(CtlBlock->TempConstraintValue.Value.StringData));
                        CtlBlock->TempConstraintValue.ValueType = DataDictPtr->BasicType;
                    }
                }
                if (CtlBlock->TempConstraintValue.ValueType != EDSLIB_BASICTYPE_NONE)
                {
                    CtlBlock->TempMemberInfo.EdsId = EdsLib_Encode_StructId(&SelectedLocation->RefObj);
                    CtlBlock->TempMemberInfo.Offset = SelectedLocation->Offset;
                    CtlBlock->TempMemberInfo.MaxSize = DataDictPtr->SizeInfo;
                    CtlBlock->UserCallback(GD, &CtlBlock->TempMemberInfo, &CtlBlock->TempConstraintValue, CtlBlock->CbArg);
                }
                CtlBlock->TempConstraintValue.ValueType = EDSLIB_BASICTYPE_NONE;
                SelectedEntry = NULL;
            }
        }
        IdentSequencePtr = &DerivDescPtr->IdentSequenceList[IdentSequencePtr->ParentOperation];
    }

    return EDSLIB_SUCCESS;
}

int32_t EdsLib_DataTypeIdentifyBuffer_Impl(const EdsLib_DatabaseObject_t *GD, const EdsLib_DataTypeDB_Entry_t *DataDictPtr, const void *Buffer, uint16_t *DerivTableIndex, EdsLib_DatabaseRef_t *ActualObj)
{
    const EdsLib_ValueEntry_t *SelectedEntry;
    const EdsLib_ConstraintEntity_t *SelectedLocation;
    const EdsLib_ContainerDescriptor_t *DerivedContainerDesc;
    const EdsLib_IdentSequenceEntry_t *IdentSequencePtr;
    EdsLib_ConstPtr_t DataPtr;
    EdsLib_GenericValueBuffer_t ValueBuff;
    intmax_t CompareResult;
    int32_t Status;

    if (DataDictPtr == NULL || DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER)
    {
        return EDSLIB_INVALID_SIZE_OR_TYPE;
    }

    DerivedContainerDesc = DataDictPtr->Detail.Container;

    Status = EDSLIB_NO_MATCHING_VALUE;
    if (DerivedContainerDesc->IdentSequenceList != NULL)
    {
        IdentSequencePtr = &DerivedContainerDesc->IdentSequenceList[DerivedContainerDesc->IdentSequenceBase];
        while (1)
        {
            if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_VALUE_CONDITION)
            {
                SelectedEntry = &DerivedContainerDesc->ValueList[IdentSequencePtr->RefIdx];
                if (ValueBuff.ValueType == EDSLIB_BASICTYPE_SIGNED_INT)
                {
                    CompareResult = (ValueBuff.Value.SignedInteger - SelectedEntry->RefValue.Integer);
                }
                else if (ValueBuff.ValueType == EDSLIB_BASICTYPE_UNSIGNED_INT)
                {
                    CompareResult = (ValueBuff.Value.UnsignedInteger - SelectedEntry->RefValue.Unsigned);
                }
                else if (ValueBuff.ValueType == EDSLIB_BASICTYPE_BINARY)
                {
                    CompareResult = strncmp(ValueBuff.Value.StringData, SelectedEntry->RefValue.String,
                            sizeof(ValueBuff.Value.StringData));
                }
                else
                {
                    CompareResult = -1;
                }
            }
            else if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_ENTITY_LOCATION)
            {
                SelectedLocation = &DerivedContainerDesc->ConstraintEntityList[IdentSequencePtr->RefIdx];
                DataPtr.Ptr = Buffer;
                DataPtr.Addr += SelectedLocation->Offset.Bytes;
                EdsLib_DataTypeLoad_Impl(&ValueBuff, DataPtr, EdsLib_DataTypeDB_GetEntry(GD, &SelectedLocation->RefObj));
                if (ValueBuff.ValueType != EDSLIB_BASICTYPE_NONE)
                {
                    CompareResult = 0;
                }
                else
                {
                    CompareResult = -1;
                }
            }
            else
            {
                break;
            }

            if (CompareResult == 0)
            {
                /*
                 * Indicated operation was satisfied or successfully completed-
                 * Move to the next entry in the table for the next operation.
                 */
                --IdentSequencePtr;
            }
            else if (CompareResult > 0)
            {
                /*
                 * Runtime value was greater than reference value from DB,
                 * so jump following the greater than link
                 */
                IdentSequencePtr = &DerivedContainerDesc->IdentSequenceList
                        [IdentSequencePtr->NextOperationGreater];
            }
            else
            {
                /*
                 * Runtime value was less than reference value from DB,
                 * so jump following the less than link
                 */
                IdentSequencePtr = &DerivedContainerDesc->IdentSequenceList
                        [IdentSequencePtr->NextOperationLess];
            }
        }

        if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_RESULT)
        {
            Status = EDSLIB_SUCCESS;
            if (DerivTableIndex != NULL)
            {
                *DerivTableIndex = IdentSequencePtr->RefIdx;
            }
            if (ActualObj != NULL)
            {
                *ActualObj = DerivedContainerDesc->DerivativeList[IdentSequencePtr->RefIdx].RefObj;
            }
        }

    }

    return Status;
}


