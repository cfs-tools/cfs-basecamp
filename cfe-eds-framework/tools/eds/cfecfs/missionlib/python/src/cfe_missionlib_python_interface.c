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


PyObject *CFE_MissionLib_Python_InterfaceCache = NULL;

static void         CFE_MissionLib_Python_Interface_dealloc(PyObject * obj);
static PyObject *   CFE_MissionLib_Python_Interface_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *   CFE_MissionLib_Python_Interface_gettopic(PyObject *obj, PyObject *arg);
static PyObject *   CFE_MissionLib_Python_Interface_repr(PyObject *obj);
static int          CFE_MissionLib_Python_Interface_traverse(PyObject *obj, visitproc visit, void *arg);
static int          CFE_MissionLib_Python_Interface_clear(PyObject *obj);
static PyObject *   CFE_MissionLib_Python_Interface_GetCmdMessage(PyObject *obj, PyObject *args);

static PyObject *   CFE_MissionLib_Python_Interface_iter(PyObject *obj);
static void         CFE_MissionLib_Python_InterfaceIterator_dealloc(PyObject * obj);
static int          CFE_MissionLib_Python_InterfaceIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int          CFE_MissionLib_Python_InterfaceIterator_clear(PyObject *obj);
static PyObject *   CFE_MissionLib_Python_InterfaceIterator_iternext(PyObject *obj);

static CFE_MissionLib_Python_Interface_t *CFE_MissionLib_Python_Interface_GetFromInterfaceId_Impl(PyTypeObject *obj, PyObject *dbobj, uint16_t IntfId);

static PyMethodDef CFE_MissionLib_Python_Interface_methods[] =
{
        {"Topic",  CFE_MissionLib_Python_Interface_gettopic, METH_O, "Lookup a Topic from an Interface."},
        {"GetCmdMessage", CFE_MissionLib_Python_Interface_GetCmdMessage, METH_VARARGS, "Get a CFE command message EDS Object from a Topic"},
        {NULL}  /* Sentinel */
};

static struct PyMemberDef CFE_MissionLib_Python_Interface_members[] =
{
        {"Name", T_OBJECT_EX, offsetof(CFE_MissionLib_Python_Interface_t, IntfName), READONLY, "Interface Name" },
        {NULL}  /* Sentinel */
};


PyTypeObject CFE_MissionLib_Python_InterfaceType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("Interface"),
    .tp_basicsize = sizeof(CFE_MissionLib_Python_Interface_t),
    .tp_dealloc = CFE_MissionLib_Python_Interface_dealloc,
    .tp_new = CFE_MissionLib_Python_Interface_new,
    .tp_methods = CFE_MissionLib_Python_Interface_methods,
    .tp_members = CFE_MissionLib_Python_Interface_members,
    .tp_repr = CFE_MissionLib_Python_Interface_repr,
    .tp_traverse = CFE_MissionLib_Python_Interface_traverse,
    .tp_clear = CFE_MissionLib_Python_Interface_clear,
    .tp_iter = CFE_MissionLib_Python_Interface_iter,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,
    .tp_weaklistoffset = offsetof(CFE_MissionLib_Python_Interface_t, WeakRefList),
    .tp_doc = PyDoc_STR("Interface database entry")
};

PyTypeObject CFE_MissionLib_Python_InterfaceIteratorType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("InterfaceIterator"),
    .tp_basicsize = sizeof(CFE_MissionLib_Python_InterfaceIterator_t),
    .tp_dealloc = CFE_MissionLib_Python_InterfaceIterator_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = CFE_MissionLib_Python_InterfaceIterator_traverse,
    .tp_clear = CFE_MissionLib_Python_InterfaceIterator_clear,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = CFE_MissionLib_Python_InterfaceIterator_iternext,
    .tp_doc = PyDoc_STR("CFE MissionLib InterfaceIteratorType")
};

