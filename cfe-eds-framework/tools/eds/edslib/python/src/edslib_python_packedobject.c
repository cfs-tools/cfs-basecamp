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
 * \file     edslib_python_packedobject.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement "packed object" type
**
**   This is just a light wrapper around the standard Python Bytes type.  Its
**   main purpose is to tell the difference between native bytes objects and
**   bytes objects which are in the packed form, so they can be converted accordingly.
 */

#include "edslib_python_internal.h"

/*
 * A shared scratch buffer for packing objects...
 *
 * Using a shared buffer here should be safe since Python already
 * serializes operations using the GIL, which is usually always held
 * EXCEPT if function intentionally gives it up i.e. to do blocking I/O.
 */

typedef struct
{
    void *ScratchPtr;
    Py_ssize_t ScratchSize;
    Py_ssize_t ContentSize;
} EdsLib_Python_ObjectScratchArea_t;


static EdsLib_Python_ObjectScratchArea_t EdsLib_Python_PackedObject_GLOBAL = { NULL, 0, 0 };

static PyObject *EdsLib_Python_PackedObjectType_new(PyTypeObject *objtype, PyObject *args, PyObject *kwds);
static PyObject *EdsLib_Python_PackedObjectType_repr(PyObject *obj);

PyTypeObject EdsLib_Python_PackedObjectType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("PackedObject"),
    .tp_basicsize = sizeof(PyBytesObject),
    .tp_itemsize = 1,
    .tp_base = &PyBytes_Type,
    .tp_new = EdsLib_Python_PackedObjectType_new,
    .tp_repr = EdsLib_Python_PackedObjectType_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("EDS Packed Object Bytes Type")
};

static int EdsLib_Python_InitScratchAreaFromEdsObject(EdsLib_Python_ObjectBase_t *srcobj,
        EdsLib_Python_ObjectScratchArea_t *scratch)
{
    EdsLib_Binding_DescriptorObject_t viewdesc;
    EdsLib_Python_DatabaseEntry_t *dbent;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    size_t actualsz;
    int32_t status;

    if (srcobj->StorageBuf == NULL)
    {
        PyErr_SetString(PyExc_BufferError, "No buffer object");
        return 0;
    }

    scratch->ContentSize = 0;
    dbent = (EdsLib_Python_DatabaseEntry_t *)Py_TYPE(srcobj);
    EdsLib_DataTypeDB_GetDerivedInfo(dbent->EdsDb->GD, dbent->EdsId, &DerivInfo);
    actualsz = (DerivInfo.MaxSize.Bits + 7) / 8;

    /*
     * ensure the scratch buffer is appropriately sized.
     * Note this size calculation includes at least 1 extra byte,
     * and rounds up to the next higher 64 byte boundary. This
     * should reduce the need for future resizes, especially for
     * small objects.
     */
    actualsz = (actualsz + 64) & ~((size_t)63);

    if (scratch->ScratchSize < actualsz)
    {
        if (scratch->ScratchPtr != NULL)
        {
            PyMem_Free(scratch->ScratchPtr);
            scratch->ScratchSize = 0;
        }

        scratch->ScratchPtr = PyMem_Malloc(actualsz);
        if (scratch->ScratchPtr != NULL)
        {
            scratch->ScratchSize = actualsz;
        }
    }

    if (scratch->ScratchSize < actualsz)
    {
        PyErr_NoMemory();
        return 0;
    }

    if (!EdsLib_Python_SetupObjectDesciptor(srcobj, &viewdesc, PyBUF_SIMPLE))
    {
        PyErr_Format(PyExc_RuntimeError, "EdsLib_Python_SetupObjectDesciptor() failed");
        return 0;
    }

    status = EdsLib_Binding_ExportToPackedBuffer(&viewdesc,
            scratch->ScratchPtr, actualsz-1);

    EdsLib_Python_ReleaseObjectDesciptor(&viewdesc);

    if (status != EDSLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "EdsLib error %d when packing buffer", (int)status);
        return 0;
    }

    /* now that the object is fully packed, the descriptor should reflect
     * the true size of the final packed bitstream.  Note that the allocator
     * for bytes implicitly adds 1 byte for a trailing null, so it is NOT
     * included here. */
    scratch->ContentSize = (viewdesc.TypeInfo.Size.Bits + 7) / 8;
    return 1;
}

static PyObject *EdsLib_Python_PackedObjectType_new(PyTypeObject *objtype, PyObject *args, PyObject *kwds)
{
    PyObject *argobj;
    PyBytesObject *result;
    void *contentdata_src;
    size_t contentdata_size;

    contentdata_src = NULL;
    contentdata_size = 0;
    result = NULL;
    if (!PyArg_ParseTuple(args, "O:PackedObjectType_new", &argobj))
    {
        return NULL;
    }

    /*
     * Construction method varies depending on the type of object.
     * Ultimately it needs to be based on an actual EDS DB entry.
     * Most objects already are, except for dynamic arrays, in which
     * case the reference to the actual DB Entry should be within
     * the object.
     */
    if (PyObject_TypeCheck(Py_TYPE(argobj), &EdsLib_Python_DatabaseEntryType))
    {
        if (EdsLib_Python_InitScratchAreaFromEdsObject(
                (EdsLib_Python_ObjectBase_t *)argobj,
                        &EdsLib_Python_PackedObject_GLOBAL))
        {
            contentdata_src = EdsLib_Python_PackedObject_GLOBAL.ScratchPtr;
            contentdata_size = EdsLib_Python_PackedObject_GLOBAL.ContentSize;
        }
    }
    else if (PyObject_TypeCheck(argobj, &PyBytes_Type))
    {
        contentdata_src = PyBytes_AS_STRING(argobj);
        contentdata_size = PyBytes_GET_SIZE(argobj);
    }
    else
    {
        /* Dynamic arrays are not yet implemented */
        PyErr_Format(PyExc_TypeError, "%s cannot be encoded", Py_TYPE(argobj)->tp_name);
    }

    if (contentdata_src != NULL)
    {
        result = (PyBytesObject*)objtype->tp_alloc(objtype, contentdata_size);
        if (result != NULL)
        {
            char *contentdata_dst = PyBytes_AS_STRING(result);
            Py_MEMCPY(contentdata_dst,
                    contentdata_src,
                    contentdata_size);
            contentdata_dst[contentdata_size] = 0;
            //bc result->ob_shash = -1;
        }
    }

    return (PyObject*)result;
}

static PyObject *EdsLib_Python_PackedObjectType_repr(PyObject *obj)
{
    PyObject *repr = EdsLib_Python_PackedObjectType.tp_base->tp_repr(obj);
    PyObject *result = NULL;

    if (repr != NULL)
    {
        result = PyUnicode_FromFormat("%s(%U)", obj->ob_type->tp_name, repr);
        Py_DECREF(repr);
    }

    return result;
}

