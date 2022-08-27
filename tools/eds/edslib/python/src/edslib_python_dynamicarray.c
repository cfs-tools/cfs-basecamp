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
 * \file     edslib_python_dynamicarray.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement Python type to represent a sequence (array) of other EDS objects
**
**   This allows an arbitrary multiple of instances of any other EDS object type
**   where the data is stored sequentially in memory.  These are arrays that are
**   created dynamically at run time rather than statically sized at compile time.
**
**   In contrast to static arrays, these instance types are _not_ directly
**   defined in EDS.
 */

#include "edslib_python_internal.h"


static int          EdsLib_Python_DynamicArray_init(PyObject *obj, PyObject *args, PyObject *kwds);
static PyObject *   EdsLib_Python_DynamicArray_call(PyObject *obj, PyObject *args, PyObject *kwds);

PyTypeObject EdsLib_Python_DynamicArrayType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("DynamicArray"),
    .tp_basicsize = sizeof(EdsLib_Python_ObjectArray_t),
    .tp_base = &EdsLib_Python_ObjectArrayType,
    .tp_init = EdsLib_Python_DynamicArray_init,
    .tp_call = EdsLib_Python_DynamicArray_call,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("EDS Dynamic Array Type")
};

static int          EdsLib_Python_DynamicArray_init(PyObject *obj, PyObject *args, PyObject *kwds)
{
    static const char *kwlist[] = { "dbent", "nelem", "elemsz", NULL };
    int result = -1;

    kwds = EdsLib_Python_ObjectBase_InitArgsToKwds(args, kwds, kwlist);
    if (kwds != NULL)
    {
        result = EdsLib_Python_DynamicArrayType.tp_base->tp_init(obj, NULL, kwds);

        Py_DECREF(kwds);
    }

    return result;
}

static PyObject *EdsLib_Python_DynamicArray_call(PyObject *obj, PyObject *args, PyObject *kwds)
{
    EdsLib_Python_ObjectArray_t *self = (EdsLib_Python_ObjectArray_t *)obj;
    PyObject *inseq = NULL;
    PyObject *outlist = NULL;
    PyObject *result = NULL;
    PyObject *subarg = NULL;
    PyObject *element = NULL;
    Py_ssize_t idx = 0;
    Py_ssize_t maxlen = -1;
    ternaryfunc callfunc = ((PyTypeObject*)self->RefDbEntry)->tp_call;


    /*
     * This function is basically just a "call iterator" that calls
     * every element with a corresponding arg from a sequence object.
     */


    /*
     * This functions has two modes:
     *  - If no arguments are provided, it acts a "getter"
     *      In this case it outputs a list
     *  - If an argument is provided, it is a "setter"
     *      In this case it needs a read-write buffer
     */
    do
    {
        if (callfunc == NULL)
        {
            /*
             * Not expected to happen, but do not crash it if does.
             * Anything inheriting from ObjectBase should implement tp_call.
             */
            PyErr_Format(PyExc_TypeError, "\'%s\' does not implement tp_call()\n",
                    ((PyTypeObject*)self->RefDbEntry)->tp_name);
            break;
        }

        if (args != NULL &&
                !PyArg_UnpackTuple(args, __func__, 0, 1, &inseq))
        {
            break;
        }

        if (inseq == NULL)
        {
            maxlen = self->ElementCount;
            outlist = PyList_New(self->ElementCount);
            if (outlist == NULL)
            {
                break;
            }
        }
        else if (!PySequence_Check(inseq))
        {
            PyErr_Format(PyExc_TypeError, "%s(): argument type \'%s\' is not a sequence", __func__, Py_TYPE(inseq)->tp_name);
            break;
        }
        else
        {
            maxlen = PySequence_Size(inseq);
            if (maxlen > self->ElementCount)
            {
                maxlen = self->ElementCount;
            }
            subarg = PyTuple_Pack(1, Py_None);
            if (subarg == NULL)
            {
                break;
            }
        }

        for (idx = 0; idx < maxlen; ++idx)
        {
            Py_XDECREF(element);
            element = PySequence_GetItem(obj, idx);
            if (element == NULL)
            {
                break;
            }

            /*
             * Recycle the existing 1-element tuple
             * Set item 0 to be the current input value.
             * This is for efficiency - it is possible because we know that the
             * base call implementation will not keep a reference to the tuple.
             */
            if (subarg != NULL)
            {
                PyObject *invalue = PySequence_GetItem(inseq, idx); /* new ref */
                if (invalue == NULL)
                {
                    break;
                }

                /*
                 * Need to manually DECREF old item first since
                 * PyTuple_SET_ITEM (macro form) leaks any existing value
                 */
                Py_DECREF(PyTuple_GET_ITEM(subarg, 0));
                PyTuple_SET_ITEM(subarg, 0, invalue); /* steals ref */
            }

            /*
             * Invoke the call routine on the subobject, with the argument from input sequence (if set)
             */
            {
                PyObject *outvalue = callfunc(element, subarg, NULL);

                if (outvalue == NULL)
                {
                    break;
                }

                /*
                 * If in getter mode (outlist is non-null) then save the result
                 * to the outlist.  Otherwise toss out the result.
                 */
                if (outlist != NULL)
                {
                    PyList_SET_ITEM(outlist, idx, outvalue); /* steals ref */
                }
                else
                {
                    Py_DECREF(outvalue);    /* discard ref */
                }
            }
        }

        if (idx != maxlen)
        {
            /* error occurred somewhere */
            break;
        }

        /*
         * Successful....
         * Figure out what to return to the caller
         */
        if (outlist != NULL)
        {
            /* getter mode - return the output list */
            result = outlist;
        }
        else
        {
            /* setter mode - return None */
            result = Py_None;
        }
        Py_INCREF(result);
    }
    while(0);

    /*
     * DECREF any owned objects used during this procedure.
     */
    Py_XDECREF(outlist);
    Py_XDECREF(subarg);
    Py_XDECREF(element);

    return result;
}


PyObject *   EdsLib_Python_DynamicArray_FromPtrAndSize(PyTypeObject *objtype, void *ptr, Py_ssize_t nelem, Py_ssize_t elemsz)
{
    static const char *kwlist[] = { "buffer", "dbent", "nelem", "elemsz", NULL };
    PyObject *args = NULL;
    PyObject *result = NULL;

    args = EdsLib_Python_ObjectBase_BuildKwArgs("NOnn", kwlist,
            EdsLib_Python_Buffer_FromPtrAndSize(ptr, nelem * elemsz),
            objtype, nelem, elemsz);
    if (args == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_GenericNew(&EdsLib_Python_DynamicArrayType, args);

    Py_DECREF(args);

    return result;
}

PyObject *   EdsLib_Python_DynamicArray_FromConstPtrAndSize(PyTypeObject *objtype, const void *ptr, Py_ssize_t nelem, Py_ssize_t elemsz)
{
    static const char *kwlist[] = { "buffer", "dbent", "nelem", "elemsz", NULL };
    PyObject *args = NULL;
    PyObject *result = NULL;

    args = EdsLib_Python_ObjectBase_BuildKwArgs("NOnn", kwlist,
            EdsLib_Python_Buffer_FromConstPtrAndSize(ptr, nelem * elemsz),
            objtype, nelem, elemsz);
    if (args == NULL)
    {
        return NULL;
    }

    result = EdsLib_Python_ObjectBase_GenericNew(&EdsLib_Python_DynamicArrayType, args);

    Py_DECREF(args);

    return result;
}




