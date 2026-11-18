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
** File:  cfe_missionlib_python_interface.c
**
** Created on: Feb 11, 2020
** Author: mathew.j.mccaskey@nasa.gov
**
** Purpose:
**   Implement the "interface" type
**
**   This is a datatype which refers back to an interface in a CFE Interface database.
**
******************************************************************************/

#include "cfe_missionlib_python_internal.h"
#include <dlfcn.h>
#include <structmember.h>
#include "cfe_mission_eds_designparameters.h"
#include "cfe_mission_eds_interface_parameters.h"

static void      CFE_MissionLib_Python_Interface_dealloc(PyObject *obj);
static PyObject *CFE_MissionLib_Python_Interface_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *CFE_MissionLib_Python_Interface_gettopic(PyObject *obj, PyObject *arg);
static PyObject *CFE_MissionLib_Python_Interface_repr(PyObject *obj);
static int       CFE_MissionLib_Python_Interface_traverse(PyObject *obj, visitproc visit, void *arg);
static int       CFE_MissionLib_Python_Interface_clear(PyObject *obj);
static PyObject *CFE_MissionLib_Python_Interface_GetCmdMessage(PyObject *obj, PyObject *args);

static PyObject *CFE_MissionLib_Python_Interface_iter(PyObject *obj);
static void      CFE_MissionLib_Python_InterfaceIterator_dealloc(PyObject *obj);
static int       CFE_MissionLib_Python_InterfaceIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int       CFE_MissionLib_Python_InterfaceIterator_clear(PyObject *obj);
static PyObject *CFE_MissionLib_Python_InterfaceIterator_iternext(PyObject *obj);

static CFE_MissionLib_Python_Interface_t *
CFE_MissionLib_Python_Interface_GetFromDeclEdsId_Impl(PyTypeObject *obj, PyObject *dbobj, EdsLib_Id_t DeclEdsId);

static PyMethodDef CFE_MissionLib_Python_Interface_methods[] = {
    { "Topic", CFE_MissionLib_Python_Interface_gettopic, METH_O, "Lookup a Topic from an Interface." },
    { "GetCmdMessage",
     CFE_MissionLib_Python_Interface_GetCmdMessage, METH_VARARGS,
     "Get a CFE command message EDS Object from a Topic" },
    { NULL }  /* Sentinel */
};

static struct PyMemberDef CFE_MissionLib_Python_Interface_members[] = {
    { "Name", T_OBJECT_EX, offsetof(CFE_MissionLib_Python_Interface_t, IntfName), READONLY, "Interface Name" },
    { NULL }  /* Sentinel */
};

PyTypeObject CFE_MissionLib_Python_InterfaceType = { PyVarObject_HEAD_INIT(NULL, 0).tp_name =
                                                         CFE_MISSIONLIB_PYTHON_ENTITY_NAME("Interface"),
                                                     .tp_basicsize = sizeof(CFE_MissionLib_Python_Interface_t),
                                                     .tp_dealloc   = CFE_MissionLib_Python_Interface_dealloc,
                                                     .tp_new       = CFE_MissionLib_Python_Interface_new,
                                                     .tp_methods   = CFE_MissionLib_Python_Interface_methods,
                                                     .tp_members   = CFE_MissionLib_Python_Interface_members,
                                                     .tp_repr      = CFE_MissionLib_Python_Interface_repr,
                                                     .tp_traverse  = CFE_MissionLib_Python_Interface_traverse,
                                                     .tp_clear     = CFE_MissionLib_Python_Interface_clear,
                                                     .tp_iter      = CFE_MissionLib_Python_Interface_iter,
                                                     .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
                                                     .tp_weaklistoffset =
                                                         offsetof(CFE_MissionLib_Python_Interface_t, WeakRefList),
                                                     .tp_doc = PyDoc_STR("Interface database entry") };

