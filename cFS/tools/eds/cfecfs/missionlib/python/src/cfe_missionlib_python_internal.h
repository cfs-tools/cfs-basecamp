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

#ifndef CFE_MISSIONLIB_PYTHON_INTERNAL_H
#define CFE_MISSIONLIB_PYTHON_INTERNAL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include "cfe_missionlib_python.h"
#include "edslib_intfdb.h"
#include "edslib_binding_objects.h"
#include "edslib_python_internal.h"

typedef struct
{
    PyObject_HEAD void                           *dl;
    const CFE_MissionLib_SoftwareBus_Interface_t *IntfDb;
    PyObject                                     *DbName;
    PyObject                                     *IntfCache;
    PyObject                                     *WeakRefList;
} CFE_MissionLib_Python_Database_t;

/* An "Interface" here is analogous to a declared intf in EDS.  In the software bus realm there
 * are two - Telecommand and Telemetry, differing only in the direction of data flow */
typedef struct
{
    PyObject_HEAD CFE_MissionLib_Python_Database_t *DbObj;
    PyObject                                       *IntfName;
    EdsLib_Id_t                                     DeclEdsId;
    EdsLib_Id_t                                     CmdEdsId;
    PyObject                                       *TopicCache;
    PyObject                                       *WeakRefList;
} CFE_MissionLib_Python_Interface_t;

typedef struct
{
    PyObject_HEAD CFE_MissionLib_Python_Interface_t *IntfObj;
    PyObject                                        *TopicName;
    uint16_t                                         TopicId;

    CFE_MissionLib_TopicInfo_t TopicInfo;

    EdsLib_Id_t DataEdsId; /* The actual data type for messages on this topic */
    PyObject   *WeakRefList;
} CFE_MissionLib_Python_Topic_t;

typedef struct
{
    PyObject_HEAD uint16_t Index;
    PyObject              *refobj;
} CFE_MissionLib_Python_InstanceIterator_t;

typedef struct
{
    PyObject_HEAD uint16_t TopicId;
    PyObject              *refobj;
} CFE_MissionLib_Python_InterfaceIterator_t;

typedef struct
{
    PyObject_HEAD EdsLib_Id_t Index;
    PyObject                 *refobj;
} CFE_MissionLib_Python_TopicIterator_t;

typedef int32_t (*EdsLib_NameLookupFunc_t)(const EdsLib_DatabaseObject_t *, const char *, EdsLib_Id_t *);

EdsLib_Id_t
CFE_MissionLib_Python_ConvertArgToEdsId(EdsLib_NameLookupFunc_t Func, const EdsLib_DatabaseObject_t *GD, PyObject *arg);

PyObject *CFE_MissionLib_Python_GetFromCache(PyObject *cachedict, PyObject *idxval, PyTypeObject *reqtype);
int       CFE_MissionLib_Python_SaveToCache(PyObject *cachedict, PyObject *idxval, PyObject *obj);

PyObject *CFE_MissionLib_Python_Interface_GetFromIntfName(CFE_MissionLib_Python_Database_t *obj,
                                                          PyObject                         *InterfaceName);
PyObject *CFE_MissionLib_Python_Topic_GetFromTopicName(CFE_MissionLib_Python_Interface_t *obj, PyObject *TopicName);

extern PyObject *CFE_MissionLib_Python_DatabaseCache;

extern PyTypeObject CFE_MissionLib_Python_DatabaseType;
extern PyTypeObject CFE_MissionLib_Python_InterfaceType;
extern PyTypeObject CFE_MissionLib_Python_TopicType;

#endif /* _CFE_MISSIONLIB_PYTHON_INTERNAL_H_ */
