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
 * \file     edslib_binding_objects.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 */

#ifndef _EDSLIB_BINDING_OBJECTS_H_
#define _EDSLIB_BINDING_OBJECTS_H_


#include <edslib_datatypedb.h>
#include <edslib_displaydb.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************
 * TYPE DEFINITIONS
 *****************************************************************************/

typedef enum
{
    EDSLIB_BINDING_COMPATIBILITY_NONE = 0,
    EDSLIB_BINDING_COMPATIBILITY_BASETYPE,
    EDSLIB_BINDING_COMPATIBILITY_EXACT
} EdsLib_Binding_Compatibility_t;

/**
 * A generic abstract buffer for holding any EDS object.
 *
 * This is a reference-counting buffer, so that many EDS descriptor objects can point
 * to the same instance.
 */
typedef struct
{
    /*
     * Note - the "Data" field is variably sized, and the MaxContentSize
     * indicates the actual number of bytes available here.
     */
    uint8_t     *Data;
    bool        IsManaged;
    size_t      MaxContentSize;    /**< The actual size of the "Data" field */
    uintmax_t   ReferenceCount;    /**< The number of references to this buffer */

} EdsLib_Binding_Buffer_Content_t;


/**
 * A runtime descriptor of an EDS object
 *
 * This object describes a piece of EDS data.  It may or may not also contain an actual reference to the data
 * being described, depending on context.
 */
struct EdsLib_Binding_DescriptorObject
{
    const EdsLib_DatabaseObject_t *GD;          /**< Handle to original database object which defines all EDS types */
    EdsLib_Id_t EdsId;          /**< The EDS ID value representing the specific type of the data */
    uint32_t Offset;            /**< Absolute Offset of member within the top-level parent structure */
    uint32_t Length;            /**< Total allocated space for this object within parent structure */
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;      /**< Information about the component type (size, number of subcomponents, etc) */
    EdsLib_Binding_Buffer_Content_t *BufferPtr; /**< Pointer to a buffer holding the data (optional, may be NULL) */
};

typedef struct EdsLib_Binding_DescriptorObject EdsLib_Binding_DescriptorObject_t;

/*****************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/

/**
 * EDS Binding API Initialization function.
 *
 * This should be called once during startup by applications
 * that wish to use the EDS Binding Object API before using
 * other API calls.
 */
void EdsLib_Binding_Initialize(void);

EdsLib_Binding_Buffer_Content_t *EdsLib_Binding_AllocManagedBuffer(size_t MaxContentSize);
EdsLib_Binding_Buffer_Content_t *EdsLib_Binding_InitUnmanagedBuffer(EdsLib_Binding_Buffer_Content_t *ContentPtr, void *DataPtr, size_t MaxContentSize);

void EdsLib_Binding_SetDescBuffer(EdsLib_Binding_DescriptorObject_t *DescrObj, EdsLib_Binding_Buffer_Content_t *TargetBuffer);

static inline bool EdsLib_Binding_IsDescBufferValid(const EdsLib_Binding_DescriptorObject_t *DescrObj)
{
    return (DescrObj->BufferPtr != NULL &&
            EdsLib_Is_Valid(DescrObj->EdsId) &&
            DescrObj->BufferPtr->MaxContentSize >= (DescrObj->Offset + DescrObj->TypeInfo.Size.Bytes));
}



void EdsLib_Binding_InitDescriptor(
        EdsLib_Binding_DescriptorObject_t *ObjectUserData,
        const EdsLib_DatabaseObject_t *EdsDB,
        EdsLib_Id_t EdsId);

void EdsLib_Binding_InitSubObject(
        EdsLib_Binding_DescriptorObject_t *SubObject,
        const EdsLib_Binding_DescriptorObject_t *ParentObj,
        const EdsLib_DataTypeDB_EntityInfo_t *Component);

void *EdsLib_Binding_GetNativeObject(const EdsLib_Binding_DescriptorObject_t *ObjectUserData);
size_t EdsLib_Binding_GetNativeSize(const EdsLib_Binding_DescriptorObject_t *ObjectUserData);
size_t EdsLib_Binding_GetBufferMaxSize(const EdsLib_Binding_DescriptorObject_t *ObjectUserData);

EdsLib_Binding_Compatibility_t EdsLib_Binding_CheckEdsObjectsCompatible(
        EdsLib_Binding_DescriptorObject_t *DestObject,
        EdsLib_Binding_DescriptorObject_t *SrcObject);

void EdsLib_Binding_InitStaticFields(EdsLib_Binding_DescriptorObject_t *ObjectUserData);

int32_t EdsLib_Binding_InitFromPackedBuffer(
        EdsLib_Binding_DescriptorObject_t *ObjectUserData,
        const void *PackedData,
        size_t PackedDataSize);

int32_t EdsLib_Binding_ExportToPackedBuffer(
        EdsLib_Binding_DescriptorObject_t *ObjectUserData,
        void *PackedData,
        size_t PackedDataSize);

int32_t EdsLib_Binding_LoadValue(const EdsLib_Binding_DescriptorObject_t *ObjectUserData, EdsLib_GenericValueBuffer_t *ValBuf);
int32_t EdsLib_Binding_StoreValue(const EdsLib_Binding_DescriptorObject_t *ObjectUserData, EdsLib_GenericValueBuffer_t *ValBuf);


#ifdef __cplusplus
}   /* extern "C" */
#endif


#endif  /* _EDSLIB_BINDING_OBJECTS_H_ */

