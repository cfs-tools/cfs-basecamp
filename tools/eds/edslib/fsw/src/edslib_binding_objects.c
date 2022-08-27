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
 * \file     edslib_binding_objects.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
**    Generic high-level EDS "binding" objects.
**    This defines a descriptor type which wraps together all EDS metadata along
**    with an object pointer to get an all-inclusive, self contained "smart object".
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "edslib_id.h"
#include "edslib_datatypedb.h"
#include "edslib_displaydb.h"
#include "edslib_binding_objects.h"

union EdsLib_Binding_AllocBuf
{
    EdsLib_Binding_Buffer_Content_t Content;
    uint8_t RawData[4];

    /*
     * The next fields are not intended to be used directly,
     * but rather to ensure that the overall union is appropriately
     * aligned within the container so that it can safely hold this data
     * if deemed necessary.  This should reflect the largest native machine
     * types.
     */
    void *Ptr;
    intmax_t AlignInt;
    long double AlignFloat;

};

typedef union EdsLib_Binding_AllocBuf EdsLib_Binding_AllocBuf_t;


/*
 * Initialization routine placeholder.
 *
 * Currently a no-op, as there are no global data
 * structures needing initialization at this time.
 *
 * For now, this is a placeholder in case that changes,
 * but this also serves an important role for static linking
 * to ensure that this code is linked into the target.
 */
void EdsLib_Binding_Initialize(void)
{
}



/*************************************************************
 * Constructors and Destructors for EdsLib binding objects
 */

void EdsLib_Binding_SetDescBuffer(EdsLib_Binding_DescriptorObject_t *DescrObj, EdsLib_Binding_Buffer_Content_t *TargetBuffer)
{
    EdsLib_Binding_Buffer_Content_t *PrevContentPtr = DescrObj->BufferPtr;

    /* If reassigning the same buffer, treat as a no-op */
    if (TargetBuffer != PrevContentPtr)
    {
        /* First release any previously-attached buffer (do nothing if NULL) */
        if (PrevContentPtr != NULL)
        {
            if (PrevContentPtr->ReferenceCount > 0)
            {
                --PrevContentPtr->ReferenceCount;
            }
            /* Managed objects should be freed whenever the RefereceCount goes down to 0. */
            if (PrevContentPtr->IsManaged && PrevContentPtr->ReferenceCount == 0)
            {
                free(PrevContentPtr);
            }
        }

        DescrObj->BufferPtr = TargetBuffer;

        /* Increase the reference count of the buffer now in use, if it is a managed buffer */
        if (TargetBuffer != NULL)
        {
            ++TargetBuffer->ReferenceCount;
        }
    }
}

EdsLib_Binding_Buffer_Content_t *EdsLib_Binding_AllocManagedBuffer(size_t MaxContentSize)
{
    EdsLib_Binding_AllocBuf_t *BufPtr;
    EdsLib_Binding_Buffer_Content_t *ContentPtr;
    size_t ActualSize;

    ActualSize = sizeof(EdsLib_Binding_AllocBuf_t) + MaxContentSize;
    BufPtr = malloc(ActualSize);

    if (BufPtr != NULL)
    {
        memset(BufPtr, 0, ActualSize);
        BufPtr->Content.Data = BufPtr[1].RawData;
        BufPtr->Content.MaxContentSize = MaxContentSize;
        BufPtr->Content.IsManaged = true;   /* mark it as "managed", this means it will be auto deleted */
        ContentPtr = &BufPtr->Content;
    }
    else
    {
        ContentPtr = NULL;
    }

    return ContentPtr;
}

EdsLib_Binding_Buffer_Content_t *EdsLib_Binding_InitUnmanagedBuffer(EdsLib_Binding_Buffer_Content_t *ContentPtr, void *DataPtr, size_t MaxContentSize)
{
    ContentPtr->Data = DataPtr;
    ContentPtr->MaxContentSize = MaxContentSize;
    ContentPtr->IsManaged = false;   /* mark it as "not managed", so it will NOT be auto-deleted */
    ContentPtr->ReferenceCount = 0;

    return ContentPtr;
}

void EdsLib_Binding_InitDescriptor(EdsLib_Binding_DescriptorObject_t *ObjectUserData,
        const EdsLib_DatabaseObject_t *EdsDB,
        EdsLib_Id_t EdsId)
{
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;

    memset(ObjectUserData,0,sizeof(*ObjectUserData));
    ObjectUserData->GD = EdsDB;
    ObjectUserData->EdsId = EdsId;
    EdsLib_DataTypeDB_GetDerivedInfo(EdsDB, EdsId, &DerivInfo);
    ObjectUserData->Length = DerivInfo.MaxSize.Bytes;
    EdsLib_DataTypeDB_GetTypeInfo(EdsDB, EdsId, &ObjectUserData->TypeInfo);
}

void EdsLib_Binding_InitSubObject(EdsLib_Binding_DescriptorObject_t *SubObject, const EdsLib_Binding_DescriptorObject_t *ParentObj, const EdsLib_DataTypeDB_EntityInfo_t *Component)
{
    memset(SubObject,0,sizeof(*SubObject));
    EdsLib_Binding_SetDescBuffer(SubObject, ParentObj->BufferPtr);
    SubObject->GD = ParentObj->GD;
    SubObject->EdsId = Component->EdsId;
    SubObject->Offset = Component->Offset.Bytes + ParentObj->Offset;
    SubObject->Length = Component->MaxSize.Bytes;
    EdsLib_DataTypeDB_GetTypeInfo(SubObject->GD, SubObject->EdsId, &SubObject->TypeInfo);
}