PyTypeObject CFE_MissionLib_Python_InterfaceIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("InterfaceIterator"),
    .tp_basicsize                          = sizeof(CFE_MissionLib_Python_InterfaceIterator_t),
    .tp_dealloc                            = CFE_MissionLib_Python_InterfaceIterator_dealloc,
    .tp_getattro                           = PyObject_GenericGetAttr,
    .tp_flags                              = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse                           = CFE_MissionLib_Python_InterfaceIterator_traverse,
    .tp_clear                              = CFE_MissionLib_Python_InterfaceIterator_clear,
    .tp_iter                               = PyObject_SelfIter,
    .tp_iternext                           = CFE_MissionLib_Python_InterfaceIterator_iternext,
    .tp_doc                                = PyDoc_STR("CFE MissionLib InterfaceIteratorType")
};

static int CFE_MissionLib_Python_Interface_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_Interface_t *self = (CFE_MissionLib_Python_Interface_t *)obj;
    Py_VISIT(self->DbObj);
    Py_VISIT(self->IntfName);
    Py_VISIT(self->TopicCache);
    return 0;
}

static int CFE_MissionLib_Python_Interface_clear(PyObject *obj)
{
    CFE_MissionLib_Python_Interface_t *self = (CFE_MissionLib_Python_Interface_t *)obj;
    Py_CLEAR(self->DbObj);
    Py_CLEAR(self->IntfName);
    Py_CLEAR(self->TopicCache);
    return 0;
}

static void CFE_MissionLib_Python_Interface_dealloc(PyObject *obj)
{
    CFE_MissionLib_Python_Interface_t *self = (CFE_MissionLib_Python_Interface_t *)obj;
    Py_CLEAR(self->DbObj);
    Py_CLEAR(self->IntfName);
    Py_CLEAR(self->TopicCache);

    if (self->WeakRefList != NULL)
    {
        PyObject_ClearWeakRefs(obj);
        self->WeakRefList = NULL;
    }

    CFE_MissionLib_Python_InterfaceType.tp_base->tp_dealloc(obj);
}

static CFE_MissionLib_Python_Interface_t *
CFE_MissionLib_Python_Interface_GetFromDeclEdsId_Impl(PyTypeObject *obj, PyObject *dbobj, EdsLib_Id_t DeclEdsId)
{
    CFE_MissionLib_Python_Database_t  *DbObj = (CFE_MissionLib_Python_Database_t *)dbobj;
    CFE_MissionLib_Python_Interface_t *self  = NULL;
    char                               Buffer[128];
    int32_t                            Status;
    PyObject                          *DeclEdsIdVal;
    bool                               IsSuccess;

    IsSuccess = false;
    do
    {
        DeclEdsIdVal = PyLong_FromUnsignedLong(DeclEdsId);
        if (DeclEdsIdVal == NULL)
        {
            break;
        }

        self = (CFE_MissionLib_Python_Interface_t *)
            CFE_MissionLib_Python_GetFromCache(DbObj->IntfCache, DeclEdsIdVal, &CFE_MissionLib_Python_InterfaceType);
        if (self != NULL)
        {
            if (EdsLib_Is_Match(DeclEdsId, self->DeclEdsId))
            {
                IsSuccess = true;
                break;
            }

            /* not a match */
            Py_DECREF(self);
            self = NULL;
        }

        self = (CFE_MissionLib_Python_Interface_t *)obj->tp_alloc(obj, 0);
        if (self == NULL)
        {
            break;
        }

        Py_INCREF(DbObj);
        self->DbObj     = DbObj;
        self->DeclEdsId = DeclEdsId;

        self->TopicCache = PyDict_New();
        if (self->TopicCache == NULL)
        {
            break;
        }

        Status = EdsLib_IntfDB_GetFullName(CFE_MissionLib_GetParent(DbObj->IntfDb), DeclEdsId, Buffer, sizeof(Buffer));
        if (Status != EDSLIB_SUCCESS)
        {
            PyErr_Format(PyExc_RuntimeError, "Unable to get name of intf ID: %lx", (unsigned long)DeclEdsId);
            break;
        }

        self->IntfName = PyUnicode_FromFormat("%s", Buffer);
        if (self->IntfName == NULL)
        {
            break;
        }

        /* All SB interfaces should have a single command on it */
        Status = EdsLib_IntfDB_FindAllCommands(CFE_MissionLib_GetParent(DbObj->IntfDb), DeclEdsId, &self->CmdEdsId, 1);
        if (Status != EDSLIB_SUCCESS)
        {
            PyErr_Format(PyExc_RuntimeError, "Unable to enumerate commands on intf ID: %lx", (unsigned long)DeclEdsId);
            break;
        }

        /* Create a weak reference to store in the local cache in case this
         * database is constructed again. */
        if (CFE_MissionLib_Python_SaveToCache(DbObj->IntfCache, DeclEdsIdVal, (PyObject *)self) < 0)
        {
            /* if something went wrong this raises an error and must return NULL */
            break;
        }

        IsSuccess = true;
    } while (0);

    Py_XDECREF(DeclEdsIdVal);
    if (!IsSuccess && self != NULL)
    {
        Py_CLEAR(self->IntfName);
        Py_CLEAR(self->TopicCache);
        Py_DECREF(self);
        self = NULL;
    }

    return self;
}