static int CFE_MissionLib_Python_Interface_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_Interface_t *self = (CFE_MissionLib_Python_Interface_t *)obj;
    Py_VISIT(self->DbObj);
    Py_VISIT(self->IntfName);
    Py_VISIT(self->TypeCache);

    return 0;
}

static int CFE_MissionLib_Python_Interface_clear(PyObject *obj)
{
    CFE_MissionLib_Python_Interface_t *self = (CFE_MissionLib_Python_Interface_t *)obj;
    Py_CLEAR(self->DbObj);
    Py_CLEAR(self->IntfName);
    Py_CLEAR(self->TypeCache);

    return 0;
}

static void CFE_MissionLib_Python_Interface_dealloc(PyObject * obj)
{
    CFE_MissionLib_Python_Interface_t *self = (CFE_MissionLib_Python_Interface_t *)obj;
    Py_CLEAR(self->DbObj);
    Py_CLEAR(self->IntfName);
    Py_CLEAR(self->TypeCache);

    if (self->WeakRefList != NULL)
    {
        PyObject_ClearWeakRefs(obj);
        self->WeakRefList = NULL;
    }

    CFE_MissionLib_Python_InterfaceType.tp_base->tp_dealloc(obj);
}

static CFE_MissionLib_Python_Interface_t *CFE_MissionLib_Python_Interface_GetFromInterfaceId_Impl(PyTypeObject *obj, PyObject *dbobj, uint16_t IntfId)
{
    CFE_MissionLib_Python_Database_t *DbObj = (CFE_MissionLib_Python_Database_t *)dbobj;
    CFE_MissionLib_Python_Interface_t *self;
    const char *InterfaceName = NULL;
    PyObject *IntfIdVal;
    PyObject *weakref;

    do
    {
        IntfIdVal = PyLong_FromUnsignedLong(IntfId);
        if (IntfIdVal == NULL)
        {
            break;
        }

        weakref = PyDict_GetItem(CFE_MissionLib_Python_InterfaceCache, IntfIdVal);
        if (weakref != NULL)
        {
            self = (CFE_MissionLib_Python_Interface_t *)PyWeakref_GetObject(weakref);
            if (Py_TYPE(self) == &CFE_MissionLib_Python_InterfaceType)
            {
                if (IntfId == self->InterfaceId)
                {
                    Py_INCREF(self);
                    break;
                }

                /* weakref expired, needs to be recreated */
                self = NULL;
            }
        }

        self = (CFE_MissionLib_Python_Interface_t*)obj->tp_alloc(obj, 0);
        if (self == NULL)
        {
            break;
        }

        self->TypeCache = PyDict_New();
        if (self->TypeCache == NULL)
        {
            Py_DECREF(self);
            break;
        }

        Py_INCREF(DbObj);
        self->DbObj = DbObj;
        InterfaceName = CFE_MissionLib_GetInterfaceName(DbObj->IntfDb, IntfId);
        self->IntfName = PyUnicode_FromFormat("%s",InterfaceName);
        self->InterfaceId = IntfId;
        CFE_MissionLib_GetInterfaceInfo(DbObj->IntfDb, IntfId, &self->IntfInfo);

        /* Create a weak reference to store in the local cache in case this
         * database is constructed again. */
        weakref = PyWeakref_NewRef((PyObject*)self, NULL);
        if (weakref == NULL)
        {
            Py_DECREF(self);
            break;
        }

        PyDict_SetItem(CFE_MissionLib_Python_InterfaceCache, IntfIdVal, weakref);
        Py_DECREF(weakref);
    }
    while(0);

    Py_XDECREF(IntfIdVal);
    return self;
}

//const CFE_MissionLib_InterfaceId_Entry_t *CFE_MissionLib_Python_Interface_GetEntry(PyObject *obj)
//{
//    if (obj == NULL)
//    {
//        return NULL;
//    }
//    if (!PyObject_TypeCheck(obj, &CFE_MissionLib_Python_InterfaceType))
//    {
//        PyErr_SetObject(PyExc_TypeError, obj);
//        return NULL;
//    }
//
//    return ((CFE_MissionLib_Python_Interface_t*)obj)->IntfEntry;
//}

