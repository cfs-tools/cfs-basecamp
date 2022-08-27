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
 * \file     edslib_python_container.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement Python datatype for EDS container objects
**
**   This extends the base object with the mapping protocol and an
**   iterator that access the attributes
 */

#include "edslib_python_internal.h"


static PyObject *   EdsLib_Python_ObjectContainer_keys(PyObject *obj);
static PyObject *   EdsLib_Python_ObjectContainer_values(PyObject *obj);
static PyObject *   EdsLib_Python_ObjectContainer_items(PyObject *obj);
static PyObject *   EdsLib_Python_ObjectContainer_iter(PyObject *obj);
static Py_ssize_t   EdsLib_Python_ObjectContainer_map_len(PyObject *obj);

static void         EdsLib_Python_ContainerIterator_dealloc(PyObject * obj);
static int          EdsLib_Python_ContainerIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int          EdsLib_Python_ContainerIterator_clear(PyObject *obj);
static PyObject *   EdsLib_Python_ContainerIterator_iternext(PyObject *obj);

static PyMethodDef EdsLib_Python_Database_methods[] =
{
        {"keys",  (PyCFunction)EdsLib_Python_ObjectContainer_keys, METH_NOARGS, "Get a list of keys in container"},
        {"values",  (PyCFunction)EdsLib_Python_ObjectContainer_values, METH_NOARGS, "Get a list of values in container"},
        {"items",  (PyCFunction)EdsLib_Python_ObjectContainer_items, METH_NOARGS, "Get a list of key/value tuples in container"},
        {NULL}  /* Sentinel */
};


static PyMappingMethods EdsLib_Python_EdsContainer_MappingMethods =
{
        .mp_length = EdsLib_Python_ObjectContainer_map_len,
        .mp_subscript = PyObject_GenericGetAttr,
        .mp_ass_subscript = PyObject_GenericSetAttr
};

PyTypeObject EdsLib_Python_ObjectContainerType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("Container"),
    .tp_basicsize = sizeof(EdsLib_Python_ObjectBase_t),
    .tp_base = &EdsLib_Python_ObjectBaseType,
    .tp_as_mapping = &EdsLib_Python_EdsContainer_MappingMethods,
    .tp_methods = EdsLib_Python_Database_methods,
    .tp_iter = EdsLib_Python_ObjectContainer_iter,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("EDS ContainerDataType")
};

PyTypeObject EdsLib_Python_ContainerIteratorType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("ContainerIterator"),
    .tp_basicsize = sizeof(EdsLib_Python_ContainerIterator_t),
    .tp_dealloc = EdsLib_Python_ContainerIterator_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = EdsLib_Python_ContainerIterator_traverse,
    .tp_clear = EdsLib_Python_ContainerIterator_clear,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = EdsLib_Python_ContainerIterator_iternext,
    .tp_doc = PyDoc_STR("EDS ContainerIteratorType")
};

static void EdsLib_Python_ContainerIterator_dealloc(PyObject * obj)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int EdsLib_Python_ContainerIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    Py_VISIT(self->refobj);
    return 0;
}

static int EdsLib_Python_ContainerIterator_clear(PyObject *obj)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    Py_CLEAR(self->refobj);
    return 0;
}

static PyObject *EdsLib_Python_ContainerIterator_iternext(PyObject *obj)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    EdsLib_Python_DatabaseEntry_t *dbent = NULL;
    PyObject *key = NULL;
    PyObject *value = NULL;
    PyObject *result = NULL;

    do
    {
        if (self->refobj == NULL)
        {
            break;
        }

        dbent = (EdsLib_Python_DatabaseEntry_t *)self->refobj->ob_type;
        if (self->Position < PyList_GET_SIZE(dbent->SubEntityList))
        {
            key = PyList_GET_ITEM(dbent->SubEntityList, self->Position); /* borrowed ref */
        }

        if (key == NULL)
        {
            /* end */
            Py_CLEAR(self->refobj);
            break;
        }
        Py_INCREF(key);
        ++self->Position;

        value = PyObject_GetAttr(self->refobj, key);
        if (value == NULL)
        {
            break;
        }

        result = PyTuple_Pack(2, key, value);
    }
    while(0);

    Py_XDECREF(key);
    Py_XDECREF(value);

    return result;
}

