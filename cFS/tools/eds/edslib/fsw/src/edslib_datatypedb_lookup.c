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
 * \file     edslib_datatypedb_lookup.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Internal API functions for dealing with the EDS-defined type identifiers
 * that are used in the external API.  These are safe indices into the internal database,
 * which can be converted into a pointer into the DB.
 *
 * Since the index value can be checked and verified before actually dereferencing the
 * data, it is safer to use than a direct memory pointer.  However all internal API calls
 * generally pass around direct DB object pointers for efficiency.
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
EdsLib_DataTypeDB_t EdsLib_DataTypeDB_GetTopLevel(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
{
    if (GD == NULL || GD->DataTypeDB_Table == NULL)
    {
        return NULL;
    }

    if (AppIdx >= GD->AppTableSize)
    {
        return NULL;
    }

    return GD->DataTypeDB_Table[AppIdx];
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_DataTypeDB_Entry_t *EdsLib_DataTypeDB_GetEntry(const EdsLib_DatabaseObject_t *GD,
                                                            const EdsLib_DatabaseRef_t    *RefObj)
{
    EdsLib_DataTypeDB_t              Dict;
    const EdsLib_DataTypeDB_Entry_t *DataDictEntry;
    const EdsLib_DatabaseRef_t      *CurrRef;

    DataDictEntry = NULL;
    CurrRef       = RefObj;

    while (true)
    {
        if (CurrRef == NULL || CurrRef->Qualifier != EdsLib_DbRef_Qualifier_DATATYPE)
        {
            break;
        }

        Dict = EdsLib_DataTypeDB_GetTopLevel(GD, CurrRef->AppIndex);
        if (Dict == NULL)
        {
            break;
        }

        if (CurrRef->SubIndex >= Dict->DataTypeTableSize)
        {
            break;
        }

        DataDictEntry = &Dict->DataTypeTable[CurrRef->SubIndex];

        if (DataDictEntry->BasicType != EDSLIB_BASICTYPE_ALIAS)
        {
            break;
        }

        CurrRef = &DataDictEntry->Detail.Alias.RefObj;
    }

    return DataDictEntry;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
void EdsLib_DataTypeDB_CopyTypeInfo(const EdsLib_DataTypeDB_Entry_t *DataDictEntry,
                                    EdsLib_DataTypeDB_TypeInfo_t    *TypeInfo)
{
    memset(TypeInfo, 0, sizeof(*TypeInfo));
    if (DataDictEntry != NULL)
    {
        TypeInfo->Size           = DataDictEntry->SizeInfo;
        TypeInfo->Flags          = DataDictEntry->Flags;
        TypeInfo->ElemType       = DataDictEntry->BasicType;
        TypeInfo->NumSubElements = DataDictEntry->NumSubElements;
    }
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_OffsetCompareBits(const void *SubjectPtr, const EdsLib_FieldDetailEntry_t *RefEntry)
{
    uint32_t OffsetVal = *((const uint32_t *)SubjectPtr);
    if (OffsetVal < RefEntry->Offset.Bits)
    {
        return -1;
    }
    if (OffsetVal > RefEntry->Offset.Bits)
    {
        return 1;
    }
    return 0;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int8_t EdsLib_OffsetCompareBytes(const void *SubjectPtr, const EdsLib_FieldDetailEntry_t *RefEntry)
{
    uint32_t OffsetVal = *((const uint32_t *)SubjectPtr);
    if (OffsetVal < RefEntry->Offset.Bytes)
    {
        return -1;
    }
    if (OffsetVal > RefEntry->Offset.Bytes)
    {
        return 1;
    }
    return 0;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
uint16_t EdsLib_GetArrayIdxFromBytes(const void *SubjectPtr, const EdsLib_SizeInfo_t *RefVal)
{
    uint32_t OffsetVal = *((const uint32_t *)SubjectPtr);

    return OffsetVal / RefVal->Bytes;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
uint16_t EdsLib_GetArrayIdxFromBits(const void *SubjectPtr, const EdsLib_SizeInfo_t *RefVal)
{
    uint32_t OffsetVal = *((const uint32_t *)SubjectPtr);

    return OffsetVal / RefVal->Bits;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_FindContainerMember_Impl(const EdsLib_DatabaseObject_t   *GD,
                                                   const EdsLib_DataTypeDB_Entry_t *BaseDict,
                                                   const void                      *CompareArg,
                                                   EdsLib_EntryCompareFunc_t        CompFunc,
                                                   EdsLib_DataTypeDB_EntityInfo_t  *EntInfo)
{
    const EdsLib_FieldDetailEntry_t *ContEntry;
    const EdsLib_FieldDetailEntry_t *NextEntry;
    const EdsLib_DataTypeDB_Entry_t *MemberDict;
    uint32_t                         Flags;
    int32_t                          Result;
    uint16_t                         Idx;

    ContEntry = BaseDict->Detail.Container->EntryList;
    NextEntry = NULL;
    Flags     = 0;

    /*
     * Locate the entry _prior_ to the one where the offset exceeds
     * the specified offset
     */
    for (Idx = 1; Idx < BaseDict->NumSubElements; ++Idx)
    {
        /* Note, This is peeking at the start of the NEXT entry */
        NextEntry = &ContEntry[Idx];
        if (CompFunc(CompareArg, NextEntry) < 0)
        {
            break;
        }
        if ((BaseDict->Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
        {
            MemberDict = EdsLib_DataTypeDB_GetEntry(GD, &ContEntry->RefObj);
            if (MemberDict != NULL && (MemberDict->Flags & EDSLIB_DATATYPE_FLAG_VARIABLE_SIZE) != 0)
            {
                /* anything following a variably-sized member is a variable location */
                Flags |= EDSLIB_DATATYPE_FLAG_VARIABLE_LOCATION;
            }
        }
        ++ContEntry;
        NextEntry = NULL;
    }

    if (ContEntry != NULL && CompFunc(CompareArg, ContEntry) >= 0)
    {
        MemberDict = EdsLib_DataTypeDB_GetEntry(GD, &ContEntry->RefObj);
        if (MemberDict != NULL)
        {
            Flags |= MemberDict->Flags;
        }

        EntInfo->EdsId  = EdsLib_Encode_StructId(&ContEntry->RefObj);
        EntInfo->Offset = ContEntry->Offset;
        EntInfo->Flags  = Flags;
        if (NextEntry != NULL)
        {
            /* Not last element, peek at offset of _next_ element to get max size */
            EntInfo->MaxSize = NextEntry->Offset;
        }
        else
        {
            /* is last element - so use size of parent as max size */
            EntInfo->MaxSize = BaseDict->SizeInfo;
        }
        EntInfo->MaxSize.Bytes -= EntInfo->Offset.Bytes;
        EntInfo->MaxSize.Bits  -= EntInfo->Offset.Bits;

        Result = EDSLIB_SUCCESS;
    }
    else
    {
        Result = EDSLIB_INVALID_OFFSET;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_DataTypeDB_FindArrayMember_Impl(const EdsLib_DatabaseObject_t   *GD,
                                               const EdsLib_DataTypeDB_Entry_t *BaseDict,
                                               const void                      *GetIdxArg,
                                               EdsLib_GetArrayIdxFunc_t         GetIdxFunc,
                                               EdsLib_DataTypeDB_EntityInfo_t  *EntInfo)
{
    EdsLib_SizeInfo_t ElementSize;
    int32_t           Result;
    uint16_t          SubIndex;

    /*
     * Calculating the start offset this way, although it involves division,
     * does not require that we actually _have_ the definition of the array
     * element type loaded.  The total byte size of the entire array _must_
     * be an integer multiple of the array length, by definition.
     */
    ElementSize.Bytes = BaseDict->SizeInfo.Bytes / BaseDict->NumSubElements;
    ElementSize.Bits  = BaseDict->SizeInfo.Bits / BaseDict->NumSubElements;
    SubIndex          = GetIdxFunc(GetIdxArg, &ElementSize);

    if (SubIndex < BaseDict->NumSubElements)
    {
        EntInfo->EdsId        = EdsLib_Encode_StructId(&BaseDict->Detail.Array->ElementRefObj);
        EntInfo->MaxSize      = ElementSize;
        EntInfo->Offset.Bytes = ElementSize.Bytes * SubIndex;
        EntInfo->Offset.Bits  = ElementSize.Bits * SubIndex;

        Result = EDSLIB_SUCCESS;
    }
    else
    {
        Result = EDSLIB_INVALID_OFFSET;
    }

    return Result;
}