EdsLib_Binding_Compatibility_t EdsLib_Binding_CheckEdsObjectsCompatible(
        EdsLib_Binding_DescriptorObject_t *DestObject,
        EdsLib_Binding_DescriptorObject_t *SrcObject)
{
    EdsLib_Binding_Compatibility_t result =
            EDSLIB_BINDING_COMPATIBILITY_NONE;

    /*
     * Determine assignment compatibility...
     * It is preferable to directly copy data from src to dest but
     * this is only possible when the native objects are compatible.
     */
    if (DestObject->TypeInfo.NumSubElements == 0 &&
            SrcObject->TypeInfo.NumSubElements == 0)
    {
        if (SrcObject->TypeInfo.ElemType == DestObject->TypeInfo.ElemType &&
                SrcObject->TypeInfo.Size.Bytes == DestObject->TypeInfo.Size.Bytes)
        {
            /* Objects are both scalars of the same native type & size -- direct copy is possible
             * note these could be from different databases -- should not matter */
            result = EDSLIB_BINDING_COMPATIBILITY_EXACT;
        }
    }
    else if (SrcObject->GD == DestObject->GD)
    {
        if (SrcObject->EdsId == DestObject->EdsId)
        {
            /*
             * Objects are the exact same EDS type -- no problem
             * This covers containers and arrays as well, unlike the previous check
             */
            result = EDSLIB_BINDING_COMPATIBILITY_EXACT;
        }
        else if (EdsLib_DataTypeDB_BaseCheck(DestObject->GD, DestObject->EdsId,
                SrcObject->EdsId) == EDSLIB_SUCCESS)
        {
            /*
             * Source object is a derivative of the destination object.
             * In this case sufficient size should be allocated in the destination for
             * the largest object.
             */
            result = EDSLIB_BINDING_COMPATIBILITY_BASETYPE;
        }
    }

    return result;
}

void *EdsLib_Binding_GetNativeObject(const EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    uint8_t *DataPtr;

    if (!EdsLib_Binding_IsDescBufferValid(ObjectUserData))
    {
        DataPtr = NULL;
    }
    else
    {
        DataPtr = ObjectUserData->BufferPtr->Data;
    }

    if (DataPtr != NULL)
    {
        DataPtr += ObjectUserData->Offset;
    }

    return DataPtr;
}

size_t EdsLib_Binding_GetBufferMaxSize(const EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    size_t Result;

    if (!EdsLib_Binding_IsDescBufferValid(ObjectUserData))
    {
        Result = 0;
    }
    else
    {
        Result = ObjectUserData->BufferPtr->MaxContentSize - ObjectUserData->Offset;
    }

    return Result;
}

size_t EdsLib_Binding_GetNativeSize(const EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    size_t Result = EdsLib_Binding_GetBufferMaxSize(ObjectUserData);

    if (Result > ObjectUserData->Length)
    {
        Result = ObjectUserData->Length;
    }

    return Result;
}

void EdsLib_Binding_InitStaticFields(EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    void *ObjData = EdsLib_Binding_GetNativeObject(ObjectUserData);

    if (ObjData != NULL)
    {
        EdsLib_DataTypeDB_InitializeNativeObject(ObjectUserData->GD,
                ObjectUserData->EdsId, ObjData);
    }
}

int32_t EdsLib_Binding_InitFromPackedBuffer(EdsLib_Binding_DescriptorObject_t *ObjectUserData, const void *PackedData, size_t PackedDataSize)
{
    int32_t Status = EdsLib_DataTypeDB_UnpackCompleteObject(
            ObjectUserData->GD,
            &ObjectUserData->EdsId,
            EdsLib_Binding_GetNativeObject(ObjectUserData),
            PackedData,
            EdsLib_Binding_GetNativeSize(ObjectUserData),
            8 * PackedDataSize);

    EdsLib_DataTypeDB_GetTypeInfo(ObjectUserData->GD, ObjectUserData->EdsId,
            &ObjectUserData->TypeInfo);

    return Status;
}

int32_t EdsLib_Binding_ExportToPackedBuffer(EdsLib_Binding_DescriptorObject_t *ObjectUserData, void *PackedData, size_t PackedDataSize)
{
    int32_t Status = EdsLib_DataTypeDB_PackCompleteObject(
            ObjectUserData->GD,
            &ObjectUserData->EdsId,
            PackedData,
            EdsLib_Binding_GetNativeObject(ObjectUserData),
            8 * PackedDataSize,
            EdsLib_Binding_GetNativeSize(ObjectUserData));

    EdsLib_DataTypeDB_GetTypeInfo(ObjectUserData->GD, ObjectUserData->EdsId,
            &ObjectUserData->TypeInfo);

    return Status;
}


int32_t EdsLib_Binding_LoadValue(const EdsLib_Binding_DescriptorObject_t *ObjectUserData, EdsLib_GenericValueBuffer_t *ValBuf)
{
    return EdsLib_DataTypeDB_LoadValue(ObjectUserData->GD, ObjectUserData->EdsId,
            ValBuf, EdsLib_Binding_GetNativeObject(ObjectUserData));
}

int32_t EdsLib_Binding_StoreValue(const EdsLib_Binding_DescriptorObject_t *ObjectUserData, EdsLib_GenericValueBuffer_t *ValBuf)
{
    return EdsLib_DataTypeDB_StoreValue(ObjectUserData->GD, ObjectUserData->EdsId,
            EdsLib_Binding_GetNativeObject(ObjectUserData), ValBuf);
}

