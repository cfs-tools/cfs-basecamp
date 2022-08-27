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
 * \file     edslib_python_accessor.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   This is an object that combines an offset and buffer size along with a
**   pointer to the associated EDS database entry.  It implements the Python
**   "descriptor protocol".  Instances of this object are can be stored as
**   attributes in other objects to describe EDS object instances within
**   arbitrary memory buffers.
 */

#include "edslib_python_internal.h"

static PyObject *   EdsLib_Python_Accessor_descr_get(PyObject *obj, PyObject *refobj, PyObject *reftype);
static int          EdsLib_Python_Accessor_descr_set(PyObject *obj, PyObject *refobj, PyObject *val);
static PyObject *   EdsLib_Python_Accessor_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static void         EdsLib_Python_Accessor_dealloc(PyObject * obj);
static PyObject *   EdsLib_Python_Accessor_repr(PyObject *obj);

PyTypeObject EdsLib_Python_AccessorType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("ElementAccessor"),
    .tp_basicsize = sizeof(EdsLib_Python_Accessor_t),
    .tp_dealloc = EdsLib_Python_Accessor_dealloc,
    .tp_new = EdsLib_Python_Accessor_new,
    .tp_repr = EdsLib_Python_Accessor_repr,
    .tp_descr_get = EdsLib_Python_Accessor_descr_get,
    .tp_descr_set = EdsLib_Python_Accessor_descr_set,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("EDS Element Accessor Type")
};


PyObject *EdsLib_Python_ElementAccessor_CreateFromOffsetSize(EdsLib_Id_t EdsId, Py_ssize_t Offset, Py_ssize_t Length)
{
    EdsLib_Python_Accessor_t *self;

    self = PyObject_New(EdsLib_Python_Accessor_t, &EdsLib_Python_AccessorType);
    if (self == NULL)
    {
        return NULL;
    }
    self->EdsId = EdsId;
    self->Offset = Offset;
    self->TotalLength = Length;

    return (PyObject*)self;
}

PyObject *EdsLib_Python_ElementAccessor_CreateFromEntityInfo(const EdsLib_DataTypeDB_EntityInfo_t *EntityInfo)
{
    return EdsLib_Python_ElementAccessor_CreateFromOffsetSize(EntityInfo->EdsId,
            EntityInfo->Offset.Bytes, EntityInfo->MaxSize.Bytes);
}

static void EdsLib_Python_Accessor_dealloc(PyObject * obj)
{
    obj->ob_type->tp_free(obj);
}

static PyObject * EdsLib_Python_Accessor_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    EdsLib_Python_Accessor_t *self = NULL;
    Py_ssize_t Offset = 0;
    Py_ssize_t BufferSize = 0;
    unsigned long EdsId;

    if (!PyArg_ParseTuple(args, "kn|n:EdsLib_Python_Accessor_new",
            &EdsId, &BufferSize, &Offset))
    {
        return NULL;
    }

    self = (EdsLib_Python_Accessor_t *)obj->tp_alloc(obj, 0);
    if (self == NULL)
    {
        return NULL;
    }

    /*
     * By default, if not supplied, the assume the buffer size is
     * equal to the maximum possible size
     */
    self->EdsId = EdsId;
    self->Offset = Offset;
    self->TotalLength = BufferSize;

    return (PyObject*)self;
}

PyObject *   EdsLib_Python_Accessor_repr(PyObject *obj)
{
    EdsLib_Python_Accessor_t *self = (EdsLib_Python_Accessor_t *)obj;
    PyObject *result = NULL;

    result = PyUnicode_FromFormat("%s(%lu,%zd,%zd)",
            obj->ob_type->tp_name,
            (unsigned long)self->EdsId,
            self->TotalLength,
            self->Offset);

    return result;
}

static PyObject *EdsLib_Python_Accessor_descr_get(PyObject *obj, PyObject *refobj, PyObject *reftype)
{
    EdsLib_Python_Accessor_t *self = (EdsLib_Python_Accessor_t *)obj;
    EdsLib_Python_Database_t *edsdb = NULL;
    PyTypeObject *subtype;
    PyObject *result;

    result = NULL;

    /*
     * In some cases this can be called with a null refobj (i.e. if accessed from a type object directly)
     * In that case there is no value
     */
    if (refobj == NULL)
    {
        Py_RETURN_NONE;
    }

    /*
     * Get the EDS database defining the object.  Where this actually comes from depends
     * on the type of the refobj...
     *  - If the metatype refobj is a db entry, then use the internal reference from this
     *  - If it is a dynamic array, then lookup inside that struct.
     *
     * In either case it will be a borrowed ref (do not decrement)
     */

    if (Py_TYPE(refobj->ob_type) == &EdsLib_Python_DatabaseEntryType)
    {
        edsdb = ((EdsLib_Python_DatabaseEntry_t *)refobj->ob_type)->EdsDb;
    }
    else if (PyObject_IsInstance(refobj, (PyObject*)&EdsLib_Python_ObjectArrayType))
    {
        edsdb = ((EdsLib_Python_ObjectArray_t*)refobj)->RefDbEntry->EdsDb;
    }

    if (edsdb == NULL)
    {
        PyErr_Format(PyExc_TypeError, "Unable to extract DB from %s type", refobj->ob_type->tp_name);
        return NULL;
    }

    subtype = EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl(edsdb, self->EdsId);
    if (subtype == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_NewSubObject(refobj, subtype, self->Offset, self->TotalLength);

    Py_DECREF(subtype);

    return result;
}

static int EdsLib_Python_Accessor_descr_set(PyObject *obj, PyObject *refobj, PyObject *val)
{
    PyObject *dstobj = NULL;
    int result = -1;

    do
    {
        dstobj = EdsLib_Python_Accessor_descr_get(obj, refobj, NULL);
        if (dstobj == NULL)
        {
            break;
        }

        if (!PyObject_TypeCheck(dstobj, &EdsLib_Python_ObjectBaseType))
        {
            PyErr_Format(PyExc_TypeError, "%s not an EDS object instance", dstobj->ob_type->tp_name);
            break;
        }

        if (!EdsLib_Python_ConvertPythonToEdsObject((EdsLib_Python_ObjectBase_t*)dstobj, val))
        {
            break;
        }

        result = 0;
    }
    while(0);

    Py_XDECREF(dstobj);

    return result;
}

