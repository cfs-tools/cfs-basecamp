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
 * \file     edslib_python_number.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement Python data type for EDS number objects
**
**   This extends the base object type and implements the "number protocol"
**   so EDS objects can work with all standard Python functions that accept numbers.
 */

#include "edslib_python_internal.h"

/*
 * Only implementing the "number conversion" methods.
 * This should allow EDS numbers to be coerced into integer or floats as necessary
 */
static PyObject * EdsLib_Python_NativeObject_NumberAsInt(PyObject *obj);
static PyObject * EdsLib_Python_NativeObject_NumberAsFloat(PyObject *obj);
static int        EdsLib_Python_NativeObject_NumberAsBool(PyObject *obj);


static PyNumberMethods EdsLib_Python_NumberMethods =
{
#if PY_MAJOR_VERSION >= 3
        .nb_bool = EdsLib_Python_NativeObject_NumberAsBool,
        .nb_int = EdsLib_Python_NativeObject_NumberAsInt,
#else
        .nb_nonzero = EdsLib_Python_NativeObject_NumberAsBool,
        .nb_long = EdsLib_Python_NativeObject_NumberAsInt,
#endif
        .nb_float = EdsLib_Python_NativeObject_NumberAsFloat,
        .nb_index = EdsLib_Python_NativeObject_NumberAsInt
};

PyTypeObject EdsLib_Python_ObjectNumberType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("Number"),
    .tp_basicsize = sizeof(EdsLib_Python_ObjectBase_t),
    .tp_base = &EdsLib_Python_ObjectScalarType,
    .tp_as_number = &EdsLib_Python_NumberMethods,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("EDS Number Value")
};

static void EdsLib_Python_NativeObject_GetValueBuffer(PyObject *obj, EdsLib_GenericValueBuffer_t *ValBuf)
{
    EdsLib_Python_ObjectBase_t *self = (EdsLib_Python_ObjectBase_t *)obj;
    EdsLib_BasicType_t DesiredValueType;
    EdsLib_Binding_DescriptorObject_t viewdesc;

    DesiredValueType = ValBuf->ValueType;
    ValBuf->ValueType = EDSLIB_BASICTYPE_NONE;

    if (!PyType_IsSubtype(Py_TYPE(obj), &EdsLib_Python_ObjectBaseType) ||
            !PyType_IsSubtype(Py_TYPE(obj->ob_type), &EdsLib_Python_DatabaseEntryType))
    {
        PyErr_SetString(PyExc_TypeError, "Object is not an EDS type");
        return;
    }

    if (!EdsLib_Python_SetupObjectDesciptor(self, &viewdesc, PyBUF_SIMPLE))
    {
        return;
    }

    if (EdsLib_Binding_LoadValue(&viewdesc, ValBuf) == EDSLIB_SUCCESS)
    {
        EdsLib_DataTypeConvert(ValBuf, DesiredValueType);
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot load number from EDS object buffer");
    }

    EdsLib_Python_ReleaseObjectDesciptor(&viewdesc);
}

static PyObject * EdsLib_Python_NativeObject_NumberAsInt(PyObject *obj)
{
    EdsLib_GenericValueBuffer_t ValBuf;
    PyObject *result;

    ValBuf.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
    ValBuf.Value.SignedInteger = 0;

    EdsLib_Python_NativeObject_GetValueBuffer(obj, &ValBuf);

    if (ValBuf.ValueType == EDSLIB_BASICTYPE_SIGNED_INT)
    {
        result = PyLong_FromLongLong(ValBuf.Value.SignedInteger);
    }
    else
    {
        /* normally the error context would have already been set */
        if (!PyErr_Occurred())
        {
            PyErr_SetString(PyExc_RuntimeError, "Unable to convert to integer");
        }
        result = NULL;
    }

    return result;
}

static PyObject * EdsLib_Python_NativeObject_NumberAsFloat(PyObject *obj)
{
    EdsLib_GenericValueBuffer_t ValBuf;
    PyObject *result;

    ValBuf.ValueType = EDSLIB_BASICTYPE_FLOAT;
    ValBuf.Value.FloatingPoint = 0;

    EdsLib_Python_NativeObject_GetValueBuffer(obj, &ValBuf);

    if (ValBuf.ValueType == EDSLIB_BASICTYPE_FLOAT)
    {
        result = PyFloat_FromDouble(ValBuf.Value.FloatingPoint);
    }
    else
    {
        /* normally the error context would have already been set */
        if (!PyErr_Occurred())
        {
            PyErr_SetString(PyExc_RuntimeError, "Unable to convert to float");
        }
        result = NULL;
    }

    return result;
}

static int        EdsLib_Python_NativeObject_NumberAsBool(PyObject *obj)
{
    EdsLib_GenericValueBuffer_t ValBuf;

    ValBuf.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
    ValBuf.Value.FloatingPoint = 0;

    EdsLib_Python_NativeObject_GetValueBuffer(obj, &ValBuf);

    return (ValBuf.ValueType == EDSLIB_BASICTYPE_UNSIGNED_INT &&
            ValBuf.Value.UnsignedInteger != 0);
}