static PyObject *CFE_MissionLib_Python_Interface_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject *arg1;
    PyObject *arg2;
    PyObject *arg3 = NULL;
    CFE_MissionLib_Python_Database_t *DbObj = NULL;

    CFE_MissionLib_InterfaceInfo_t IntfInfo;
    int32_t Status;

    uint16_t InterfaceId = 0;  // 0 is an invalid InterfaceId

    PyObject *tempargs = NULL;
    PyObject *result = NULL;

    /*
     * Interface entries are constructed from two values:
     *   - An Interface Database
     *   - An Interface Identifier
     *   - An EdsDb Python Object (only needed if a string is used as an Interface Database Identifier)
     *
     * Databases are natively of the CFE_MissionLib_Python_Database_t type
     * Interface Identifiers are indices which are natively unsigned integers (uint16_t specifically)
     *
     * However, either value can alternatively be supplied as a string,
     * in which case it can be looked up and converted to the native value.
     */
    if (!PyArg_UnpackTuple(args, "Interface_new", 2, 3, &arg1, &arg2, &arg3))
    {
        PyErr_Format(PyExc_RuntimeError, "Interface arguments expected: Interface Database, Interface Identifier, and EdsDb Python Object (optional)");
    	return NULL;
    }

    do
    {
    	// Set up the Interface Database Python Object
        if (Py_TYPE(arg1) == &CFE_MissionLib_Python_DatabaseType)
        {
            DbObj = (CFE_MissionLib_Python_Database_t*)arg1;
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
            DbObj = (CFE_MissionLib_Python_Database_t*)PyObject_Call((PyObject*)&CFE_MissionLib_Python_DatabaseType, tempargs, NULL);
            Py_DECREF(tempargs);
            tempargs = NULL;
        }

        if (DbObj == NULL)
        {
            break;
        }

        // Set up the Interface Python Object from an Interface Id or Interface Name
        if (PyNumber_Check(arg2))
        {
            tempargs = PyNumber_Long(arg2);
            if (tempargs == NULL)
            {
                break;
            }

            InterfaceId = PyLong_AsUnsignedLong(tempargs);
            Status = CFE_MissionLib_GetInterfaceInfo(DbObj->IntfDb, InterfaceId, &IntfInfo);
            if (Status != CFE_MISSIONLIB_SUCCESS)
            {
                PyErr_Format(PyExc_RuntimeError, "Invalid Interface Id: %u\tStatus: %d", InterfaceId, Status);
                break;
            }
        }
        else
        {
            if (PyUnicode_Check(arg2))
            {
                tempargs = PyUnicode_AsUTF8String(arg2);
            }
            else
            {
                tempargs = PyObject_Bytes(arg2);
            }

            if (tempargs == NULL)
            {
                break;
            }

            Status = CFE_MissionLib_FindInterfaceByName(DbObj->IntfDb, PyBytes_AsString(tempargs), &InterfaceId);

            if (Status != CFE_MISSIONLIB_SUCCESS)
            {
                PyErr_Format(PyExc_RuntimeError, "Interface %s undefined\tStatus: %d", PyBytes_AsString(tempargs), Status);
                break;
            }
        }

        result = (PyObject*)CFE_MissionLib_Python_Interface_GetFromInterfaceId_Impl(&CFE_MissionLib_Python_InterfaceType, (PyObject *)DbObj, InterfaceId);
    }
    while(0);

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
    CFE_MissionLib_Python_Interface_t *IntfObj = (CFE_MissionLib_Python_Interface_t *)obj;
    PyObject *tempargs = NULL;
    PyObject *result = NULL;

    tempargs = PyTuple_Pack(3, IntfObj->DbObj, IntfObj, arg);
    if (tempargs == NULL)
    {
        return NULL;
    }
    result = PyObject_Call((PyObject*)&CFE_MissionLib_Python_TopicType, tempargs, NULL);

    Py_DECREF(tempargs);
    return result;
}

