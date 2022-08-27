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
** File:  cfe_missionlib_python_internal.h
**
** Created on: Feb 11, 2020
** Author: mathew.j.mccaskey
**
** Purpose:
**      Internal Header file for Python / cfe_missionlib bindings
**
******************************************************************************/


#ifndef _CFE_MISSIONLIB_PYTHON_INTERNAL_H_
#define _CFE_MISSIONLIB_PYTHON_INTERNAL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include "cfe_missionlib_python.h"
#include "cfe_missionlib_database_types.h"
#include "edslib_binding_objects.h"
#include "edslib_python_internal.h"

typedef struct
{
    PyObject_HEAD
    void *dl;
    const CFE_MissionLib_SoftwareBus_Interface_t *IntfDb;
    EdsLib_Python_Database_t *EdsDbObj;
    PyObject *DbName;
    /* TypeCache contains weak references to Databases, such that they
     * will not be re-created each time they are required.  This also gives persistence,
     * i.e. repeated calls to lookup the same type give the same object, instead of a
     * separate-but-equal object. */
    PyObject *TypeCache;
    PyObject *WeakRefList;
} CFE_MissionLib_Python_Database_t;

typedef struct
{
    PyObject_HEAD
    CFE_MissionLib_Python_Database_t *DbObj;
    PyObject *IntfName;
    uint16_t InterfaceId;
    CFE_MissionLib_InterfaceInfo_t IntfInfo;

    /* TypeCache contains weak references to Interfaces in the db, such that they
     * will not be re-created each time they are required.  This also gives persistence,
     * i.e. repeated calls to lookup the same type give the same object, instead of a
     * separate-but-equal object. */
    PyObject *TypeCache;
    PyObject *WeakRefList;
} CFE_MissionLib_Python_Interface_t;

typedef struct
{
    PyObject_HEAD
    CFE_MissionLib_Python_Interface_t *IntfObj;
    PyObject *TopicName;
    uint16_t TopicId;
    EdsLib_Id_t EdsId;
    CFE_MissionLib_IndicationInfo_t IndInfo;

    /* TypeCache contains weak references to Topics in the db, such that they
     * will not be re-created each time they are required.  This also gives persistence,
     * i.e. repeated calls to lookup the same type give the same object, instead of a
     * separate-but-equal object. */
    PyObject *TypeCache;
    PyObject *WeakRefList;
} CFE_MissionLib_Python_Topic_t;

typedef struct
{
	PyObject_HEAD
	uint16_t Index;
	PyObject* refobj;
} CFE_MissionLib_Python_InstanceIterator_t;

typedef struct
{
	PyObject_HEAD
	uint16_t Index;
	PyObject* refobj;
} CFE_MissionLib_Python_InterfaceIterator_t;

typedef struct
{
	PyObject_HEAD
	EdsLib_Id_t Index;
	PyObject* refobj;
} CFE_MissionLib_Python_TopicIterator_t;

const CFE_MissionLib_SoftwareBus_Interface_t *CFE_MissionLib_Python_Database_GetDB(PyObject *obj);
//const CFE_MissionLib_InterfaceId_Entry_t *CFE_MissionLib_Python_Interface_GetEntry(PyObject *obj);
//const CFE_MissionLib_TopicId_Entry_t *CFE_MissionLib_Python_Topic_GetEntry(PyObject *obj);

PyObject *CFE_MissionLib_Python_Interface_GetFromIntfName(CFE_MissionLib_Python_Database_t *obj, PyObject *InterfaceName);
PyObject *CFE_MissionLib_Python_Topic_GetFromTopicName(CFE_MissionLib_Python_Interface_t *obj, PyObject *TopicName);

PyObject *CFE_MissionLib_Python_Database_CreateFromStaticDB(const char *Name, const CFE_MissionLib_SoftwareBus_Interface_t *IntfDb);

extern PyObject *CFE_MissionLib_Python_DatabaseCache;
extern PyObject *CFE_MissionLib_Python_InterfaceCache;
extern PyObject *CFE_MissionLib_Python_TopicCache;

extern PyTypeObject CFE_MissionLib_Python_DatabaseType;
extern PyTypeObject CFE_MissionLib_Python_InterfaceType;
extern PyTypeObject CFE_MissionLib_Python_TopicType;

#endif /* _CFE_MISSIONLIB_PYTHON_INTERNAL_H_ */
