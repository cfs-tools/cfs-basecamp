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
 * Auto-Generated stub implementations for functions defined in edslib_binding_objects header
 */

#include "edslib_binding_objects.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_AllocManagedBuffer()
 * ----------------------------------------------------
 */
EdsLib_Binding_Buffer_Content_t *EdsLib_Binding_AllocManagedBuffer(size_t MaxContentSize)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_AllocManagedBuffer, EdsLib_Binding_Buffer_Content_t *);

    UT_GenStub_AddParam(EdsLib_Binding_AllocManagedBuffer, size_t, MaxContentSize);

    UT_GenStub_Execute(EdsLib_Binding_AllocManagedBuffer, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_AllocManagedBuffer, EdsLib_Binding_Buffer_Content_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_CheckEdsObjectsCompatible()
 * ----------------------------------------------------
 */
EdsLib_Binding_Compatibility_t EdsLib_Binding_CheckEdsObjectsCompatible(EdsLib_Binding_DescriptorObject_t *DestObject,
                                                                        EdsLib_Binding_DescriptorObject_t *SrcObject)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_CheckEdsObjectsCompatible, EdsLib_Binding_Compatibility_t);

    UT_GenStub_AddParam(EdsLib_Binding_CheckEdsObjectsCompatible, EdsLib_Binding_DescriptorObject_t *, DestObject);
    UT_GenStub_AddParam(EdsLib_Binding_CheckEdsObjectsCompatible, EdsLib_Binding_DescriptorObject_t *, SrcObject);

    UT_GenStub_Execute(EdsLib_Binding_CheckEdsObjectsCompatible, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_CheckEdsObjectsCompatible, EdsLib_Binding_Compatibility_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_ExportToPackedBuffer()
 * ----------------------------------------------------
 */
int32_t EdsLib_Binding_ExportToPackedBuffer(EdsLib_Binding_DescriptorObject_t *ObjectUserData, void *PackedData,
                                            size_t PackedDataSize)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_ExportToPackedBuffer, int32_t);

    UT_GenStub_AddParam(EdsLib_Binding_ExportToPackedBuffer, EdsLib_Binding_DescriptorObject_t *, ObjectUserData);
    UT_GenStub_AddParam(EdsLib_Binding_ExportToPackedBuffer, void *, PackedData);
    UT_GenStub_AddParam(EdsLib_Binding_ExportToPackedBuffer, size_t, PackedDataSize);

    UT_GenStub_Execute(EdsLib_Binding_ExportToPackedBuffer, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_ExportToPackedBuffer, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_GetBufferMaxSize()
 * ----------------------------------------------------
 */
size_t EdsLib_Binding_GetBufferMaxSize(const EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_GetBufferMaxSize, size_t);

    UT_GenStub_AddParam(EdsLib_Binding_GetBufferMaxSize, const EdsLib_Binding_DescriptorObject_t *, ObjectUserData);

    UT_GenStub_Execute(EdsLib_Binding_GetBufferMaxSize, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_GetBufferMaxSize, size_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_GetNativeObject()
 * ----------------------------------------------------
 */
void *EdsLib_Binding_GetNativeObject(const EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_GetNativeObject, void *);

    UT_GenStub_AddParam(EdsLib_Binding_GetNativeObject, const EdsLib_Binding_DescriptorObject_t *, ObjectUserData);

    UT_GenStub_Execute(EdsLib_Binding_GetNativeObject, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_GetNativeObject, void *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_GetNativeSize()
 * ----------------------------------------------------
 */
size_t EdsLib_Binding_GetNativeSize(const EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_GetNativeSize, size_t);

    UT_GenStub_AddParam(EdsLib_Binding_GetNativeSize, const EdsLib_Binding_DescriptorObject_t *, ObjectUserData);

    UT_GenStub_Execute(EdsLib_Binding_GetNativeSize, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_GetNativeSize, size_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_InitDescriptor()
 * ----------------------------------------------------
 */
void EdsLib_Binding_InitDescriptor(EdsLib_Binding_DescriptorObject_t *ObjectUserData,
                                   const EdsLib_DatabaseObject_t *EdsDB, EdsLib_Id_t EdsId)
{
    UT_GenStub_AddParam(EdsLib_Binding_InitDescriptor, EdsLib_Binding_DescriptorObject_t *, ObjectUserData);
    UT_GenStub_AddParam(EdsLib_Binding_InitDescriptor, const EdsLib_DatabaseObject_t *, EdsDB);
    UT_GenStub_AddParam(EdsLib_Binding_InitDescriptor, EdsLib_Id_t, EdsId);

    UT_GenStub_Execute(EdsLib_Binding_InitDescriptor, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_InitFromPackedBuffer()
 * ----------------------------------------------------
 */
int32_t EdsLib_Binding_InitFromPackedBuffer(EdsLib_Binding_DescriptorObject_t *ObjectUserData, const void *PackedData,
                                            size_t PackedDataSize)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_InitFromPackedBuffer, int32_t);

    UT_GenStub_AddParam(EdsLib_Binding_InitFromPackedBuffer, EdsLib_Binding_DescriptorObject_t *, ObjectUserData);
    UT_GenStub_AddParam(EdsLib_Binding_InitFromPackedBuffer, const void *, PackedData);
    UT_GenStub_AddParam(EdsLib_Binding_InitFromPackedBuffer, size_t, PackedDataSize);

    UT_GenStub_Execute(EdsLib_Binding_InitFromPackedBuffer, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_InitFromPackedBuffer, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_InitStaticFields()
 * ----------------------------------------------------
 */
void EdsLib_Binding_InitStaticFields(EdsLib_Binding_DescriptorObject_t *ObjectUserData)
{
    UT_GenStub_AddParam(EdsLib_Binding_InitStaticFields, EdsLib_Binding_DescriptorObject_t *, ObjectUserData);

    UT_GenStub_Execute(EdsLib_Binding_InitStaticFields, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_InitSubObject()
 * ----------------------------------------------------
 */
void EdsLib_Binding_InitSubObject(EdsLib_Binding_DescriptorObject_t *      SubObject,
                                  const EdsLib_Binding_DescriptorObject_t *ParentObj,
                                  const EdsLib_DataTypeDB_EntityInfo_t *   Component)
{
    UT_GenStub_AddParam(EdsLib_Binding_InitSubObject, EdsLib_Binding_DescriptorObject_t *, SubObject);
    UT_GenStub_AddParam(EdsLib_Binding_InitSubObject, const EdsLib_Binding_DescriptorObject_t *, ParentObj);
    UT_GenStub_AddParam(EdsLib_Binding_InitSubObject, const EdsLib_DataTypeDB_EntityInfo_t *, Component);

    UT_GenStub_Execute(EdsLib_Binding_InitSubObject, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_InitUnmanagedBuffer()
 * ----------------------------------------------------
 */
EdsLib_Binding_Buffer_Content_t *EdsLib_Binding_InitUnmanagedBuffer(EdsLib_Binding_Buffer_Content_t *ContentPtr,
                                                                    void *DataPtr, size_t MaxContentSize)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_InitUnmanagedBuffer, EdsLib_Binding_Buffer_Content_t *);

    UT_GenStub_AddParam(EdsLib_Binding_InitUnmanagedBuffer, EdsLib_Binding_Buffer_Content_t *, ContentPtr);
    UT_GenStub_AddParam(EdsLib_Binding_InitUnmanagedBuffer, void *, DataPtr);
    UT_GenStub_AddParam(EdsLib_Binding_InitUnmanagedBuffer, size_t, MaxContentSize);

    UT_GenStub_Execute(EdsLib_Binding_InitUnmanagedBuffer, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_InitUnmanagedBuffer, EdsLib_Binding_Buffer_Content_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_Initialize()
 * ----------------------------------------------------
 */
void EdsLib_Binding_Initialize(void)
{

    UT_GenStub_Execute(EdsLib_Binding_Initialize, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_LoadValue()
 * ----------------------------------------------------
 */
int32_t EdsLib_Binding_LoadValue(const EdsLib_Binding_DescriptorObject_t *ObjectUserData,
                                 EdsLib_GenericValueBuffer_t *            ValBuf)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_LoadValue, int32_t);

    UT_GenStub_AddParam(EdsLib_Binding_LoadValue, const EdsLib_Binding_DescriptorObject_t *, ObjectUserData);
    UT_GenStub_AddParam(EdsLib_Binding_LoadValue, EdsLib_GenericValueBuffer_t *, ValBuf);

    UT_GenStub_Execute(EdsLib_Binding_LoadValue, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_LoadValue, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_SetDescBuffer()
 * ----------------------------------------------------
 */
void EdsLib_Binding_SetDescBuffer(EdsLib_Binding_DescriptorObject_t *DescrObj,
                                  EdsLib_Binding_Buffer_Content_t *  TargetBuffer)
{
    UT_GenStub_AddParam(EdsLib_Binding_SetDescBuffer, EdsLib_Binding_DescriptorObject_t *, DescrObj);
    UT_GenStub_AddParam(EdsLib_Binding_SetDescBuffer, EdsLib_Binding_Buffer_Content_t *, TargetBuffer);

    UT_GenStub_Execute(EdsLib_Binding_SetDescBuffer, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Binding_StoreValue()
 * ----------------------------------------------------
 */
int32_t EdsLib_Binding_StoreValue(const EdsLib_Binding_DescriptorObject_t *ObjectUserData,
                                  EdsLib_GenericValueBuffer_t *            ValBuf)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Binding_StoreValue, int32_t);

    UT_GenStub_AddParam(EdsLib_Binding_StoreValue, const EdsLib_Binding_DescriptorObject_t *, ObjectUserData);
    UT_GenStub_AddParam(EdsLib_Binding_StoreValue, EdsLib_GenericValueBuffer_t *, ValBuf);

    UT_GenStub_Execute(EdsLib_Binding_StoreValue, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Binding_StoreValue, int32_t);
}
