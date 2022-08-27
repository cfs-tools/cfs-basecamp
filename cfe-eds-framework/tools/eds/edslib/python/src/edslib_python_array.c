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
 * \file     edslib_python_array.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   This extends the base type with the Python Sequence protocol.  This forms
**   the base type of all EDS array objects.  For static EDS arrays, i.e.
**   those defined at compile time in an EDS file using the "ArrayDataType" EDS
**   element, this type will be further derived with the EDS definition in a
**   manner similar to scalars and containers.
**
**   The python bindings also allow for dynamic arrays, which are allocated at
**   run time from any other EDS object and allow any length to be created,
**   memory permitting.  These also derive (separately) from this base type.
 */

#include "edslib_python_internal.h"

static Py_ssize_t   EdsLib_Python_ObjectArray_seq_len(PyObject *obj);
static PyObject *   EdsLib_Python_ObjectArray_seq_item(PyObject *obj, Py_ssize_t idx);
static int          EdsLib_Python_ObjectArray_seq_ass_item(PyObject *obj, Py_ssize_t idx, PyObject *val);
static int          EdsLib_Python_ObjectArray_getbuffer(PyObject *obj, Py_buffer *view, int flags);
static void         EdsLib_Python_ObjectArray_releasebuffer(PyObject *obj, Py_buffer *view);
static int          EdsLib_Python_ObjectArray_init(PyObject *obj, PyObject *args, PyObject *kwds);


static PyBufferProcs EdsLib_Python_ObjectArray_BufferProcs =
{
        .bf_getbuffer = EdsLib_Python_ObjectArray_getbuffer,
        .bf_releasebuffer = EdsLib_Python_ObjectArray_releasebuffer
};


static PySequenceMethods EdsLib_Python_ObjectArray_SequenceMethods =
{
        .sq_length = EdsLib_Python_ObjectArray_seq_len,
        .sq_item = EdsLib_Python_ObjectArray_seq_item,
        .sq_ass_item = EdsLib_Python_ObjectArray_seq_ass_item
};

PyTypeObject EdsLib_Python_ObjectArrayType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("Array"),
    .tp_basicsize = sizeof(EdsLib_Python_ObjectArray_t),
    .tp_base = &EdsLib_Python_ObjectBaseType,
    .tp_as_buffer = &EdsLib_Python_ObjectArray_BufferProcs,
    .tp_as_sequence = &EdsLib_Python_ObjectArray_SequenceMethods,
    .tp_init = EdsLib_Python_ObjectArray_init,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("EDS Array Type")
};


static Py_ssize_t EdsLib_Python_ObjectArray_seq_len(PyObject *obj)
{
    EdsLib_Python_ObjectArray_t *self = (EdsLib_Python_ObjectArray_t *)obj;
    return self->ElementCount;
}

static PyObject *   EdsLib_Python_ObjectArray_GetIndexAccessor(PyObject *obj, Py_ssize_t idx)
{
    EdsLib_Python_ObjectArray_t *self = (EdsLib_Python_ObjectArray_t *)obj;
    PyObject *accessor;


    if (idx >= 0 && idx < self->ElementCount)
    {
        accessor = EdsLib_Python_ElementAccessor_CreateFromOffsetSize(self->RefDbEntry->EdsId,
                self->ElementSize * idx, self->ElementSize);
    }
    else
    {
        PyErr_Format(PyExc_IndexError, "Request for index %zd in array of length %zd",
                idx, self->ElementCount);
        accessor = NULL;
    }

    return accessor;
}

static PyObject *   EdsLib_Python_ObjectArray_seq_item(PyObject *obj, Py_ssize_t idx)
{
    PyObject *accessor = EdsLib_Python_ObjectArray_GetIndexAccessor(obj, idx);
    PyObject *result;
    descrgetfunc getfunc;

    if (accessor == NULL)
    {
        return NULL;
    }

    /*
     * Note this is basically equivalent to what Python does natively for
     * containers with descriptors.  Unfortunately Python didn't do the same
     * thing for arrays, so mimic it here manually.
     */
    getfunc = accessor->ob_type->tp_descr_get;
    if (getfunc == NULL)
    {
        Py_INCREF(accessor);
        result = accessor;
    }
    else
    {
        result = getfunc(accessor, obj, (PyObject *)obj->ob_type);
    }

    Py_DECREF(accessor);

    return result;
}

static int EdsLib_Python_ObjectArray_seq_ass_item(PyObject *obj, Py_ssize_t idx, PyObject *val)
{
    PyObject *accessor = EdsLib_Python_ObjectArray_GetIndexAccessor(obj, idx);
    int result;
    descrsetfunc setfunc;

    if (accessor == NULL)
    {
        return -1;
    }

    /*
     * Note this is basically equivalent to what Python does natively for
     * containers with descriptors.  Unfortunately Python didn't do the same
     * thing for arrays, so mimic it here manually.
     */
    setfunc = accessor->ob_type->tp_descr_set;
    if (setfunc == NULL)
    {
        return -1;
    }

    result = setfunc(accessor, obj, val);
    Py_DECREF(accessor);

    return result;
}

