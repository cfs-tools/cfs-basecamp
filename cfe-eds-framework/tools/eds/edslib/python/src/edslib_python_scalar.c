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
 * \file     edslib_python_scalar.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement Python data type for EDS scalar objects.
**
**   This is to provide a better implementation of the "str" routine, which
**   converts EDS objects into strings.  Without this, the "repr" function
**   would be used which produces less useful results for passing into another
**   function which expects a string.
 */

#include "edslib_python_internal.h"

static PyObject *EdsLib_Python_NativeObject_ScalarType_str(PyObject *obj);

PyTypeObject EdsLib_Python_ObjectScalarType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("Scalar"),
    .tp_basicsize = sizeof(EdsLib_Python_ObjectBase_t),
    .tp_base = &EdsLib_Python_ObjectBaseType,
    .tp_str = EdsLib_Python_NativeObject_ScalarType_str,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("EDS ScalarDataType")
};

static PyObject *EdsLib_Python_NativeObject_ScalarType_str(PyObject *obj)
{
    PyObject *pyobj;
    PyObject *result = NULL;

    /*
     * First convert into the most appropriate native python object
     *
     * This may return a unicode object directly, if it is indeed a StringDataType in EDS.
     *
     * Note that only scalars should ever get here, as arrays and containers would
     * not be a derivative of this type.
     */
    pyobj = EdsLib_Python_ConvertEdsObjectToPython((EdsLib_Python_ObjectBase_t *)obj);

    if (pyobj != NULL)
    {
        if (pyobj->ob_type == &PyUnicode_Type)
        {
            /* already a unicode object, so return it */
            Py_INCREF(pyobj);
            result = pyobj;
        }
        else
        {
            /* further coerce the python object into a string */
            result = PyObject_Str(pyobj);
        }
        Py_DECREF(pyobj);
    }

    return result;
}
