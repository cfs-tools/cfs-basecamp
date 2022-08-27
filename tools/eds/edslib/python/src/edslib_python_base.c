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
 * \file     edslib_python_base.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement common base type Python bindings for EDS objects
**
**   This extends the accessor type with actual instance information, mainly
**   a buffer object to hold the EDS object described by the accessor.
**
**   This is functionality that every instance of an EDS object should implement,
**   such as the buffer protocol.
 */

/*
 * CONSTRUCTION FLOW:
 * Basic requirements:
 *
 * EDS Objects can be instantiated from either Python or C
 *
 *    From Python:
 *      The default DataType(arg) constructor call shall create a _new_ instance.  Arg
 *      can be any python type, or omitted entirely.  A new instance will be created and
 *      initialized based on the argument, using whatever coercion/protocol scheme is
 *      deemed appropriate given the context (mapping for containers, sequence for arrays,
 *      number protocol for numbers, etc etc).  In all cases the result will be a copy.
 *      The original argument will be unchanged, and no reference to it is kept after
 *      construction.
 *
 *      A "reference" construction can specify the "buffer=" keyword to wrap any
 *      existing Python object that supports the buffer protocol and manipulate it as an
 *      EDS object.  In this mode the buffer is _referenced_, not copied.
 *      The idea here is that code should be explicit that it actually wants to treat
 *      this as a reference/buffer and not actually create a new EDS object.
 *
 *      For both of these constructions, the "tp_new" and "tp_init" methods are invoked
 *      by Python per the calling context, with "args" and "kwds" parameters set accordingly.
 *
 *      "tp_new" only performs allocation, no initialization whatsoever.
 *      "tp_init" handles all initialization, even for internal fields.
 *
 *      IMPORTANT: "tp_init" can be overridden as necessary by derived types to provide
 *      additional initialization procedures.  The init implementation in this file is only
 *      for the fields in the corresponding base object.  Typically a derived type (such as
 *      array) would call this base init routine first, followed by its own init.  This
 *      provides for a sane initialization without having to hardcode the knowledge of
 *      derived types at this level.
 *
 *
 *    From C:
 *      In C the initialization flow is not as clean, since we do not get the benefit
 *      of the Python interpreter calling the correct init automatically.  However,
 *      for the benefit of client code, it is important to have a single API call
 *      that can properly construct and initialize an EDS object of any type.
 *
 *      One option is to actually invoke the python interpreter from C which has the
 *      benefit of following the exact same construction flow (good) but requires
 *      instantiation and initialization of the argument objects as required for the
 *      Python calling convention (bad).  It is anticipated that the typical use-case
 *      from C would be to wrap an existing pointer and size, which mean invoking the
 *      buffer constructor, which requires creation of a dictionary object, so this would
 *      be the least efficient possible thing.
 *
 *    Compromise approach:
 *      It is desired to use the same "tp_init" that the Python interpreter calls to
 *      support efficient initialization of objects created from C.  To do this, a
 *      slight overload is done, as follows:
 *
 *      The exception is if the first positional argument is specifically of the EDS
 *      "Buffer" type (i.e. an instance of the EdsLib_Python_BufferType).  In this mode,
 *      the same buffer object will be referenced instead of creating a new buffer as
 *      the backing store.
 *
 *      Note that backing store objects (always of EdsLib_Python_BufferType) are generally
 *      not exposed to Python - the type does not implement "tp_new" nor is it exposed
 *      through any other attribute.  Also, exact type checks such as this are fast
 *      and easy in C as it is just checking a pointer for equality.  So for all intents
 *      and purposes, it is not possible to invoke the init function in this form
 *      through Python, it can only be done through C.
 *
 *
 *
 */


#include "edslib_python_internal.h"

static char         EDSLIB_PYTHON_BYTES_FORMAT[] = "B";

static void         EdsLib_Python_ObjectBase_dealloc(PyObject * obj);
static int          EdsLib_Python_ObjectBase_init(PyObject *obj, PyObject *args, PyObject *kwds);
static PyObject *   EdsLib_Python_ObjectBase_repr(PyObject *obj);
static PyObject *   EdsLib_Python_ObjectBase_call(PyObject *obj, PyObject *args, PyObject *kwds);
static int          EdsLib_Python_ObjectBase_getbuffer(PyObject *obj, Py_buffer *view, int flags);
static void         EdsLib_Python_ObjectBase_releasebuffer(PyObject *obj, Py_buffer *view);

