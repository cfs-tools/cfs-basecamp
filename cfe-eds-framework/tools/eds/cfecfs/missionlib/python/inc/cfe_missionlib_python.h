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
** File:  cfe_missionlib_python_module.h
**
** Created on: Feb 11, 2020
** Author: mathew.j.mccaskey@nasa.gov
**
** Purpose:
**      Header file for Python / MissionLib bindings
**
******************************************************************************/

#ifndef TOOLS_EDS_CFECFS_MISSIONLIB_PYTHON_INC_CFE_MISSIONLIB_PYTHON_H_
#define TOOLS_EDS_CFECFS_MISSIONLIB_PYTHON_INC_CFE_MISSIONLIB_PYTHON_H_

#include <Python.h>
#include "cfe_missionlib_api.h"
//#include <edslib_api_types.h>
//#include <edslib_id.h>

/**
 * Documentation string for this Python module
 *
 * This is provided for applications that supply their own custom init routine.
 */
#define CFE_MISSIONLIB_PYTHON_DOC            "Module which provides an interface to the CFE_MissionLib Runtime Library."

/**
 * Base Name of the Python module
 *
 * Applications should use this name if registering a custom inittab.
 * It needs to be consistent because all module-supplied types use this
 * as the base name, which carries through to the respective "repr()"
 * implementation and other user-visible items.
 */
#define CFE_MISSIONLIB_PYTHON_MODULE_NAME    "CFE_MissionLib"

/**
 * Get the name of a Python entity
 *
 * This macro converts an unqualified name to a qualified name,
 * by adding a prefix of CFE_MISSIONLIB_PYTHON_MODULE_NAME.  It is used
 * by all types to keep the naming consistent.
 */
#define CFE_MISSIONLIB_PYTHON_ENTITY_NAME(x)   CFE_MISSIONLIB_PYTHON_MODULE_NAME "." x


/**
 * Main Initializer function that sets up a newly-minted module object
 *
 * This calls PyType_Ready() for all the Python type objects defined in
 * this module, and adds the required members to the module, and returns
 * the new module to the caller.
 *
 * This is almost (but not quite) usable as a Python module init function.
 * Depending on the environment, additional members may need to be added,
 * and the API/name might need to be adjusted (Python2 and Python3 have
 * different naming and calling conventions).  So it is expected that
 * an additional wrapper around this will be added to accommodate this.
 */
PyObject* CFE_MissionLib_Python_CreateModule(void);

/**
 * Creates a Python SoftwareBus Interface Database object from a C Database Object
 *
 * This would typically be used when the DB object is statically linked
 * into the C environment, and should be exposed to Python as a built-in.
 */
PyObject *CFE_MissionLib_Python_Database_CreateFromStaticDB(const char *Name, const CFE_MissionLib_SoftwareBus_Interface_t *Intf);

/**
 * Gets the C EDS Database object from a Python MissionLib Database Object
 *
 * This should be used when calling the MissionLib API from Python.
 */
const CFE_MissionLib_SoftwareBus_Interface_t *CFE_MissionLib_Python_Database_GetDB(PyObject *obj);

/**
 * Gets a topic entry from a Python CFE MissionLib Interface Database object using a C Msg_Id_t value
 *
 * This is a convenience wrapper that performs type checking and conversions
 */
//PyTypeObject *CFE_MissionLib_Python_TopicEntry_GetFromMsgId(PyObject *dbobj, const CFE_SB_SoftwareBus_PubSub_Interface_t Params);

#endif /* TOOLS_EDS_CFECFS_MISSIONLIB_PYTHON_INC_CFE_MISSIONLIB_PYTHON_H_ */