PyObject *CFE_MissionLib_Python_Interface_GetFromIntfName(CFE_MissionLib_Python_Database_t *dbobj, PyObject *IntfName)
{
    int32_t status;
    uint16_t InterfaceId;

    status = CFE_MissionLib_FindInterfaceByName(dbobj->IntfDb, PyBytes_AsString(IntfName), &InterfaceId);

    if (status != CFE_MISSIONLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "Interface %s undefined", PyBytes_AsString(IntfName));
        return NULL;
    }

    return (PyObject*)CFE_MissionLib_Python_Interface_GetFromInterfaceId_Impl(&CFE_MissionLib_Python_InterfaceType, (PyObject *)dbobj, InterfaceId);
}

static PyObject *  CFE_MissionLib_Python_Interface_iter(PyObject *obj)
{
    CFE_MissionLib_Python_Interface_t *Intf = (CFE_MissionLib_Python_Interface_t *) obj;
    CFE_MissionLib_Python_InterfaceIterator_t *IntfIter;
    PyObject *result = NULL;

    if (Intf->IntfInfo.NumTopics != 0)
    {
        IntfIter = PyObject_GC_New(CFE_MissionLib_Python_InterfaceIterator_t, &CFE_MissionLib_Python_InterfaceIteratorType);

        if (IntfIter == NULL)
        {
            return NULL;
        }

        IntfIter->Index = 1;

        Py_INCREF(obj);
        IntfIter->refobj = obj;

        result = (PyObject *)IntfIter;
        PyObject_GC_Track(result);
    }
    else
    {
        PyErr_Format(PyExc_RuntimeError, "Not an iterable interface");
    }
    return result;
}

static void CFE_MissionLib_Python_InterfaceIterator_dealloc(PyObject * obj)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t*)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int CFE_MissionLib_Python_InterfaceIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t*)obj;
    Py_VISIT(self->refobj);
    return 0;
}

static int CFE_MissionLib_Python_InterfaceIterator_clear(PyObject *obj)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t*)obj;
    Py_CLEAR(self->refobj);
    return 0;
}

static PyObject *CFE_MissionLib_Python_InterfaceIterator_iternext(PyObject *obj)
{
    CFE_MissionLib_Python_InterfaceIterator_t *self = (CFE_MissionLib_Python_InterfaceIterator_t*)obj;
    CFE_MissionLib_Python_Interface_t *intf = NULL;
    const char * Label = NULL;
    uint16_t idx;
    PyObject *key = NULL;
    PyObject *topicid = NULL;
    PyObject *result = NULL;

    do
    {
    	if (self->refobj == NULL)
        {
            break;
        }

        intf = (CFE_MissionLib_Python_Interface_t *)self->refobj;
        idx = self->Index;

        do
        {
            Label = CFE_MissionLib_GetTopicName(intf->DbObj->IntfDb, intf->InterfaceId, idx);
            ++idx;
        }
        while((Label == NULL) && (idx <= intf->IntfInfo.NumTopics+1));

        if (idx <= intf->IntfInfo.NumTopics+1)
        {
            key = PyUnicode_FromString(Label);

            if (key == NULL)
            {
            	/* end */
            	Py_CLEAR(self->refobj);
            	break;
            }

            topicid = PyLong_FromLong(idx-1);

            self->Index = idx;
            result = PyTuple_Pack(2, key, topicid);
        }
    }
    while(0);

    Py_XDECREF(key);
    Py_XDECREF(topicid);

    return result;
}

static PyObject *CFE_MissionLib_Python_Interface_GetCmdMessage(PyObject *obj, PyObject *args)
{
    return PyUnicode_FromFormat("Interface_GetCmdMessage still needs to be implemented");
}
