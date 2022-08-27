/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 * LEW-20211-1, Python Bindings for the Core Flight Executive Mission Library
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
 * \file     edslib_python_internal.h
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**      Internal Header file for Python / EDS bindings
 */

#ifndef _EDSLIB_PYTHON_INTERNAL_H_
#define _EDSLIB_PYTHON_INTERNAL_H_


#include "edslib_python.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include "edslib_binding_objects.h"

#define EDSLIB_PYTHON_FORMATCODE_LEN    12

typedef struct
{
    PyObject_HEAD
    void *dl;
    const EdsLib_DatabaseObject_t *GD;
    PyObject *DbName;
    /* TypeCache contains weak references to entries in the db, such that they
     * will not be re-created each time they are required.  This also gives persistence,
     * i.e. repeated calls to lookup the same type give the same object, instead of a
     * separate-but-equal object. */
    PyObject *TypeCache;
    PyObject *WeakRefList;
} EdsLib_Python_Database_t;

typedef struct
{
    PyHeapTypeObject type_base;
    EdsLib_Python_Database_t *EdsDb;
    PyObject *BaseName;
    PyObject *EdsTypeName;
    EdsLib_Id_t EdsId;
    char FormatInfo[EDSLIB_PYTHON_FORMATCODE_LEN];
    PyObject* SubEntityList;
} EdsLib_Python_DatabaseEntry_t;

typedef struct
{
    PyObject_HEAD
    Py_ssize_t Position;
    PyObject* refobj;
} EdsLib_Python_ContainerIterator_t;

typedef struct
{
    PyObject_HEAD
    uint16_t Index;
    PyObject* refobj;
} EdsLib_Python_EnumerationIterator_t;

typedef struct
{
    PyObject_HEAD
    EdsLib_Id_t EdsId;
    Py_ssize_t Offset;
    Py_ssize_t TotalLength;
} EdsLib_Python_Accessor_t;

typedef struct
{
    PyObject *pyobj;
    Py_buffer view;
} EdsLib_Python_View_t;

typedef struct
{
    PyObject_HEAD
    EdsLib_Binding_Buffer_Content_t edsbuf;
    uint8_t is_readonly;
    uint8_t is_dynamic;
    uint8_t is_initialized;
    EdsLib_Python_View_t *bufobj;
} EdsLib_Python_Buffer_t;

typedef struct
{
    PyObject_HEAD
    Py_ssize_t Offset;
    Py_ssize_t TotalLength;
    EdsLib_Python_Buffer_t *StorageBuf;
} EdsLib_Python_ObjectBase_t;

typedef struct
{
    EdsLib_Python_ObjectBase_t objbase;
    EdsLib_Python_DatabaseEntry_t *RefDbEntry;
    Py_ssize_t ElementSize;
    Py_ssize_t ElementCount;
} EdsLib_Python_ObjectArray_t;

extern PyObject *EdsLib_Python_DatabaseCache;

bool            EdsLib_Python_ConvertPythonToEdsScalar(EdsLib_Binding_DescriptorObject_t *edsobj, PyObject *pyobj);
bool            EdsLib_Python_ConvertPythonToEdsObject(EdsLib_Python_ObjectBase_t *self, PyObject *pyobj);
PyObject *      EdsLib_Python_ConvertEdsScalarToPython(EdsLib_Binding_DescriptorObject_t *edsobj);
PyObject *      EdsLib_Python_ConvertEdsObjectToPython(EdsLib_Python_ObjectBase_t *self);

PyTypeObject* EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl(EdsLib_Python_Database_t *EdsDb, EdsLib_Id_t EdsId);
PyObject *EdsLib_Python_ElementAccessor_CreateFromOffsetSize(EdsLib_Id_t EdsId, Py_ssize_t Offset, Py_ssize_t Length);
PyObject *EdsLib_Python_ElementAccessor_CreateFromEntityInfo(const EdsLib_DataTypeDB_EntityInfo_t *EntityInfo);

