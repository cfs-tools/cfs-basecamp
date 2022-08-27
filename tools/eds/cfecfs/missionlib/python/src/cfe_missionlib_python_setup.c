/*
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

/******************************************************************************
** File:  cfe_missionlib_python_setup.c
**
** Created on: Feb 11, 2020
** Author: mathew.j.mccaskey
**
** Purpose:
**   Implement setup function / initializer for EDS/MissionLib module objects
**
******************************************************************************/

#include "cfe_missionlib_python_internal.h"

/*
 * Instantiation function.
 *
 * The C API differs between Python 3+ (PyModule_Create) and
 * Python 2 (Py_InitModule3).  This just creates a wrapper
 * to call the right one.
 */
#if (PY_MAJOR_VERSION >= 3)

static PyModuleDef CFE_MissionLib_Python_ModuleDef =
{
    PyModuleDef_HEAD_INIT,
    CFE_MISSIONLIB_PYTHON_MODULE_NAME,
    PyDoc_STR(CFE_MISSIONLIB_PYTHON_DOC),
    -1
};

static inline PyObject* CFE_MissionLib_Python_InstantiateModule(void)
{
    /* python3 uses PyModule_Create() API */
    return PyModule_Create(&CFE_MissionLib_Python_ModuleDef);
}

#else

static inline PyObject* CFE_MissionLib_Python_InstantiateModule(void)
{
    /* python2 uses Py_InitModule3() API */
    return Py_InitModule3(CFE_MISSIONLIB_PYTHON_MODULE_NAME, NULL, CFE_MISSIONLIB_PYTHON_DOC);
}

#endif


PyObject* CFE_MissionLib_Python_CreateModule(void)
{
    PyObject *m = NULL;

    do
    {
        /*
         * Prepare all of the types defined here
         */
        if (PyType_Ready(&CFE_MissionLib_Python_DatabaseType) != 0 ||
            PyType_Ready(&CFE_MissionLib_Python_InterfaceType) != 0 ||
            PyType_Ready(&CFE_MissionLib_Python_TopicType) != 0)
        {
            break;
        }

        if (CFE_MissionLib_Python_DatabaseCache == NULL)
        {
            CFE_MissionLib_Python_DatabaseCache = PyDict_New();
            if (CFE_MissionLib_Python_DatabaseCache == NULL)
            {
                break;
            }
        }

        if (CFE_MissionLib_Python_InterfaceCache == NULL)
        {
            CFE_MissionLib_Python_InterfaceCache = PyDict_New();
            if (CFE_MissionLib_Python_InterfaceCache == NULL)
            {
                break;
            }
        }

        if (CFE_MissionLib_Python_TopicCache == NULL)
        {
            CFE_MissionLib_Python_TopicCache = PyDict_New();
            if (CFE_MissionLib_Python_TopicCache == NULL)
            {
                break;
            }
        }

        m = CFE_MissionLib_Python_InstantiateModule();
        if (m == NULL)
        {
            break;
        }

        /*
         * Add appropriate types so object instances can be constructed
         */
        PyModule_AddObject(m, "Database", (PyObject*)&CFE_MissionLib_Python_DatabaseType);
        PyModule_AddObject(m, "Interface", (PyObject*)&CFE_MissionLib_Python_InterfaceType);
        PyModule_AddObject(m, "Topic", (PyObject*)&CFE_MissionLib_Python_TopicType);
    }
    while(0);

    return m;
}