static int EdsLib_Python_ObjectArray_getbuffer(PyObject *obj, Py_buffer *view, int flags)
{
    EdsLib_Python_ObjectArray_t * self = (EdsLib_Python_ObjectArray_t *)obj;
    Py_ssize_t ArrayItemSize;

    ArrayItemSize = EdsLib_Python_DatabaseEntry_GetMaxSize((PyTypeObject*)self->RefDbEntry);
    if (view->itemsize <= 0)
    {
        return -1;
    }

    if (EdsLib_Python_ObjectBase_InitBufferView(&self->objbase, view, flags) != 0)
    {
        return -1;
    }

    /*
     * As this is actually an array, if STRIDES/SHAPE/FORMAT info was requested,
     * this is a good place to fixup these values in the view to make it more accurate
     *
     *  - Item size is based on the DB entry size (may be less than stride size in some cases)
     *  - Format string is based on the DB entry (should be correct for all basic types)
     *  - Stride size is based on the ElementSize in the dynamic array object
     *  - Shape, if requested, is equal to the total length
     */
    view->itemsize = ArrayItemSize;
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
    {
        view->format = self->RefDbEntry->FormatInfo;
    }
    if ((flags & PyBUF_ND) == PyBUF_ND)
    {
        view->ndim = 1;
        view->shape = &self->ElementCount;
    }
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
    {
        view->strides = &self->ElementSize;
    }

    return 0;
}

static void EdsLib_Python_ObjectArray_releasebuffer(PyObject *obj, Py_buffer *view)
{
    EdsLib_Python_Buffer_ReleaseContentRef(view->internal);
}