PyObject *EdsLib_Python_ObjectBase_InitArgsToKwds(PyObject *args, PyObject *kwds, const char **kwlist);
PyObject *EdsLib_Python_ObjectBase_BuildKwArgs(const char *format, const char **kwlist, ...);
bool EdsLib_Python_ObjectBase_SetKwArg(PyObject *kwds, const char *format, const char *kw, ...);
bool EdsLib_Python_ObjectBase_GetKwArg(PyObject *kwds, const char *format, const char *kw, void *OutPtr);

PyObject *EdsLib_Python_ObjectBase_GenericNew(PyTypeObject *objtype, PyObject *kwargs);
PyObject *EdsLib_Python_ObjectBase_NewSubObject(PyObject *obj, PyTypeObject *subobjtype, Py_ssize_t Offset, Py_ssize_t MaxSize);
int EdsLib_Python_ObjectBase_InitBufferView(EdsLib_Python_ObjectBase_t *self, Py_buffer *view, int flags);

int EdsLib_Python_SetupObjectDesciptor(EdsLib_Python_ObjectBase_t *self, EdsLib_Binding_DescriptorObject_t *descobj, int flags);
void EdsLib_Python_ReleaseObjectDesciptor(EdsLib_Binding_DescriptorObject_t *descobj);

/* various methods to construct EDS python object buffers */
EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_New(Py_ssize_t len);
EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_Copy(const void *buf, Py_ssize_t len);
EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_FromObject(PyObject *bufobj, int readonly);
EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_FromPtrAndSize(void *buf, Py_ssize_t len);
EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_FromConstPtrAndSize(const void *buf, Py_ssize_t len);
const void *EdsLib_Python_Buffer_Peek(EdsLib_Python_Buffer_t* buf);
Py_ssize_t EdsLib_Python_Buffer_GetMaxSize(EdsLib_Python_Buffer_t* buf);
bool EdsLib_Python_Buffer_IsInitialized(EdsLib_Python_Buffer_t* buf);
void EdsLib_Python_Buffer_SetInitialized(EdsLib_Python_Buffer_t* buf);

EdsLib_Binding_Buffer_Content_t* EdsLib_Python_Buffer_GetContentRef(EdsLib_Python_Buffer_t *self, int userflags);
void EdsLib_Python_Buffer_ReleaseContentRef(EdsLib_Binding_Buffer_Content_t* ref);

/*
 * EDS-defined data types are split into three categories for Python purposes-
 *  scalars, arrays, containers.
 *
 * These are all represented by the same C data type, "EdsLib_Python_Object_t",
 * but they have different capabilities in the Python environment, specifically
 * which protocols they implement.
 *  - The "Base" is the foundation of all EDS objects and defines common methods
 *  - Number and String are scalars i.e. the simple values with no sub-objects
 *  - Base objects (everything) implement the buffer protocol to share data with others
 *  - Arrays implement the sequence protocol
 *  - Containers implement the mapping protocol
 */
extern PyTypeObject EdsLib_Python_DatabaseType;
extern PyTypeObject EdsLib_Python_DatabaseEntryType;
extern PyTypeObject EdsLib_Python_BufferType;
extern PyTypeObject EdsLib_Python_AccessorType;
extern PyTypeObject EdsLib_Python_PackedObjectType;
extern PyTypeObject EdsLib_Python_ContainerIteratorType;
extern PyTypeObject EdsLib_Python_EnumEntryIteratorType;
extern PyTypeObject EdsLib_Python_ContainerEntryIteratorType;

extern PyTypeObject EdsLib_Python_ObjectBaseType;
extern PyTypeObject EdsLib_Python_ObjectNumberType;
extern PyTypeObject EdsLib_Python_ObjectScalarType;
extern PyTypeObject EdsLib_Python_ObjectContainerType;
extern PyTypeObject EdsLib_Python_ObjectArrayType;
extern PyTypeObject EdsLib_Python_DynamicArrayType;


#endif  /* _EDSLIB_PYTHON_INTERNAL_H_ */