static PyObject *CFE_MissionLib_Python_Interface_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject                         *arg1;
    PyObject                         *arg2;
    PyObject                         *arg3  = NULL;
    CFE_MissionLib_Python_Database_t *DbObj = NULL;

    EdsLib_Id_t DeclEdsId = EDSLIB_ID_INVALID;

    PyObject *tempargs = NULL;
    PyObject *result   = NULL;

    /*
     * Interface entries are constructed from two values:
     *   - An Interface Database
     *   - An Interface Identifier
     *   - An EdsDb Python Object (only needed if a string is used as an Interface Database Identifier)
     *
     * Databases are natively of the CFE_MissionLib_Python_Database_t type
     * Interface Identifiers are indices which are natively unsigned integers (EdsLib_Id_t specifically)
     *
     * However, either value can alternatively be supplied as a string,
     * in which case it can be looked up and converted to the native value.
     */
    if (!PyArg_UnpackTuple(args, "Interface_new", 2, 3, &arg1, &arg2, &arg3))
    {
        PyErr_Format(PyExc_RuntimeError,
                     "Interface arguments expected: Interface Database, Interface Identifier, and EdsDb Python Object "
                     "(optional)");
        return NULL;
    }

    do
    {
        // Set up the Interface Database Python Object
        if (Py_TYPE(arg1) == &CFE_MissionLib_Python_DatabaseType)
        {
            DbObj = (CFE_MissionLib_Python_Database_t *)arg1;
            Py_INCREF(DbObj);
        }
        else
        {
            if (arg3 != NULL)
            {
                tempargs = PyTuple_Pack(2, arg1, arg3);
            }
            else
            {
                tempargs = PyTuple_Pack(2, arg1, Py_None);
            }

            if (tempargs == NULL)
            {
                break;
            }
            DbObj = (CFE_MissionLib_Python_Database_t *)PyObject_Call((PyObject *)&CFE_MissionLib_Python_DatabaseType,
                                                                      tempargs,
                                                                      NULL);
            Py_DECREF(tempargs);
            tempargs = NULL;
        }

        if (DbObj == NULL)
        {
            break;
        }

        // Set up the Interface Python Object from an Interface Id or Interface Name
        DeclEdsId = CFE_MissionLib_Python_ConvertArgToEdsId(EdsLib_IntfDB_FindDeclaredInterfaceByFullName,
                                                            CFE_MissionLib_GetParent(DbObj->IntfDb),
                                                            arg2);

        /* if the lookup failed it should have already set an exception */
        if (EdsLib_Is_Valid(DeclEdsId))
        {
            result =
                (PyObject *)CFE_MissionLib_Python_Interface_GetFromDeclEdsId_Impl(&CFE_MissionLib_Python_InterfaceType,
                                                                                  (PyObject *)DbObj,
                                                                                  DeclEdsId);
        }
    } while (0);

    /* decrement refcount for all temporary objects created */
    Py_XDECREF(DbObj);
    Py_XDECREF(tempargs);

    return result;
}