static int          EdsLib_Python_ObjectArray_init(PyObject *obj, PyObject *args, PyObject *kwds)
{
    static const char *in_kwlist[] = { "value", NULL };
    EdsLib_Python_ObjectArray_t * self = (EdsLib_Python_ObjectArray_t *)obj;
    EdsLib_Python_DatabaseEntry_t *dbent = NULL;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    Py_ssize_t maxsize = -1;
    Py_ssize_t nelem = -1;
    Py_ssize_t elemsz = -1;
    Py_ssize_t idx;
    int result = -1;

    /*
     * First drop any existing buffer (e.g. in case __init__ is invoked directly)
     */
    Py_CLEAR(self->RefDbEntry);

    kwds = EdsLib_Python_ObjectBase_InitArgsToKwds(args, kwds, in_kwlist);
    args = NULL;    /* should no longer be used directly */

    do
    {
        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|O:ObjectArray_init", "dbent", &dbent))
        {
            break;
        }

        if (dbent != NULL)
        {
            Py_INCREF(dbent); /* make owned.  It is DECREF'ed at the end of this function. */
            if (Py_TYPE(dbent) != &EdsLib_Python_DatabaseEntryType)
            {
                PyErr_Format(PyExc_TypeError, "dbent must be a database entry type, not %s",
                        Py_TYPE(dbent)->tp_name);
                break;
            }
        }

        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|n:ObjectArray_init", "nelem", &nelem))
        {
            break;
        }

        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|n:ObjectArray_init", "elemsz", &elemsz))
        {
            break;
        }

        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|n:ObjectArray_init", "maxsize", &maxsize))
        {
            break;
        }



        /*
         * If the object has a direct EDS definition (static array) then look up the info now.
         * This does not work for dynamic arrays or anything that doesn't directly map to an EDS definition.
         */
        if (Py_TYPE(Py_TYPE(self)) == &EdsLib_Python_DatabaseEntryType)
        {
            EdsLib_Python_DatabaseEntry_t *SelfDbEntry = (EdsLib_Python_DatabaseEntry_t *)Py_TYPE(self);

            if (EdsLib_DataTypeDB_GetTypeInfo(SelfDbEntry->EdsDb->GD, SelfDbEntry->EdsId, &TypeInfo) != EDSLIB_SUCCESS)
            {
                PyErr_Format(PyExc_RuntimeError, "Cannot get array type info from EDS DB");
                break;
            }

            if (dbent == NULL)
            {
                EdsLib_DataTypeDB_EntityInfo_t EntityInfo;

                if (EdsLib_DataTypeDB_GetMemberByIndex(SelfDbEntry->EdsDb->GD, SelfDbEntry->EdsId, 0, &EntityInfo) != EDSLIB_SUCCESS)
                {
                    PyErr_Format(PyExc_RuntimeError, "Cannot get array entity info from EDS DB");
                    break;
                }

                dbent = (EdsLib_Python_DatabaseEntry_t *)EdsLib_Python_DatabaseEntry_GetFromEdsId((PyObject*)SelfDbEntry->EdsDb, EntityInfo.EdsId);
            }

        }
        else
        {
            memset(&TypeInfo, 0, sizeof(TypeInfo));
        }

        /*
         * If the DB entry is left undefined, this is an error.
         * The caller needs to supply it via the "dbent" keyword
         */
        if (dbent == NULL)
        {
            PyErr_Format(PyExc_TypeError, "A dbent value is required when building %s objects", Py_TYPE(self)->tp_name);
            break;
        }

        /*
         * If element size was not specified, then extrapolate from:
         * a) if this is a static EDS array, then use the EDS type info
         * b) if max size and num elements is known, then divide evenly
         * c) the max size of the element as defined by EDS.
         *
         * Note that the latter potentially includes padding for the worst-case,
         * derivative so this is the fallback option.
         */
        if (elemsz <= 0)
        {
            if (TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY)
            {
                elemsz = TypeInfo.Size.Bytes / TypeInfo.NumSubElements;
            }
            else if (maxsize > 0 && nelem > 0)
            {
                elemsz = maxsize / nelem;
            }
            else if (EdsLib_DataTypeDB_GetDerivedInfo(dbent->EdsDb->GD,
                    dbent->EdsId, &DerivInfo) == EDSLIB_SUCCESS)
            {
                elemsz = DerivInfo.MaxSize.Bytes;
            }
        }

        /*
         * If number of elements was not specified, then extrapolate from:
         * a) if this is a static EDS array, then use the EDS type info
         * b) if max size and element size is known, then divide evenly
         */
        if (nelem <= 0)
        {
            if (TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY)
            {
                nelem = TypeInfo.NumSubElements;
            }
            else if (maxsize > 0 && elemsz > 0)
            {
                nelem = maxsize / elemsz;
            }
        }

        /*
         * At this point must have a valid nelem/elemsz combo
         * both must be positive non-zero values.
         */
        if (elemsz <= 0 || nelem <= 0)
        {
            PyErr_Format(PyExc_ValueError, "Cannot instantiate array with nelem=%zd elemsz=%zd", nelem, elemsz);
            break;
        }

        /*
         * If the maxsize keyword wasn't specified, then
         * fill it in now based on the nelem and elemsz
         */
        if (maxsize <= 0)
        {
            maxsize = elemsz * nelem;
            if (!EdsLib_Python_ObjectBase_SetKwArg(kwds, "n", "maxsize", maxsize))
            {
                break;
            }
        }

        /*
         * Call the initializer for the base object
         */
        if (EdsLib_Python_ObjectArrayType.tp_base->tp_init(obj, NULL, kwds) != 0)
        {
            break;
        }

        self->RefDbEntry = dbent;
        dbent = NULL; /* steal ref */
        self->ElementCount = nelem;
        self->ElementSize = elemsz;

        /*
         * At this point the Python object is fully initialized....
         * Proceed to initialization of the object buffer content, if necessary.
         * Many times this is not necessary -- the parent initializer can take care of it
         * if this was a static EDS array, or the buffer could be pointing to an existing
         * object which is already initialized.
         */
        if (!EdsLib_Python_Buffer_IsInitialized(self->objbase.StorageBuf))
        {
            /*
             * Now get the type info for the _element_ rather than the array itself.
             * This should always be present
             */
            if (EdsLib_DataTypeDB_GetTypeInfo(self->RefDbEntry->EdsDb->GD, self->RefDbEntry->EdsId, &TypeInfo) != EDSLIB_SUCCESS)
            {
                PyErr_Format(PyExc_RuntimeError, "Cannot get type info from EDS DB");
                break;
            }

            /* don't bother looping through an array of scalars.
             * (this is to speed up the case of a dynamic array of thousands or millions
             * of integers - such as ADC codes - and these have no initialization)
             * */
            if (TypeInfo.NumSubElements > 0)
            {
                EdsLib_Binding_Buffer_Content_t *content;
                EdsLib_Binding_DescriptorObject_t desc;

                content = EdsLib_Python_Buffer_GetContentRef(self->objbase.StorageBuf, PyBUF_WRITABLE);
                if (content == NULL)
                {
                    return -1;
                }

                memset(&desc, 0, sizeof(desc));

                desc.GD = self->RefDbEntry->EdsDb->GD;
                desc.EdsId = self->RefDbEntry->EdsId;
                desc.Offset = self->objbase.Offset;
                desc.Length = self->ElementSize;
                EdsLib_Binding_SetDescBuffer(&desc, content);

                for (idx = 0; idx < self->ElementCount; ++idx)
                {
                    EdsLib_Binding_InitStaticFields(&desc);
                    desc.Offset += self->ElementSize;
                }

                EdsLib_Binding_SetDescBuffer(&desc, NULL);
                EdsLib_Python_Buffer_ReleaseContentRef(content);
            }
        }

        result = 0;
    }
    while (0);

    Py_XDECREF(dbent);
    Py_XDECREF(kwds);

    return result;
}

