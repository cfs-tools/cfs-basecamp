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
 * \file     edslib_python_buffer.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement Python wrapper type for EdsLib buffer objects
 */

#include "edslib_python_internal.h"

static PyObject *   EdsLib_Python_Buffer_repr(PyObject *obj);
static void         EdsLib_Python_Buffer_dealloc(PyObject *obj);

PyTypeObject EdsLib_Python_BufferType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("Buffer"),
    .tp_dealloc = EdsLib_Python_Buffer_dealloc,
    .tp_basicsize = sizeof(EdsLib_Python_Buffer_t),
    .tp_repr = EdsLib_Python_Buffer_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("EDS Buffer")
};


EdsLib_Binding_Buffer_Content_t* EdsLib_Python_Buffer_GetContentRef(EdsLib_Python_Buffer_t *self, int userflags)
{
    /*
     * Confirm writability
     * Check if the user has requested writable flags on an non-writable buffer
     */
    if ((userflags & PyBUF_WRITABLE) == PyBUF_WRITABLE && self->is_readonly)
    {
        PyErr_SetString(PyExc_BufferError, "Object is not writable.");
        return NULL;
    }

    if (self->edsbuf.ReferenceCount == 0 && self->bufobj != NULL)
    {
        /*
         * NOTE: since this buffer might be re-used, always get a writable buffer
         * if the underlying object is writable, regardless if this request is for
         * writable or not.
         */
        int bflags = PyBUF_SIMPLE;

        if (!self->is_readonly)
        {
            bflags |= PyBUF_WRITABLE;
        }

        if (PyObject_GetBuffer(self->bufobj->pyobj, &self->bufobj->view, bflags) != 0)
        {
            return NULL;
        }

        EdsLib_Binding_InitUnmanagedBuffer(&self->edsbuf, self->bufobj->view.buf, self->bufobj->view.len);
    }

    ++self->edsbuf.ReferenceCount;
    Py_INCREF(self);

    return &self->edsbuf;
}

void EdsLib_Python_Buffer_ReleaseContentRef(EdsLib_Binding_Buffer_Content_t* ref)
{
    union
    {
        uint8_t *byte;
        const EdsLib_Binding_Buffer_Content_t* ref;
        EdsLib_Python_Buffer_t *self;
    } ptr;

    ptr.ref = ref;
    ptr.byte -= offsetof(EdsLib_Python_Buffer_t, edsbuf);

    if (ptr.self->edsbuf.ReferenceCount > 0)
    {
        --ptr.self->edsbuf.ReferenceCount;
        if (ptr.self->edsbuf.ReferenceCount == 0 && ptr.self->bufobj != NULL)
        {
            PyBuffer_Release(&ptr.self->bufobj->view);
        }
    }
    Py_DECREF(ptr.self);
}

static void EdsLib_Python_Buffer_dealloc(PyObject *obj)
{
    EdsLib_Python_Buffer_t *self = (EdsLib_Python_Buffer_t*)obj;
    if (self->bufobj)
    {
        Py_DECREF(self->bufobj->pyobj);
        PyMem_Free(self->bufobj);
        self->bufobj = NULL;
    }
    if (self->is_dynamic && self->edsbuf.Data != NULL)
    {
        PyMem_Free(self->edsbuf.Data);
        self->edsbuf.Data = NULL;
    }
    PyObject_Del(obj);
}

static PyObject *EdsLib_Python_Buffer_repr(PyObject *obj)
{
    EdsLib_Python_Buffer_t *self = (EdsLib_Python_Buffer_t*)obj;
    PyObject *result;

    if (self->bufobj != NULL)
    {
        result = PyUnicode_FromFormat("%s(%R)",obj->ob_type->tp_name,self->bufobj->pyobj);
    }
    else
    {
        result = PyUnicode_FromFormat("%s(%p,%zu)",obj->ob_type->tp_name, self->edsbuf.Data, self->edsbuf.MaxContentSize);
    }

    return result;
}

EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_FromObject(PyObject *bufobj, int readonly)
{
    EdsLib_Python_Buffer_t *self = NULL;
    EdsLib_Python_View_t *viewobj = NULL;

    do
    {
        /*
         * If the supplied object is already an EDS buffer, no need to duplicate,
         * just increase the refcount.
         */
        if (Py_TYPE(bufobj) == &EdsLib_Python_BufferType)
        {
            self = (EdsLib_Python_Buffer_t *)bufobj;
            Py_INCREF(bufobj);
            break;
        }

        viewobj = PyMem_Malloc(sizeof(EdsLib_Python_View_t*));
        if (viewobj == NULL)
        {
            PyErr_NoMemory();
            break;
        }

        self = PyObject_New(EdsLib_Python_Buffer_t, &EdsLib_Python_BufferType);
        if (self == NULL)
        {
            break;
        }

        /*
         * Note that PyObject_New() only initializes fields in the
         * PyObject header, so all the local fields are undefined.
         * Each must be individually set here or else unpredictable
         * behavior ensues.
         */
        memset(viewobj, 0, sizeof(*viewobj));
        Py_INCREF(bufobj);
        viewobj->pyobj = bufobj;
        self->bufobj = viewobj;
        self->is_readonly = readonly;
        self->is_dynamic = 0;
        self->is_initialized = 1;
        memset(&self->edsbuf, 0, sizeof(self->edsbuf));
    }
    while(0);

    if (self == NULL && viewobj != NULL)
    {
        PyMem_Free(viewobj);
    }

    return self;
}

EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_New(Py_ssize_t len)
{
    EdsLib_Python_Buffer_t* self;
    void *mem;

    mem = PyMem_Malloc(len);
    if (mem == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }

    self = PyObject_New(EdsLib_Python_Buffer_t, &EdsLib_Python_BufferType);
    if (self != NULL)
    {
        /*
         * Note that PyObject_New() only initializes fields in the
         * PyObject header, so all the local fields are undefined.
         * Each must be individually set here or else unpredictable
         * behavior ensues.
         */
        self->bufobj = NULL;
        self->is_readonly = 0;
        self->is_dynamic = 1;
        self->is_initialized = 0;
        memset(mem, 0, len);
        EdsLib_Binding_InitUnmanagedBuffer(&self->edsbuf, mem, len);
    }
    else
    {
        PyMem_Free(mem);
    }

    return self;
}

EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_FromPtrAndSize(void *buf, Py_ssize_t len)
{
    EdsLib_Python_Buffer_t* self;

    self = PyObject_New(EdsLib_Python_Buffer_t, &EdsLib_Python_BufferType);
    if (self != NULL)
    {
        /*
         * Note that PyObject_New() only initializes fields in the
         * PyObject header, so all the local fields are undefined.
         * Each must be individually set here or else unpredictable
         * behavior ensues.
         */
        self->bufobj = NULL;
        self->is_readonly = 0;
        self->is_dynamic = 0;
        self->is_initialized = 1;
        EdsLib_Binding_InitUnmanagedBuffer(&self->edsbuf, buf, len);
    }

    return self;
}

EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_Copy(const void *buf, Py_ssize_t len)
{
    EdsLib_Python_Buffer_t* self = (EdsLib_Python_Buffer_t*)EdsLib_Python_Buffer_New(len);

    if (self != NULL)
    {
        memcpy(self->edsbuf.Data, buf, len);
        self->is_initialized = 1;
    }

    return self;
}

EdsLib_Python_Buffer_t* EdsLib_Python_Buffer_FromConstPtrAndSize(const void *buf, Py_ssize_t len)
{
    EdsLib_Python_Buffer_t* self;

    self = PyObject_New(EdsLib_Python_Buffer_t, &EdsLib_Python_BufferType);
    if (self != NULL)
    {
        /*
         * Note that PyObject_New() only initializes fields in the
         * PyObject header, so all the local fields are undefined.
         * Each must be individually set here or else unpredictable
         * behavior ensues.
         */
        self->bufobj = NULL;
        self->is_readonly = 1;
        self->is_dynamic = 0;
        self->is_initialized = 1;
        EdsLib_Binding_InitUnmanagedBuffer(&self->edsbuf, (void*)buf, len);
    }

    return self;
}

const void *EdsLib_Python_Buffer_Peek(EdsLib_Python_Buffer_t* buf)
{
    /* cannot peek into Python-backed objects */
    if (buf->bufobj != NULL)
    {
        return NULL;
    }

    return buf->edsbuf.Data;
}

Py_ssize_t EdsLib_Python_Buffer_GetMaxSize(EdsLib_Python_Buffer_t* buf)
{
    EdsLib_Binding_Buffer_Content_t *content;
    Py_ssize_t MaxSize = -1;

    content = EdsLib_Python_Buffer_GetContentRef(buf, PyBUF_SIMPLE);
    if (content != NULL)
    {
        MaxSize = content->MaxContentSize;
        EdsLib_Python_Buffer_ReleaseContentRef(content);
    }

    return MaxSize;
}

bool EdsLib_Python_Buffer_IsInitialized(EdsLib_Python_Buffer_t* buf)
{
    return buf->is_initialized;
}

void EdsLib_Python_Buffer_SetInitialized(EdsLib_Python_Buffer_t* buf)
{
    buf->is_initialized = 1;
}

