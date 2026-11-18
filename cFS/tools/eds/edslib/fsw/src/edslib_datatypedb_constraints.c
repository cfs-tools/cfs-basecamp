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

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_ConstraintIterator_Impl(const EdsLib_DatabaseObject_t            *GD,
                                                  EdsLib_ConstraintIterator_ControlBlock_t *CtlBlock,
                                                  const EdsLib_DatabaseRef_t               *BaseRef)
{
    const EdsLib_DataTypeDB_Entry_t      *DataDictPtr;
    const EdsLib_ContainerDescriptor_t   *DerivDescPtr;
    const EdsLib_IdentityCheckSequence_t *DerivativeCheck;
    const EdsLib_IdentSequenceEntry_t    *IdentSequencePtr;
    const EdsLib_DerivativeEntry_t       *DerivEntryPtr;
    const EdsLib_ValueEntry_t            *SelectedEntry;
    const EdsLib_ConstraintEntity_t      *SelectedLocation;
    uint16_t                              DerivIdx;
    int32_t                               Status;

    SelectedEntry    = NULL;
    SelectedLocation = NULL;

    DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, BaseRef);
    if (DataDictPtr == NULL || DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER)
    {
        return EDSLIB_INVALID_SIZE_OR_TYPE;
    }

    DerivDescPtr = DataDictPtr->Detail.Container;
    if (DerivDescPtr == NULL)
    {
        DerivativeCheck = NULL;
    }
    else
    {
        DerivativeCheck = DerivDescPtr->CheckSequence;
    }

    if (DerivativeCheck == NULL)
    {
        return EDSLIB_NO_MATCHING_VALUE;
    }

    Status        = EDSLIB_NO_MATCHING_VALUE;
    DerivEntryPtr = DerivDescPtr->DerivativeList;
    for (DerivIdx = 0; DerivIdx < DerivDescPtr->DerivativeListSize; ++DerivIdx)
    {
        if (EdsLib_DatabaseRef_IsEqual(&DerivEntryPtr->RefObj, &CtlBlock->TargetRef))
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
    IdentSequencePtr                        = &DerivativeCheck->SequenceList[DerivEntryPtr->IdentSeqIdx];
    CtlBlock->TempConstraintValue.ValueType = EDSLIB_BASICTYPE_NONE;
    while (IdentSequencePtr->EntryType != EDSLIB_IDENT_SEQUENCE_INVALID)
    {
        if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_CHECK_CONDITION)
        {
            SelectedEntry = &DerivativeCheck->ValueList[IdentSequencePtr->Datum];
        }
        else if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_ENTITY_LOCATION)
        {
            SelectedLocation = &DerivativeCheck->ConstraintEntityList[IdentSequencePtr->Datum];
            if (SelectedEntry != NULL)
            {
                DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &SelectedLocation->RefObj);
                if (DataDictPtr != NULL && SelectedEntry->ConstraintStyle == EDSLIB_VALUE_CONSTRAINTSTYLE_SINGLE_VALUE)
                {
                    if (SelectedEntry->BasicType == EDSLIB_BASICTYPE_SIGNED_INT)
                    {
                        CtlBlock->TempConstraintValue.Value.SignedInteger = SelectedEntry->RefValue.Single.SignedInt;
                        CtlBlock->TempConstraintValue.ValueType           = SelectedEntry->BasicType;
                    }
                    else if (SelectedEntry->BasicType == EDSLIB_BASICTYPE_UNSIGNED_INT)
                    {
                        CtlBlock->TempConstraintValue.Value.UnsignedInteger =
                            SelectedEntry->RefValue.Single.UnsignedInt;
                        CtlBlock->TempConstraintValue.ValueType = SelectedEntry->BasicType;
                    }
                    else if (SelectedEntry->BasicType == EDSLIB_BASICTYPE_FLOAT)
                    {
                        CtlBlock->TempConstraintValue.Value.FloatingPoint = SelectedEntry->RefValue.Single.FloatingPt;
                        CtlBlock->TempConstraintValue.ValueType           = SelectedEntry->BasicType;
                    }
                    else if (SelectedEntry->BasicType == EDSLIB_BASICTYPE_BINARY)
                    {
                        strncpy(CtlBlock->TempConstraintValue.Value.StringData,
                                SelectedEntry->RefValue.Single.String,
                                sizeof(CtlBlock->TempConstraintValue.Value.StringData));
                        CtlBlock->TempConstraintValue.ValueType = SelectedEntry->BasicType;
                    }
                }
                if (CtlBlock->TempConstraintValue.ValueType != EDSLIB_BASICTYPE_NONE)
                {
                    CtlBlock->TempMemberInfo.EdsId   = EdsLib_Encode_StructId(&SelectedLocation->RefObj);
                    CtlBlock->TempMemberInfo.Offset  = SelectedLocation->Offset;
                    CtlBlock->TempMemberInfo.MaxSize = DataDictPtr->SizeInfo;
                    CtlBlock->UserCallback(GD,
                                           &CtlBlock->TempMemberInfo,
                                           &CtlBlock->TempConstraintValue,
                                           CtlBlock->CbArg);
                }
                CtlBlock->TempConstraintValue.ValueType = EDSLIB_BASICTYPE_NONE;
                SelectedEntry                           = NULL;
            }
        }
        IdentSequencePtr = &DerivativeCheck->SequenceList[IdentSequencePtr->ParentOperation];
    }

    return EDSLIB_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Compare two numeric values
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_DataType_CompareSignedInt(EdsLib_Generic_SignedInt_t Value1, EdsLib_Generic_SignedInt_t Value2)
{
    int8_t Result;

    if (Value1 < Value2)
    {
        Result = -1;
    }
    else if (Value1 > Value2)
    {
        Result = 1;
    }
    else
    {
        Result = 0;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Compare two numeric values
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_DataType_CompareUnsignedInt(EdsLib_Generic_UnsignedInt_t Value1, EdsLib_Generic_UnsignedInt_t Value2)
{
    int8_t Result;

    if (Value1 < Value2)
    {
        Result = -1;
    }
    else if (Value1 > Value2)
    {
        Result = 1;
    }
    else
    {
        Result = 0;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Compare two numeric values
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_DataType_CompareFloatingPt(EdsLib_Generic_FloatingPoint_t Value1, EdsLib_Generic_FloatingPoint_t Value2)
{
    int8_t Result;

    if (Value1 < Value2)
    {
        Result = -1;
    }
    else if (Value1 > Value2)
    {
        Result = 1;
    }
    else
    {
        Result = 0;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Compare two numeric values
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_DataType_CompareGeneric(const EdsLib_GenericValueBuffer_t *Value1, EdsLib_GenericValueBuffer_t *Value2)
{
    int8_t Result;

    /* ensure the data is consistently typed - value2 will be coerced into whatever type value1 is */
    EdsLib_DataTypeConvert(Value2, Value1->ValueType);
    switch (Value1->ValueType)
    {
        case EDSLIB_BASICTYPE_UNSIGNED_INT:
            Result = EdsLib_DataType_CompareUnsignedInt(Value1->Value.UnsignedInteger, Value2->Value.UnsignedInteger);
            break;
        case EDSLIB_BASICTYPE_SIGNED_INT:
            Result = EdsLib_DataType_CompareSignedInt(Value1->Value.SignedInteger, Value2->Value.SignedInteger);
            break;
        case EDSLIB_BASICTYPE_FLOAT:
            Result = EdsLib_DataType_CompareUnsignedInt(Value1->Value.FloatingPoint, Value2->Value.FloatingPoint);
            break;
        case EDSLIB_BASICTYPE_BINARY:
            Result = strncmp(Value1->Value.StringData, Value2->Value.StringData, sizeof(Value1->Value));
            break;
        default:
            Result = -1;
            break;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Load a reference value from the DB
 *
 *-----------------------------------------------------------------*/
void EdsLib_DataType_IdentLoadReference(EdsLib_GenericValueBuffer_t *ValuePtr,
                                        EdsLib_BasicType_t           RefType,
                                        const EdsLib_SingleValue_t  *RefValPtr)
{
    ValuePtr->ValueType = RefType;
    switch (RefType)
    {
        case EDSLIB_BASICTYPE_UNSIGNED_INT:
            ValuePtr->Value.UnsignedInteger = RefValPtr->UnsignedInt;
            break;
        case EDSLIB_BASICTYPE_SIGNED_INT:
            ValuePtr->Value.SignedInteger = RefValPtr->SignedInt;
            break;
        case EDSLIB_BASICTYPE_FLOAT:
            ValuePtr->Value.FloatingPoint = RefValPtr->FloatingPt;
            break;
        case EDSLIB_BASICTYPE_BINARY:
            strncpy(ValuePtr->Value.StringData, RefValPtr->String, sizeof(ValuePtr->Value));
            break;
        default:
            memset(ValuePtr, 0, sizeof(*ValuePtr));
            break;
    }
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Implement single value check
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_DataType_IdentValueCheckSingle(const EdsLib_ValueEntry_t         *SelectedEntry,
                                             const EdsLib_GenericValueBuffer_t *ValuePtr)
{
    EdsLib_GenericValueBuffer_t RefValue;

    /* this case is simple */
    EdsLib_DataType_IdentLoadReference(&RefValue, SelectedEntry->BasicType, &SelectedEntry->RefValue.Single);
    return EdsLib_DataType_CompareGeneric(ValuePtr, &RefValue);
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Implement range value check
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_DataType_IdentValueCheckRange(const EdsLib_ValueEntry_t         *SelectedEntry,
                                            const EdsLib_GenericValueBuffer_t *ValuePtr)
{
    EdsLib_GenericValueBuffer_t RefTemp;
    int8_t                      MinResult;
    int8_t                      MaxResult;
    int8_t                      MinReq;
    int8_t                      MaxReq;

    /* load the minimum, if applicable */
    switch (SelectedEntry->InclusionStyle)
    {
        case EDSLIB_VALUE_INCLUSIONSTYLE_NONE:
            MinResult = -1;
            break;

        case EDSLIB_VALUE_INCLUSIONSTYLE_INCLUSIVE_MIN_INCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_INCLUSIVE_MIN_EXCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_EXCLUSIVE_MIN_INCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_EXCLUSIVE_MIN_EXCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_AT_LEAST:
        case EDSLIB_VALUE_INCLUSIONSTYLE_GREATER_THAN:
            EdsLib_DataType_IdentLoadReference(&RefTemp, SelectedEntry->BasicType, &SelectedEntry->RefValue.Pair.Min);
            MinResult = EdsLib_DataType_CompareGeneric(ValuePtr, &RefTemp);
            break;

        default:
            MinResult = 1;
            break;
    }

    /* load the maximum, if applicable */
    switch (SelectedEntry->InclusionStyle)
    {
        case EDSLIB_VALUE_INCLUSIONSTYLE_NONE:
            MaxResult = 1;
            break;

        case EDSLIB_VALUE_INCLUSIONSTYLE_INCLUSIVE_MIN_INCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_INCLUSIVE_MIN_EXCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_EXCLUSIVE_MIN_INCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_EXCLUSIVE_MIN_EXCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_LESS_THAN:
        case EDSLIB_VALUE_INCLUSIONSTYLE_AT_MOST:
            EdsLib_DataType_IdentLoadReference(&RefTemp, SelectedEntry->BasicType, &SelectedEntry->RefValue.Pair.Max);
            MaxResult = EdsLib_DataType_CompareGeneric(ValuePtr, &RefTemp);
            break;

        default:
            MaxResult = -1;
            break;
    }

    switch (SelectedEntry->InclusionStyle)
    {
        case EDSLIB_VALUE_INCLUSIONSTYLE_INCLUSIVE_MIN_INCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_INCLUSIVE_MIN_EXCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_AT_LEAST:
            MinReq = 0;
            break;

        default:
            MinReq = 1;
            break;
    }

    switch (SelectedEntry->InclusionStyle)
    {
        case EDSLIB_VALUE_INCLUSIONSTYLE_INCLUSIVE_MIN_INCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_EXCLUSIVE_MIN_INCLUSIVE_MAX:
        case EDSLIB_VALUE_INCLUSIONSTYLE_AT_MOST:
            MaxReq = 0;
            break;

        default:
            MaxReq = -1;
            break;
    }

    if (MinResult < MinReq)
    {
        return -1;
    }
    else if (MaxResult > MaxReq)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function function
 * Implement value check supporting both single and range
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_DataType_IdentValueCheck(const EdsLib_ValueEntry_t         *SelectedEntry,
                                       const EdsLib_GenericValueBuffer_t *ValuePtr)
{
    int8_t Result;

    switch (SelectedEntry->ConstraintStyle)
    {
        case EDSLIB_VALUE_CONSTRAINTSTYLE_SINGLE_VALUE:
        {
            Result = EdsLib_DataType_IdentValueCheckSingle(SelectedEntry, ValuePtr);
            break;
        }

        case EDSLIB_VALUE_CONSTRAINTSTYLE_RANGE_VALUE:
        {
            Result = EdsLib_DataType_IdentValueCheckRange(SelectedEntry, ValuePtr);
            break;
        }

        default:
        {
            Result = -1;
            break;
        }
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_IdentSequenceEntry_t *
EdsLib_DataTypeExecuteConstraintSequence_Impl(const EdsLib_DatabaseObject_t        *GD,
                                              const EdsLib_IdentityCheckSequence_t *CheckSequence,
                                              const void                           *Buffer,
                                              size_t                                MaxOffset)
{
    const EdsLib_ValueEntry_t         *SelectedValue;
    const EdsLib_ConstraintEntity_t   *SelectedConstraint;
    const EdsLib_IdentSequenceEntry_t *IdentSequencePtr;
    const EdsLib_DataTypeDB_Entry_t   *DataTypePtr;
    EdsLib_ConstPtr_t                  SrcPtr;
    EdsLib_GenericValueBuffer_t        ValueBuff;
    int8_t                             CompareResult;
    uint16_t                           CurrSeqPos;

    CurrSeqPos = CheckSequence->SequenceEntryIdx;
    do
    {
        if (CurrSeqPos > CheckSequence->SequenceEntryIdx)
        {
            CurrSeqPos = 0;
        }

        IdentSequencePtr = &CheckSequence->SequenceList[CurrSeqPos];

        if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_CHECK_CONDITION)
        {
            SelectedValue = &CheckSequence->ValueList[IdentSequencePtr->Datum];
            CompareResult = EdsLib_DataType_IdentValueCheck(SelectedValue, &ValueBuff);
        }
        else if (IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_ENTITY_LOCATION)
        {
            SelectedConstraint = &CheckSequence->ConstraintEntityList[IdentSequencePtr->Datum];
            DataTypePtr        = EdsLib_DataTypeDB_GetEntry(GD, &SelectedConstraint->RefObj);
            if (DataTypePtr != NULL && (SelectedConstraint->Offset.Bytes + DataTypePtr->SizeInfo.Bytes) <= MaxOffset)
            {
                SrcPtr.Ptr = (const uint8_t *)Buffer + SelectedConstraint->Offset.Bytes;
                EdsLib_DataTypeLoad_Impl(&ValueBuff, SrcPtr, DataTypePtr);
            }
            else
            {
                ValueBuff.ValueType = EDSLIB_BASICTYPE_NONE;
            }
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
            /* got to the end, this is always the "result" regardless of what it is */
            /* The caller should interpret this as a default result */
            break;
        }

        if (CompareResult > 0)
        {
            /*
             * Runtime value was greater than reference value from DB,
             * so jump following the greater than link
             */
            CurrSeqPos = IdentSequencePtr->JumpIfGreater;
        }
        else if (CompareResult < 0)
        {
            /*
             * Runtime value was less than reference value from DB,
             * so jump following the less than link
             */
            CurrSeqPos = IdentSequencePtr->JumpIfLess;
        }
        else
        {
            /* a "match" means simply move to the next item */
            --CurrSeqPos;
        }
    } while (true);

    return IdentSequencePtr;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeIdentifyBuffer_Impl(const EdsLib_DatabaseObject_t   *GD,
                                           const EdsLib_DataTypeDB_Entry_t *DataDictPtr,
                                           const void                      *Buffer,
                                           size_t                           MaxOffset,
                                           uint16_t                        *DerivTableIndex,
                                           EdsLib_DatabaseRef_t            *ActualObj)
{
    const EdsLib_ContainerDescriptor_t *DerivedContainerDesc;
    const EdsLib_IdentSequenceEntry_t  *IdentSequencePtr;
    int32_t                             Status;

    if (DataDictPtr == NULL || DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER)
    {
        return EDSLIB_INVALID_SIZE_OR_TYPE;
    }

    DerivedContainerDesc = DataDictPtr->Detail.Container;

    Status = EDSLIB_NO_MATCHING_VALUE;
    if (DerivedContainerDesc->CheckSequence != NULL)
    {
        IdentSequencePtr =
            EdsLib_DataTypeExecuteConstraintSequence_Impl(GD, DerivedContainerDesc->CheckSequence, Buffer, MaxOffset);

        if (IdentSequencePtr != NULL && IdentSequencePtr->EntryType == EDSLIB_IDENT_SEQUENCE_REFOBJ_RESULT)
        {
            Status = EDSLIB_SUCCESS;
            if (DerivTableIndex != NULL)
            {
                *DerivTableIndex = IdentSequencePtr->Datum;
            }
            if (ActualObj != NULL)
            {
                *ActualObj = DerivedContainerDesc->DerivativeList[IdentSequencePtr->Datum].RefObj;
            }
        }
    }

    return Status;
}