static PyBufferProcs EdsLib_Python_ObjectBase_BufferProcs =
{
        .bf_getbuffer = EdsLib_Python_ObjectBase_getbuffer,
        .bf_releasebuffer = EdsLib_Python_ObjectBase_releasebuffer
};


PyTypeObject EdsLib_Python_ObjectBaseType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("Object"),
    .tp_basicsize = sizeof(EdsLib_Python_ObjectBase_t),
    .tp_dealloc = EdsLib_Python_ObjectBase_dealloc,
    .tp_init = EdsLib_Python_ObjectBase_init,
    .tp_new = PyType_GenericNew,
    .tp_repr = EdsLib_Python_ObjectBase_repr,
    .tp_call = EdsLib_Python_ObjectBase_call,
    .tp_as_buffer = &EdsLib_Python_ObjectBase_BufferProcs,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("EDS Object")
};

static void EdsLib_Python_ObjectBase_dealloc(PyObject * obj)
{
    EdsLib_Python_ObjectBase_t *self = (EdsLib_Python_ObjectBase_t *)obj;

    Py_CLEAR(self->StorageBuf);

    EdsLib_Python_ObjectBaseType.tp_base->tp_dealloc(obj);
}

PyObject *EdsLib_Python_ObjectBase_GenericNew(PyTypeObject *objtype, PyObject *kwargs)
{
    PyObject *self = NULL;

    do
    {
        /* sanity check -- this only constructs EDS object instance types */
        /* Note - A SubType is intended here, the base type is abstract,
         * so to construct an object it should aways be a subtype */
        if (!PyType_IsSubtype(objtype, &EdsLib_Python_ObjectBaseType))
        {
            PyErr_Format(PyExc_TypeError, "Cannot construct EDS objects of type %s\n",
                    objtype->tp_name);
            break;
        }

        self = objtype->tp_alloc(objtype, 0);
        if (self == NULL)
        {
            break;
        }

        if (objtype->tp_init(self, NULL, kwargs) != 0)
        {
            Py_DECREF(self);
            self = NULL;
            break;
        }
    }
    while(0);


    return self;
}

