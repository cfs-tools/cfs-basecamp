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
 * \file     edslib_datatypedb_load_store.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Operations to load/store data members within EDS-defined objects
 */

#include <stdlib.h>
#include <string.h>

#include "edslib_datatypedb.h"
#include "edslib_internal.h"

void EdsLib_DataTypeLoad_Impl(EdsLib_GenericValueBuffer_t *ValueBuff, EdsLib_ConstPtr_t SrcPtr, const EdsLib_DataTypeDB_Entry_t *DictEntryPtr)
{
    EdsLib_BasicType_t SubjectType;
    uint32_t SubjectSize;

    /*
     * In keeping with existing paradigms in other internal API calls, this should deal with
     * the dictionary pointer being NULL and just return something reasonable.
     */
    if (DictEntryPtr == NULL)
    {
        SubjectType = EDSLIB_BASICTYPE_NONE;
        SubjectSize = 0;
    }
    else
    {
        SubjectType = DictEntryPtr->BasicType;
        SubjectSize = DictEntryPtr->SizeInfo.Bytes;
    }

    ValueBuff->ValueType = SubjectType;
    switch(EDSLIB_TYPE_AND_SIZE(SubjectSize, SubjectType))
    {
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.i8), EDSLIB_BASICTYPE_SIGNED_INT):
        ValueBuff->Value.SignedInteger = SrcPtr.u->i8;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.u8), EDSLIB_BASICTYPE_UNSIGNED_INT):
        ValueBuff->Value.UnsignedInteger = SrcPtr.u->u8;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.i16), EDSLIB_BASICTYPE_SIGNED_INT):
        ValueBuff->Value.SignedInteger = SrcPtr.u->i16;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.u16), EDSLIB_BASICTYPE_UNSIGNED_INT):
        ValueBuff->Value.UnsignedInteger = SrcPtr.u->u16;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.i32), EDSLIB_BASICTYPE_SIGNED_INT):
        ValueBuff->Value.SignedInteger = SrcPtr.u->i32;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.u32), EDSLIB_BASICTYPE_UNSIGNED_INT):
        ValueBuff->Value.UnsignedInteger = SrcPtr.u->u32;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.i64), EDSLIB_BASICTYPE_SIGNED_INT):
        ValueBuff->Value.SignedInteger = SrcPtr.u->i64;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.u64), EDSLIB_BASICTYPE_UNSIGNED_INT):
        ValueBuff->Value.UnsignedInteger = SrcPtr.u->u64;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(ValueBuff->Value.fpsgl), EDSLIB_BASICTYPE_FLOAT):
        ValueBuff->Value.FloatingPoint = SrcPtr.u->fpsgl;
        break;
    default:
        if (SubjectType == EDSLIB_BASICTYPE_FLOAT)
        {
            /* NOTE - this check for max-precision float is done in the default case because
             *  on some machines that do not have a long double, it is identical to double.
             * this avoids a potential "duplicate case" error on those machines.
             * Other machines might have float and double as the same width. */
            if (SubjectSize == sizeof(ValueBuff->Value.fpmax))
            {
                ValueBuff->Value.FloatingPoint = SrcPtr.u->fpmax;
            }
            else if (SubjectSize == sizeof(ValueBuff->Value.fpdbl))
            {
                ValueBuff->Value.FloatingPoint = SrcPtr.u->fpdbl;
            }
            else
            {
                ValueBuff->ValueType = EDSLIB_BASICTYPE_NONE;
                ValueBuff->Value.UnsignedInteger = 0;
            }
        }
        else if (SubjectType == EDSLIB_BASICTYPE_BINARY && SubjectSize <= sizeof(ValueBuff->Value.StringData))
        {
            strncpy(ValueBuff->Value.StringData, SrcPtr.Ptr, SubjectSize);
        }
        else
        {
            ValueBuff->ValueType = EDSLIB_BASICTYPE_NONE;
            ValueBuff->Value.UnsignedInteger = 0;
        }
        break;
    }
}