static PyObject *EdsLib_Python_ObjectContainer_keys(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *dbent = (EdsLib_Python_DatabaseEntry_t *)obj->ob_type;
    Py_ssize_t idx;
    Py_ssize_t size;
    PyObject *key;
    PyObject *result;

    /* sanity check that the db entry has a valid subentry list (it should for all containers) */
    if (dbent->SubEntityList == NULL || !PyList_Check(dbent->SubEntityList))
    {
        return PyErr_Format(PyExc_TypeError, "%s is not an mappable EDS type", Py_TYPE(obj)->tp_name);
    }

    size = PyList_GET_SIZE(dbent->SubEntityList);
    result = PyList_New(size);
    if (result == NULL)
    {
        return NULL;
    }

    /* return a _copy_ of the key list, rather than a direct ref to the original.
     * This is to ensure that the original (within the type object) is not modified.
     * If the list were ever modified, bad things would happen. */
    for (idx = 0; idx < size; ++idx)
    {
        key = PyList_GET_ITEM(dbent->SubEntityList, idx);   /* borrowed ref */
        if (key == NULL)
        {
            key = Py_None;
        }
        Py_INCREF(key);                                     /* compensate for stealing */
        PyList_SET_ITEM(result, idx, key);                  /* steals ref */
    }

    return result;
}

static PyObject *EdsLib_Python_ObjectContainer_values(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *dbent = (EdsLib_Python_DatabaseEntry_t *)obj->ob_type;
    Py_ssize_t idx;
    Py_ssize_t size;
    PyObject *val;
    PyObject *key;
    PyObject *result;

    if (dbent->SubEntityList == NULL || !PyList_Check(dbent->SubEntityList))
    {
        return PyErr_Format(PyExc_TypeError, "%s is not an mappable EDS type", Py_TYPE(obj)->tp_name);
    }

    size = PyList_GET_SIZE(dbent->SubEntityList);
    result = PyList_New(size);
    if (result == NULL)
    {
        return NULL;
    }

    for (idx = 0; idx < size; ++idx)
    {
        key = PyList_GET_ITEM(dbent->SubEntityList, idx);   /* borrowed ref */
        if (key != NULL)
        {
            val = PyObject_GetAttr(obj, key);               /* new ref */
        }
        else
        {
            val = NULL;
        }
        if (val == NULL)
        {
            /* unlikely, but just in case */
            val = Py_None;
            Py_INCREF(val);
        }
        PyList_SET_ITEM(result, idx, val);                  /* steals ref */
    }

    return result;
}

static PyObject *EdsLib_Python_ObjectContainer_items(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *dbent = (EdsLib_Python_DatabaseEntry_t *)obj->ob_type;
    Py_ssize_t idx;
    Py_ssize_t size;
    PyObject *item;
    PyObject *val;
    PyObject *key;
    PyObject *result;

    if (dbent->SubEntityList == NULL || !PyList_Check(dbent->SubEntityList))
    {
        return PyErr_Format(PyExc_TypeError, "%s is not an mappable EDS type", Py_TYPE(obj)->tp_name);
    }

    size = PyList_GET_SIZE(dbent->SubEntityList);
    result = PyList_New(size);
    if (result == NULL)
    {
        return NULL;
    }

    for (idx = 0; idx < size; ++idx)
    {
        key = PyList_GET_ITEM(dbent->SubEntityList, idx);   /* borrowed ref */
        if (key != NULL)
        {
            val = PyObject_GetAttr(obj, key);               /* new ref */
        }
        else
        {
            key = Py_None;
            val = NULL;
        }
        if (val == NULL)
        {
            /* unlikely, but just in case */
            val = Py_None;
            Py_INCREF(val);
        }
        item = PyTuple_Pack(2, key, val);
        if (item != NULL)
        {
            PyList_SET_ITEM(result, idx, item);              /* steals ref to item */
        }
        Py_DECREF(val);                                      /* release val ref */
    }

    return result;
}

static Py_ssize_t   EdsLib_Python_ObjectContainer_map_len(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *dbent = (EdsLib_Python_DatabaseEntry_t *)obj->ob_type;
    lenfunc lenf = dbent->type_base.as_sequence.sq_length;

    if (lenf == NULL)
    {
        return 0;
    }

    return lenf((PyObject*)dbent);
}

static PyObject *   EdsLib_Python_ObjectContainer_iter(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *dbent = (EdsLib_Python_DatabaseEntry_t *)obj->ob_type;
    EdsLib_Python_ContainerIterator_t *result;

    /* sanity check that the db entry has a valid subentry list (it should for all containers) */
    if (dbent->SubEntityList == NULL || !PyList_Check(dbent->SubEntityList))
    {
        return PyErr_Format(PyExc_TypeError, "%s is not an mappable EDS type", Py_TYPE(obj)->tp_name);
    }

    result = PyObject_GC_New(EdsLib_Python_ContainerIterator_t, &EdsLib_Python_ContainerIteratorType);
    if (result == NULL)
    {
        return NULL;
    }

    result->Position = 0;
    Py_INCREF(obj);
    result->refobj = obj;
    PyObject_GC_Track(result);

    return (PyObject*)result;
}