PyObject *CFE_MissionLib_Python_Interface_repr(PyObject *obj)
{
    CFE_MissionLib_Python_Interface_t *self = (CFE_MissionLib_Python_Interface_t *)obj;
    return PyUnicode_FromFormat("%s(%R,%R)", obj->ob_type->tp_name, self->DbObj->DbName, self->IntfName);
}

static PyObject *CFE_MissionLib_Python_Interface_gettopic(PyObject *obj, PyObject *arg)
{
    CFE_MissionLib_Python_Interface_t *IntfObj  = (CFE_MissionLib_Python_Interface_t *)obj;
    PyObject                          *tempargs = NULL;
    PyObject                          *result   = NULL;

    tempargs = PyTuple_Pack(3, IntfObj->DbObj, IntfObj, arg);
    if (tempargs == NULL)
    {
        return NULL;
    }
    result = PyObject_Call((PyObject *)&CFE_MissionLib_Python_TopicType, tempargs, NULL);

    Py_DECREF(tempargs);
    return result;
}

PyObject *CFE_MissionLib_Python_Interface_GetFromIntfName(CFE_MissionLib_Python_Database_t *obj,
                                                          PyObject                         *InterfaceName)
{
    int32_t     status;
    EdsLib_Id_t DeclEdsId;

    status = EdsLib_IntfDB_FindDeclaredInterfaceByFullName(CFE_MissionLib_GetParent(obj->IntfDb),
                                                           PyBytes_AsString(InterfaceName),
                                                           &DeclEdsId);
    if (status != EDSLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "Interface %s undefined", PyBytes_AsString(InterfaceName));
        return NULL;
    }

    return (PyObject *)CFE_MissionLib_Python_Interface_GetFromDeclEdsId_Impl(&CFE_MissionLib_Python_InterfaceType,
                                                                             (PyObject *)obj,
                                                                             DeclEdsId);
}

static PyObject *CFE_MissionLib_Python_Interface_iter(PyObject *obj)
{
    CFE_MissionLib_Python_InterfaceIterator_t *IntfIter;
    PyObject                                  *result = NULL;

    IntfIter = PyObject_GC_New(CFE_MissionLib_Python_InterfaceIterator_t, &CFE_MissionLib_Python_InterfaceIteratorType);

    if (IntfIter == NULL)
    {
        return NULL;
    }

    IntfIter->TopicId = 0;

    Py_INCREF(obj);
    IntfIter->refobj = obj;

    result = (PyObject *)IntfIter;
    PyObject_GC_Track(result);

    return result;
}

static void CFE_MissionLib_Python_InterfaceIterator_dealloc(PyObject *obj)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t *)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int CFE_MissionLib_Python_InterfaceIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t *)obj;
    Py_VISIT(self->refobj);
    return 0;
}

static int CFE_MissionLib_Python_InterfaceIterator_clear(PyObject *obj)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t *)obj;
    Py_CLEAR(self->refobj);
    return 0;
}