void EdsLib_DataTypeConvert(EdsLib_GenericValueBuffer_t *ValueBuff, EdsLib_BasicType_t DesiredType)
{
    switch(EDSLIB_TYPE_AND_SIZE(DesiredType, ValueBuff->ValueType))
    {
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_SIGNED_INT, EDSLIB_BASICTYPE_UNSIGNED_INT):
        ValueBuff->Value.SignedInteger = ValueBuff->Value.UnsignedInteger;
        ValueBuff->ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
        break;
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_FLOAT, EDSLIB_BASICTYPE_UNSIGNED_INT):
        ValueBuff->Value.FloatingPoint = ValueBuff->Value.UnsignedInteger;
        ValueBuff->ValueType = EDSLIB_BASICTYPE_FLOAT;
        break;
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_UNSIGNED_INT, EDSLIB_BASICTYPE_SIGNED_INT):
        ValueBuff->Value.UnsignedInteger = ValueBuff->Value.SignedInteger;
        ValueBuff->ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
        break;
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_FLOAT, EDSLIB_BASICTYPE_SIGNED_INT):
        ValueBuff->Value.FloatingPoint = ValueBuff->Value.SignedInteger;
        ValueBuff->ValueType = EDSLIB_BASICTYPE_FLOAT;
        break;
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_SIGNED_INT, EDSLIB_BASICTYPE_FLOAT):
        ValueBuff->Value.SignedInteger = ValueBuff->Value.FloatingPoint;
        ValueBuff->ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
        break;
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_UNSIGNED_INT, EDSLIB_BASICTYPE_FLOAT):
        ValueBuff->Value.UnsignedInteger = ValueBuff->Value.FloatingPoint;
        ValueBuff->ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
        break;
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_FLOAT, EDSLIB_BASICTYPE_FLOAT):
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_SIGNED_INT, EDSLIB_BASICTYPE_SIGNED_INT):
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_UNSIGNED_INT, EDSLIB_BASICTYPE_UNSIGNED_INT):
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_BINARY, EDSLIB_BASICTYPE_BINARY):
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_ARRAY, EDSLIB_BASICTYPE_ARRAY):
    case EDSLIB_TYPE_AND_SIZE(EDSLIB_BASICTYPE_CONTAINER, EDSLIB_BASICTYPE_CONTAINER):
        /* No changes to ValueBuff */
        break;
    default:
        /* Invalid */
        ValueBuff->ValueType = EDSLIB_BASICTYPE_NONE;
        break;
    }
}


void EdsLib_DataTypeStore_Impl(EdsLib_Ptr_t DstPtr, EdsLib_GenericValueBuffer_t *ValueBuff, const EdsLib_DataTypeDB_Entry_t *DictEntryPtr)
{
    EdsLib_BasicType_t SubjectType;
    uint32_t SubjectSize;

    /*
     * In keeping with existing paradigms in other internal API calls, this should deal with
     * the dictionary pointer being NULL and just return something reasonable.
     */
    if (DictEntryPtr == NULL)
    {
        SubjectType = EDSLIB_BASICTYPE_NONE;
        SubjectSize = 0;
    }
    else
    {
        SubjectType = DictEntryPtr->BasicType;
        SubjectSize = DictEntryPtr->SizeInfo.Bytes;
    }

    EdsLib_DataTypeConvert(ValueBuff, SubjectType);

    switch(EDSLIB_TYPE_AND_SIZE(SubjectSize, ValueBuff->ValueType))
    {
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->i8), EDSLIB_BASICTYPE_SIGNED_INT):
        DstPtr.u->i8 = ValueBuff->Value.SignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->u8), EDSLIB_BASICTYPE_UNSIGNED_INT):
        DstPtr.u->u8 = ValueBuff->Value.UnsignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->i16), EDSLIB_BASICTYPE_SIGNED_INT):
        DstPtr.u->i16 = ValueBuff->Value.SignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->u16), EDSLIB_BASICTYPE_UNSIGNED_INT):
        DstPtr.u->u16 = ValueBuff->Value.UnsignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->i32), EDSLIB_BASICTYPE_SIGNED_INT):
        DstPtr.u->i32 = ValueBuff->Value.SignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->u32), EDSLIB_BASICTYPE_UNSIGNED_INT):
        DstPtr.u->u32 = ValueBuff->Value.UnsignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->i64), EDSLIB_BASICTYPE_SIGNED_INT):
        DstPtr.u->i64 = ValueBuff->Value.SignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->u64), EDSLIB_BASICTYPE_UNSIGNED_INT):
        DstPtr.u->u64 = ValueBuff->Value.UnsignedInteger;
        break;
    case EDSLIB_TYPE_AND_SIZE(sizeof(DstPtr.u->fpsgl), EDSLIB_BASICTYPE_FLOAT):
        DstPtr.u->fpsgl = ValueBuff->Value.FloatingPoint;
        break;
    default:
        if (SubjectType == EDSLIB_BASICTYPE_FLOAT)
        {
            /* floating point sizes checked here, since on some machines
             * the size of double/long double might be the same.
             * This would result in a duplicate case value */
            if (SubjectSize == sizeof(ValueBuff->Value.fpmax))
            {
                DstPtr.u->fpmax = ValueBuff->Value.FloatingPoint;
            }
            else if (SubjectSize == sizeof(ValueBuff->Value.fpdbl))
            {
                DstPtr.u->fpdbl = ValueBuff->Value.FloatingPoint;
            }
            else
            {
                /* invalid size */
                ValueBuff->ValueType = EDSLIB_BASICTYPE_NONE;
            }
        }
        else if (SubjectType == EDSLIB_BASICTYPE_BINARY &&
                    SubjectSize <= sizeof(ValueBuff->Value.StringData))
        {
            strncpy(DstPtr.Ptr, ValueBuff->Value.StringData, SubjectSize);
        }
        else
        {
            ValueBuff->ValueType = EDSLIB_BASICTYPE_NONE;
        }
        break;
    }
}