PyObject *EdsLib_Python_ObjectBase_NewSubObject(PyObject *obj, PyTypeObject *subobjtype, Py_ssize_t Offset, Py_ssize_t MaxSize)
{
    static const char *kwlist[] = { "buffer", "offset", "maxsize", NULL };
    PyObject *args = NULL;
    PyObject *result = NULL;
    EdsLib_Python_ObjectBase_t *self;

    /* sanity check -- the type should always be an EDS object */
    if (!PyObject_TypeCheck(obj, &EdsLib_Python_ObjectBaseType))
    {
        PyErr_Format(PyExc_TypeError, "Object is not an EDS object type: %s", Py_TYPE(obj)->tp_name);
        return NULL;
    }

    self = (EdsLib_Python_ObjectBase_t *)obj;

    args = EdsLib_Python_ObjectBase_BuildKwArgs("Onn", kwlist,
            self->StorageBuf, Offset + self->Offset, MaxSize);
    if (args == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_GenericNew(subobjtype, args);
    Py_DECREF(args);

    return result;
}


PyObject *EdsLib_Python_ObjectFromBuffer(PyTypeObject *objtype, PyObject *bufobj, int readonly)
{
    static const char *kwlist[] = { "buffer", NULL };
    PyObject *args = NULL;
    PyObject *result = NULL;

    args = EdsLib_Python_ObjectBase_BuildKwArgs("N", kwlist,
            EdsLib_Python_Buffer_FromObject(bufobj, readonly));
    if (args == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_GenericNew(objtype, args);
    Py_DECREF(args);

    return result;

}


PyObject *EdsLib_Python_ObjectFromPtr(PyTypeObject *objtype, void *ptr, Py_ssize_t size)
{
    static const char *kwlist[] = { "buffer", NULL };
    PyObject *args = NULL;
    PyObject *result = NULL;

    args = EdsLib_Python_ObjectBase_BuildKwArgs("N", kwlist,
            EdsLib_Python_Buffer_FromPtrAndSize(ptr, size));
    if (args == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_GenericNew(objtype, args);
    Py_DECREF(args);

    return result;
}

PyObject *EdsLib_Python_ObjectNewCopy(PyTypeObject *objtype, const void *ptr, Py_ssize_t size)
{
    static const char *kwlist[] = { "buffer", NULL };
    PyObject *args = NULL;
    PyObject *result = NULL;

    args = EdsLib_Python_ObjectBase_BuildKwArgs("N", kwlist,
            EdsLib_Python_Buffer_Copy(ptr, size));
    if (args == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_GenericNew(objtype, args);
    Py_DECREF(args);

    return result;
}

PyObject *EdsLib_Python_ObjectFromConstPtr(PyTypeObject *objtype, const void *ptr, Py_ssize_t size)
{
    static const char *kwlist[] = { "buffer", NULL };
    PyObject *args = NULL;
    PyObject *result = NULL;

    args = EdsLib_Python_ObjectBase_BuildKwArgs("N", kwlist,
            EdsLib_Python_Buffer_FromConstPtrAndSize(ptr, size));
    if (args == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_GenericNew(objtype, args);
    Py_DECREF(args);

    return result;
}

const void *EdsLib_Python_ObjectPeek(PyObject *obj)
{
    EdsLib_Python_ObjectBase_t *self = NULL;

    /* sanity check -- should always be an EDS object */
    if (!PyObject_TypeCheck(obj, &EdsLib_Python_ObjectBaseType))
    {
        return NULL;
    }

    self = (EdsLib_Python_ObjectBase_t *)obj;

    return EdsLib_Python_Buffer_Peek(self->StorageBuf);
}

static int EdsLib_Python_ObjectBase_init(PyObject *obj, PyObject *args, PyObject *kwds)
{
    static const char *in_kwlist[] = { "value", NULL };
    EdsLib_Python_ObjectBase_t *self = (EdsLib_Python_ObjectBase_t *)obj;
    EdsLib_Binding_DescriptorObject_t viewdesc;
    PyObject *srcvalue = NULL;
    PyObject *bufobj = NULL;
    Py_ssize_t MaxSize = 0;
    Py_ssize_t Offset = 0;
    int result = -1;

    /*
     * First drop any existing buffer (e.g. in case __init__ is invoked directly)
     */
    Py_CLEAR(self->StorageBuf);

    kwds = EdsLib_Python_ObjectBase_InitArgsToKwds(args, kwds, in_kwlist);
    args = NULL;    /* should no longer be used directly */

    do
    {
        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|O:ObjectBase_init", "value", &srcvalue))
        {
            break;
        }

        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|O:ObjectBase_init", "buffer", &bufobj))
        {
            break;
        }

        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|n:ObjectBase_init", "offset", &Offset))
        {
            break;
        }

        if (!EdsLib_Python_ObjectBase_GetKwArg(kwds,"|n:ObjectBase_init", "maxsize", &MaxSize))
        {
            break;
        }

        if (bufobj == NULL || bufobj == Py_None)
        {
            /* creating a new buffer.
             * allocate according to the "worst-case" size of the object.
             * If not actually a database entry, this will fail w/exception. */
            if (MaxSize == 0)
            {
                MaxSize = EdsLib_Python_DatabaseEntry_GetMaxSize(Py_TYPE(obj));
                if (MaxSize < 0)
                {
                    break;
                }
            }

            self->StorageBuf = EdsLib_Python_Buffer_New(Offset + MaxSize);
            if (self->StorageBuf == NULL)
            {
                break;
            }
        }
        else
        {
            /* using an existing buffer.
             * if maxsize was not specified, get the size from the buffer. */
            self->StorageBuf = EdsLib_Python_Buffer_FromObject(bufobj, 0);
            if (self->StorageBuf == NULL)
            {
                break;
            }

            if (MaxSize == 0)
            {
                MaxSize = EdsLib_Python_Buffer_GetMaxSize(self->StorageBuf);
                if (MaxSize < 0)
                {
                    break;
                }

                MaxSize -= Offset;
            }
        }

        /* catch invalid size/offset that hasn't been caught yet.
         * neither can be negative, and size must also not be 0 */
        if (Offset < 0 || MaxSize <= 0)
        {
            PyErr_Format(PyExc_ValueError, "Invalid size=%zd offset=%zd for EDS object", MaxSize, Offset);
            break;
        }

        self->Offset = Offset;
        self->TotalLength = MaxSize;

        /*
         * Initialize internal fields within the object
         * This is generally only done for "new" objects; it is not done when
         * wrapping an existing buffer or if the data was otherwise already initialized.
         * Note This default approach only works for those objects directly in EDS (DB entries)
         * For other object types like Dynamic Arrays they will need a custom routine to do this.
         */
        if (!EdsLib_Python_Buffer_IsInitialized(self->StorageBuf) &&
                Py_TYPE(Py_TYPE(self)) == &EdsLib_Python_DatabaseEntryType)
        {
            if (!EdsLib_Python_SetupObjectDesciptor(self, &viewdesc, PyBUF_WRITABLE))
            {
                break;
            }

            /* Pre-initialize the memory according to the EDS-defined constraints */
            EdsLib_Binding_InitStaticFields(&viewdesc);
            EdsLib_Python_ReleaseObjectDesciptor(&viewdesc);
            EdsLib_Python_Buffer_SetInitialized(self->StorageBuf);
        }

        /* attempt to initialize the new object from the passed in value.
         * If no srcvalue was provided, do nothing. */
        if (srcvalue != NULL &&
                !EdsLib_Python_ConvertPythonToEdsObject(self, srcvalue))
        {
            break;
        }

        result = 0;
    }
    while(0);

    Py_XDECREF(kwds);

    return result;
}

static PyObject *EdsLib_Python_ObjectBase_repr(PyObject *obj)
{
    PyObject *val;
    PyObject *result;
    ternaryfunc callfunc = Py_TYPE(obj)->tp_call;

    if (callfunc == NULL)
    {
        /*
         * Not expected to happen, but do not crash it if does.
         * Anything inheriting from ObjectBase should implement tp_call.
         */
        val = PyUnicode_FromString("??");
    }
    else
    {
        /*
         * Invoke the call routine to convert the EDS value into a fundamental
         * python value.  Calling with NULL args invokes "getter" mode.
         *
         * It is done this way instead of calling EdsLib_Python_ConvertEdsObjectToPython()
         * directly so that derived types can override this if necessary.
         */
        val = callfunc(obj, NULL, NULL);
    }

    if (val == NULL)
    {
        return NULL;
    }

    result = PyUnicode_FromFormat("%R(%R)", obj->ob_type, val);
    Py_DECREF(val);

    return result;

}

static PyObject *EdsLib_Python_ObjectBase_call(PyObject *obj, PyObject *args, PyObject *kwds)
{
    EdsLib_Python_ObjectBase_t *self = (EdsLib_Python_ObjectBase_t *)obj;
    PyObject *value = NULL;
    PyObject *result;

    /*
     * This functions has two modes:
     *  - If no arguments are provided, it acts a "getter"
     *      In this case it needs a read-only buffer
     *  - If an argument is provided, it is a "setter"
     *      In this case it needs a read-write buffer
     */
    if (args != NULL &&
            !PyArg_UnpackTuple(args, __func__, 0, 1, &value))
    {
        return NULL;
    }

    if (value == NULL)
    {
        /* getter mode */
        result = EdsLib_Python_ConvertEdsObjectToPython(self);
    }
    else if (EdsLib_Python_ConvertPythonToEdsObject(self, value))
    {
        /* setter mode, successful result.
         * Currently just returning "none" here */
        result = Py_None;
        Py_INCREF(result);
    }
    else
    {
        /* unsuccessful set, exception will be set */
        result = NULL;
    }

    return result;
}

int EdsLib_Python_ObjectBase_InitBufferView(EdsLib_Python_ObjectBase_t *self, Py_buffer *view, int flags)
{
    EdsLib_Binding_Buffer_Content_t* content;

    memset(view, 0, sizeof(*view));

    if (self->StorageBuf == NULL)
    {
        PyErr_SetString(PyExc_BufferError, "No buffer object");
        return -1;
    }

    content = EdsLib_Python_Buffer_GetContentRef(self->StorageBuf, flags & PyBUF_WRITABLE);
    if (content == NULL)
    {
        return -1;
    }

    /*
     * IMPORTANT:
     * If an error occurs after this point, the buffer must be released first.
     */

    if (content->MaxContentSize < (self->Offset + self->TotalLength))
    {
        PyErr_SetString(PyExc_BufferError, "Insufficient buffer for object");
        EdsLib_Python_Buffer_ReleaseContentRef(content);
        return -1;
    }

    /* adjust the view pointer/length to point to the window indicated by the accessor */
    view->obj = (PyObject*)self;
    Py_INCREF(view->obj);
    view->buf = content->Data + self->Offset;
    view->len = self->TotalLength;
    view->readonly = self->StorageBuf->is_readonly;
    view->itemsize = 0;
    view->ndim = 0;
    view->format = NULL;
    view->shape = NULL;
    view->strides = NULL;
    view->suboffsets = NULL;

    /*
     * Important last step --
     * save the content pointer to the internal field,
     * as this is what needs to be released later
     */
    view->internal = content;

    return 0;
}

static int EdsLib_Python_ObjectBase_getbuffer(PyObject *obj, Py_buffer *view, int flags)
{
    EdsLib_Python_ObjectBase_t * self = (EdsLib_Python_ObjectBase_t *)obj;

    if (EdsLib_Python_ObjectBase_InitBufferView(self, view, flags) != 0)
    {
        return -1;
    }

    /*
     * If a buffer is requested at this base object, treat it as a pure "binary blob"
     *  - Format char is always 'B' (unsigned char)
     *  - Item Size is always 1 (corresponds to 'B' by definition)
     *  - Shape, if requested, is equal to the total length
     */
    view->itemsize = 1;
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
    {
        view->format = EDSLIB_PYTHON_BYTES_FORMAT;
    }
    if ((flags & PyBUF_ND) == PyBUF_ND)
    {
        view->ndim = 1;
        view->shape = &view->len;
    }
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
    {
        view->strides = &view->itemsize;
    }


    return 0;
}

static void EdsLib_Python_ObjectBase_releasebuffer(PyObject *obj, Py_buffer *view)
{
    EdsLib_Python_Buffer_ReleaseContentRef(view->internal);
}

PyObject *EdsLib_Python_ObjectBase_InitArgsToKwds(PyObject *args, PyObject *kwds, const char **kwlist)
{
    PyObject *subargs;
    bool positional_args_valid = (args != NULL && PyTuple_Check(args));
    bool kw_args_valid = (kwds != NULL && PyDict_Check(kwds));
    Py_ssize_t idx;

    /*
     * Convert any positional args to keyword args
     * positional_args_valid should be true if this is called direct from Python.
     *
     * In any case create a new dictionary reference that can be passed on.
     * - If kwds is not valid at all then make a new dict
     * - If called from another init (NOT from python) then use the existing dict
     * - If called from Python w/a valid kwds dict, then copy it first before modifying
     */
    if (!kw_args_valid)
    {
        /* first invocation with no dictionary --
         * make a new one now and we will own it */
        subargs = PyDict_New();
    }
    else if (!positional_args_valid)
    {
        /* assume called from another init --
         * so the dict is already our own and can be modified. */
        subargs = kwds;
        Py_INCREF(subargs);
    }
    else
    {
        /* first invocation from Python with a kwds dict --
         * Use it as the base, but copy it first,
         * avoid modifying the dict from the interpreter */
        subargs = PyDict_Copy(kwds);
    }

    if (positional_args_valid && subargs != NULL && kwlist != NULL)
    {
        for (idx = 0; kwlist[idx] != NULL && idx < PyTuple_GET_SIZE(args); ++idx)
        {
            if (PyDict_SetItemString(subargs, kwlist[idx], PyTuple_GET_ITEM(args, idx)) != 0)
            {
                /* failed to insert -- unlikely but possible */
                Py_DECREF(subargs);
                subargs = NULL;
                break;
            }
        }
    }

    return subargs;
}

bool EdsLib_Python_ObjectBase_SetKwArg(PyObject *kwds, const char *format, const char *kw, ...)
{
    PyObject *args;
    bool result = false;
    va_list va;

    /*
     * First convert the value to a python object, then
     * populate the dictionary with the value.
     */
    va_start(va, kw);
    args = Py_VaBuildValue(format, va);
    va_end(va);


    if (args != NULL)
    {
        result = (PyDict_SetItemString(kwds, kw, args) == 0);
        Py_DECREF(args);
    }

    return result;
}

PyObject *EdsLib_Python_ObjectBase_BuildKwArgs(const char *format, const char **kwlist, ...)
{
    PyObject *args = NULL;
    PyObject *kwds = NULL;
    Py_ssize_t idx;
    va_list va;

    do
    {
        /*
         * First convert the value to a tuple, then use the tuple entries
         * to populate the dictionary using the keyword values.
         */
        va_start(va, kwlist);
        args = Py_VaBuildValue(format, va);
        va_end(va);
        if (args == NULL)
        {
            break;
        }

        kwds = PyDict_New();
        if (kwds == NULL)
        {
            break;
        }

        /*
         * Note that Py_BuildValue() only returns a tuple if 2 or more format chars were used.
         * If only a single format char was specified then it returns a single
         * Python object reflecting that format char - not a tuple.
         *
         * However, if it _does_ return a tuple, it is difficult to differentiate between
         * normal multiple values and an intentional tuple (i.e. wrapping the format in ())
         *
         * For those use-cases where one actually wants to pack a tuple into the keyword dict,
         * is probably better to use EdsLib_Python_ObjectBase_SetKwArg() for that argument, to
         * avoid the ambiguity here.
         */
        if (PyTuple_Check(args))
        {
            for (idx = 0; kwlist[idx] != NULL && idx < PyTuple_GET_SIZE(args); ++idx)
            {
                if (PyDict_SetItemString(kwds, kwlist[idx], PyTuple_GET_ITEM(args, idx)) != 0)
                {
                    /* failed to insert -- unlikely but possible */
                    Py_DECREF(kwds);
                    kwds = NULL;
                    break;
                }
            }
        }
        else if (kwlist[0] != NULL &&
                PyDict_SetItemString(kwds, kwlist[0], args) != 0)
        {
            /* failed to insert -- unlikely but possible */
            Py_DECREF(kwds);
            kwds = NULL;
            break;
        }
    }
    while(0);

    Py_XDECREF(args);

    return kwds;
}


bool EdsLib_Python_ObjectBase_GetKwArg(PyObject *kwds, const char *format, const char *kw, void *OutPtr)
{
    bool result;
    bool is_optional;
    PyObject *item;

    /*
     * Check if the format starts with a |
     * This is the PyArg_ParseTuple flag for optional args
     * As this only processes single arguments at a time, this char
     * should only appear at the beginning or not at all.
     */
    if (*format == '|')
    {
        is_optional = true;
        ++format;
    }
    else
    {
        is_optional = false;
    }

    item = PyDict_GetItemString(kwds, kw); /* borrowed ref */
    if (item != NULL && item != Py_None)
    {
        /*
         * Pass the item along to PyArg_Parse to convert it into the
         * correct type.  Sets an exception and returns nonzero if it fails.
         */
        result = (PyArg_Parse(item, format, OutPtr) != 0);
    }
    else if (is_optional)
    {
        /*
         * Arg was not supplied by caller...
         * If the arg is optional then return true
         */
        result = true;
    }
    else
    {
        /*
         * Arg was required but not supplied.
         * Generate a suitable exception here.
         */
        const char *fname;
        fname = strchr(format, ':');
        if (fname == NULL)
        {
            fname = __func__;
        }
        PyErr_Format(PyExc_TypeError, "%s(): keyword arg \'%s\' is required", fname, kw);
        result = false;
    }

    return result;
}