static PyObject *CFE_MissionLib_Python_InterfaceIterator_iternext(PyObject *obj)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t *)obj;
    CFE_MissionLib_Python_Interface_t         *intf = NULL;
    char                                       Buffer[128];
    CFE_MissionLib_TopicInfo_t                 TopicInfo;
    EdsLib_IntfDB_InterfaceInfo_t              IntfInfo;
    int32_t                                    Status;
    PyObject                                  *key     = NULL;
    PyObject                                  *topicid = NULL;
    PyObject                                  *result  = NULL;

    if (self->refobj == NULL)
    {
        return NULL;
    }

    intf = (CFE_MissionLib_Python_Interface_t *)self->refobj;

    memset(&TopicInfo, 0, sizeof(TopicInfo));

    do
    {
        /* This function will return a unique error code if the topic ID is beyond the valid range */
        /* it returns success on unused topic IDs, but the ParentInfId will be invalid if unused */
        ++self->TopicId;
        Status = CFE_MissionLib_GetTopicInfo(intf->DbObj->IntfDb, self->TopicId, &TopicInfo);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            break;
        }

        Status = EdsLib_IntfDB_GetComponentInterfaceInfo(CFE_MissionLib_GetParent(intf->DbObj->IntfDb),
                                                         TopicInfo.ParentIntfId,
                                                         &IntfInfo);
        if (Status == EDSLIB_SUCCESS)
        {
            /* Only look at entries associated with this interface (Bypass topics from other interfaces) */
            if (IntfInfo.IntfTypeEdsId != intf->DeclEdsId)
            {
                Status = EDSLIB_NO_MATCHING_VALUE;
            }
            else
            {
                /* This fails quickly if passed an invalid identifier - it will not assemble the string */
                Status = EdsLib_IntfDB_GetFullName(CFE_MissionLib_GetParent(intf->DbObj->IntfDb),
                                                   TopicInfo.ParentIntfId,
                                                   Buffer,
                                                   sizeof(Buffer));
            }
        }

    } while (Status != EDSLIB_SUCCESS);

    if (Status == EDSLIB_SUCCESS)
    {
        key = PyUnicode_FromString(Buffer);

        if (key == NULL)
        {
            /* end */
            Py_CLEAR(self->refobj);
        }
        else
        {
            topicid = PyLong_FromLong(self->TopicId);
            result  = PyTuple_Pack(2, key, topicid);
        }
    }

    Py_XDECREF(key);
    Py_XDECREF(topicid);

    return result;
}

static PyObject *CFE_MissionLib_Python_Interface_GetCmdMessage(PyObject *obj, PyObject *args)
{
    return PyUnicode_FromFormat("Interface_GetCmdMessage still needs to be implemented");
}

EdsLib_Id_t
CFE_MissionLib_Python_ConvertArgToEdsId(EdsLib_NameLookupFunc_t Func, const EdsLib_DatabaseObject_t *GD, PyObject *arg)
{
    EdsLib_Id_t Result;
    int32_t     Status;
    PyObject   *temp_id;

    temp_id = NULL;
    Result  = EDSLIB_ID_INVALID;

    /*
     * The identifier might come from the python interpreter as a string or number.
     *  - If it is a number, treat it as an EdsId number (typecast it, basically)
     *  - If it is a string, it could be unicode or some other nature of bytes object.
     *
     * In the latter case we must look up the name in EdsLib, but the names in EdsLib
     * are only strict ASCII.  (allowing unicode chars might be a future enhancement).
     * Thus we must make sure that the name is only ASCII before calling the API.
     */
    if (PyNumber_Check(arg))
    {
        /* Already numeric, assume it is an ID number */
        temp_id = PyNumber_Long(arg);
        if (temp_id != NULL)
        {
            Result = PyLong_AsUnsignedLong(temp_id);
        }
    }
    else
    {
        /* Do a lookup but make sure the string is plain ASCII */
        if (PyUnicode_Check(arg))
        {
            temp_id = PyUnicode_AsASCIIString(arg);
        }
        else
        {
            temp_id = PyObject_ASCII(arg);
        }

        if (temp_id != NULL)
        {
            Status = Func(GD, PyBytes_AsString(temp_id), &Result);
            if (Status != EDSLIB_SUCCESS)
            {
                PyErr_Format(PyExc_ValueError,
                             "Name lookup on \'%s\' failed with code=%d",
                             PyBytes_AsString(temp_id),
                             (int)Status);
                Result = EDSLIB_ID_INVALID;
            }
        }
    }

    Py_XDECREF(temp_id);

    return Result;
}
