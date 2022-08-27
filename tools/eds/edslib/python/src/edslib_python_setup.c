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
 * \file     edslib_python_setup.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement setup function / initializer for EDS/Python module objects
 */

#include "edslib_python_internal.h"

/*
 * Instantiation function.
 *
 * The C API differs between Python 3+ (PyModule_Create) and
 * Python 2 (Py_InitModule3).  This just creates a wrapper
 * to call the right one.
 */
#if (PY_MAJOR_VERSION >= 3)

static PyModuleDef EdsLib_Python_ModuleDef =
{
    PyModuleDef_HEAD_INIT,
    EDSLIB_PYTHON_MODULE_NAME,
    PyDoc_STR(EDSLIB_PYTHON_DOC),
    -1
};

static inline PyObject* EdsLib_Python_InstantiateModule(void)
{
    /* python3 uses PyModule_Create() API */
    return PyModule_Create(&EdsLib_Python_ModuleDef);
}

#else

static inline PyObject* EdsLib_Python_InstantiateModule(void)
{
    /* python2 uses Py_InitModule3() API */
    return Py_InitModule3(EDSLIB_PYTHON_MODULE_NAME, NULL, EDSLIB_PYTHON_DOC);
}

#endif


PyObject* EdsLib_Python_CreateModule(void)
{
    PyObject *m = NULL;

    do
    {
        /*
         * Prepare all of the types defined here
         */
        if (PyType_Ready(&EdsLib_Python_DatabaseType) != 0 ||
                PyType_Ready(&EdsLib_Python_DatabaseEntryType) != 0 ||
                PyType_Ready(&EdsLib_Python_BufferType) != 0 ||
                PyType_Ready(&EdsLib_Python_AccessorType) != 0 ||
                PyType_Ready(&EdsLib_Python_PackedObjectType) != 0 ||
                PyType_Ready(&EdsLib_Python_ContainerIteratorType) != 0 ||
                PyType_Ready(&EdsLib_Python_EnumEntryIteratorType) != 0 ||
                PyType_Ready(&EdsLib_Python_ContainerEntryIteratorType) != 0 ||
                PyType_Ready(&EdsLib_Python_ObjectBaseType) != 0 ||
                PyType_Ready(&EdsLib_Python_ObjectScalarType) != 0 ||
                PyType_Ready(&EdsLib_Python_ObjectNumberType) != 0 ||
                PyType_Ready(&EdsLib_Python_ObjectContainerType) != 0 ||
                PyType_Ready(&EdsLib_Python_ObjectArrayType) != 0 ||
                PyType_Ready(&EdsLib_Python_DynamicArrayType) != 0)
        {
            break;
        }

        if (EdsLib_Python_DatabaseCache == NULL)
        {
            EdsLib_Python_DatabaseCache = PyDict_New();
            if (EdsLib_Python_DatabaseCache == NULL)
            {
                break;
            }
        }

        m = EdsLib_Python_InstantiateModule();
        if (m == NULL)
        {
            break;
        }

        /*
         * Add appropriate types so object instances can be constructed
         */
        PyModule_AddObject(m, "Database", (PyObject*)&EdsLib_Python_DatabaseType);
        PyModule_AddObject(m, "DatabaseEntry", (PyObject*)&EdsLib_Python_DatabaseEntryType);
        PyModule_AddObject(m, "PackedObject", (PyObject*)&EdsLib_Python_PackedObjectType);
        PyModule_AddObject(m, "ElementAccessor", (PyObject*)&EdsLib_Python_AccessorType);
        PyModule_AddObject(m, "DynamicArray", (PyObject*)&EdsLib_Python_DynamicArrayType);
    }
    while(0);

    return m;
}

