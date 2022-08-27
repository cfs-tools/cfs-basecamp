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
** File:  cfe_missionlib_python_database.c
**
** Created on: Feb 11, 2020
** Author: mathew.j.mccaskey@nasa.gov
**
** Purpose:
**   Implement python type which represents an CFE_MissionLib database
**
**   Allows python code to dynamically load a database object and retrieve
**   entries from it.
**
******************************************************************************/

#include "cfe_missionlib_python_internal.h"
#include <dlfcn.h>
#include <structmember.h>
#include "cfe_missionlib_runtime.h"

#include "cfe_mission_eds_interface_parameters.h"
#include "ccsds_spacepacket_eds_typedefs.h"
#include "ccsds_spacepacket_eds_defines.h"
#include "cfe_sb_eds_typedefs.h"
#include "edslib_binding_objects.h"

PyObject *CFE_MissionLib_Python_DatabaseCache = NULL;

static void         CFE_MissionLib_Python_Database_dealloc(PyObject * obj);
static PyObject *   CFE_MissionLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *   CFE_MissionLib_Python_Database_repr(PyObject *obj);
static int          CFE_MissionLib_Python_Database_traverse(PyObject *obj, visitproc visit, void *arg);
static int          CFE_MissionLib_Python_Database_clear(PyObject *obj);

static PyObject *   CFE_MissionLib_Python_Database_GetInterface(PyObject *obj, PyObject *key);
static PyObject *   CFE_MissionLib_Python_DecodeEdsId(PyObject *obj, PyObject *args);
static PyObject *   CFE_MissionLib_Python_Set_PubSub(PyObject *obj, PyObject *args);

static PyObject *   CFE_MissionLib_Python_Instance_iter(PyObject *obj);
static void         CFE_MissionLib_Python_InstanceIterator_dealloc(PyObject * obj);
static int          CFE_MissionLib_Python_InstanceIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int          CFE_MissionLib_Python_InstanceIterator_clear(PyObject *obj);
static PyObject *   CFE_MissionLib_Python_InstanceIterator_iternext(PyObject *obj);

static PyMethodDef CFE_MissionLib_Python_Database_methods[] =
{
        {"Interface",  CFE_MissionLib_Python_Database_GetInterface, METH_O, "Lookup an Interface type from DB."},
        {"DecodeEdsId", CFE_MissionLib_Python_DecodeEdsId, METH_VARARGS, "Decode the EdsID from a packed cFE message"},
        {"SetPubSub", CFE_MissionLib_Python_Set_PubSub, METH_VARARGS, "Set the PubSub parameters for a command message"},
        {NULL}  /* Sentinel */
};

static struct PyMemberDef CFE_MissionLib_Python_Database_members[] =
{
        {"Name", T_OBJECT_EX, offsetof(CFE_MissionLib_Python_Database_t, DbName), READONLY, "Database Name" },
        {NULL}  /* Sentinel */
};


PyTypeObject CFE_MissionLib_Python_DatabaseType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("Database"),
    .tp_basicsize = sizeof(CFE_MissionLib_Python_Database_t),
    .tp_dealloc = CFE_MissionLib_Python_Database_dealloc,
    .tp_new = CFE_MissionLib_Python_Database_new,
    .tp_methods = CFE_MissionLib_Python_Database_methods,
    .tp_members = CFE_MissionLib_Python_Database_members,
    .tp_repr = CFE_MissionLib_Python_Database_repr,
    .tp_traverse = CFE_MissionLib_Python_Database_traverse,
    .tp_clear = CFE_MissionLib_Python_Database_clear,
    .tp_iter = CFE_MissionLib_Python_Instance_iter,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,
    .tp_weaklistoffset = offsetof(CFE_MissionLib_Python_Database_t, WeakRefList),
    .tp_doc = PyDoc_STR("Interface database")
};

PyTypeObject CFE_MissionLib_Python_InstanceIteratorType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("InstanceIterator"),
    .tp_basicsize = sizeof(CFE_MissionLib_Python_InstanceIterator_t),
    .tp_dealloc = CFE_MissionLib_Python_InstanceIterator_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = CFE_MissionLib_Python_InstanceIterator_traverse,
    .tp_clear = CFE_MissionLib_Python_InstanceIterator_clear,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = CFE_MissionLib_Python_InstanceIterator_iternext,
    .tp_doc = PyDoc_STR("CFE MissionLib InstanceIteratorType")
};

static int CFE_MissionLib_Python_Database_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;
    Py_VISIT(self->DbName);
    Py_VISIT(self->EdsDbObj);
    Py_VISIT(self->TypeCache);
    return 0;
}

static int CFE_MissionLib_Python_Database_clear(PyObject *obj)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;
    Py_CLEAR(self->DbName);
    Py_CLEAR(self->EdsDbObj);
    Py_CLEAR(self->TypeCache);
    return 0;
}

static void CFE_MissionLib_Python_Database_dealloc(PyObject * obj)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;

    Py_CLEAR(self->DbName);
    Py_CLEAR(self->EdsDbObj);
    Py_CLEAR(self->TypeCache);

    if (self->WeakRefList != NULL)
    {
        PyObject_ClearWeakRefs(obj);
        self->WeakRefList = NULL;
    }

    if (self->dl != NULL)
    {
        dlclose(self->dl);
        self->dl = NULL;
    }

    CFE_MissionLib_Python_DatabaseType.tp_base->tp_dealloc(obj);
}

static CFE_MissionLib_Python_Database_t *CFE_MissionLib_Python_Database_CreateImpl(PyTypeObject *obj, PyObject *Name, const CFE_MissionLib_SoftwareBus_Interface_t *IntfDb, PyObject *EdsDb)
{
    CFE_MissionLib_Python_Database_t *self;
    PyObject *weakref;

    self = (CFE_MissionLib_Python_Database_t*)obj->tp_alloc(obj, 0);
    if (self == NULL)
    {
        return NULL;
    }

    self->TypeCache = PyDict_New();
    if (self->TypeCache == NULL)
    {
        Py_DECREF(self);
        return NULL;
    }

  	self->IntfDb = IntfDb;
   	Py_INCREF(EdsDb);
    self->EdsDbObj = (EdsLib_Python_Database_t *)EdsDb;
    Py_INCREF(Name);
    self->DbName = Name;
    //self->NumInterfaces = IntfDb->NumInterfaces;

    /* Create a weak reference to store in the local cache in case this
     * database is constructed again. */
    weakref = PyWeakref_NewRef((PyObject*)self, NULL);
    if (weakref == NULL)
    {
        Py_DECREF(self);
        return NULL;
    }

    PyDict_SetItem(CFE_MissionLib_Python_DatabaseCache, Name, weakref);
    Py_DECREF(weakref);

    return self;
}

//PyObject *CFE_MissionLib_Python_Database_CreateFromStaticDB(const char *Name, const CFE_MissionLib_SoftwareBus_Interface_t *IntfDb)
//{
//    PyObject *pyname = PyUnicode_FromString(Name);
//    CFE_MissionLib_Python_Database_t *self;
//    PyObject *weakref;
//
//    if (pyname == NULL)
//    {
//        return NULL;
//    }
//
//    weakref = PyDict_GetItem(CFE_MissionLib_Python_DatabaseCache, pyname);
//    if (weakref != NULL)
//    {
//        self = (CFE_MissionLib_Python_Database_t *)PyWeakref_GetObject(weakref);
//        if (Py_TYPE(self) == &CFE_MissionLib_Python_DatabaseType)
//        {
//            if (self->IntfDb != IntfDb)
//            {
//                PyErr_SetString(PyExc_RuntimeError, "Database name conflict");
//                self = NULL;
//            }
//            else
//            {
//                Py_INCREF(self);
//            }
//            Py_DECREF(pyname);
//            return (PyObject*)self;
//        }
//    }
//
//    Py_DECREF(pyname);
//    return (PyObject*)CFE_MissionLib_Python_Database_CreateImpl(&CFE_MissionLib_Python_DatabaseType, pyname, IntfDb, NULL);
//}

const CFE_MissionLib_SoftwareBus_Interface_t *CFE_MissionLib_Python_Database_GetDB(PyObject *obj)
{
    if (obj == NULL)
    {
        return NULL;
    }
    if (!PyObject_TypeCheck(obj, &CFE_MissionLib_Python_DatabaseType))
    {
    	PyErr_SetObject(PyExc_TypeError, obj);
        return NULL;
    }

    return ((CFE_MissionLib_Python_Database_t*)obj)->IntfDb;
}

static PyObject *CFE_MissionLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject *arg1;
    PyObject *arg2;

    PyObject *tempargs;
    const char *dbstr;
    PyObject *nameobj;
    char *p;
    char tempstring[512];
    void *handle;
    void *symbol;
    const char *errstr;
    CFE_MissionLib_Python_Database_t *self;
    PyObject *weakref;

    if (!PyArg_UnpackTuple(args, "Database_new", 1, 2, &arg1, &arg2))
    {
        PyErr_Format(PyExc_RuntimeError, "Database arguments: Interface Database Identifier and EdsDb Python Object (If Database associated with Identifier has not been created");
    	return NULL;
    }

    if (PyUnicode_Check(arg1))
    {
        tempargs = PyUnicode_AsUTF8String(arg1);
    }
    else
    {
        tempargs = PyObject_Bytes(arg1);
    }
    dbstr = PyBytes_AsString(tempargs);

    /*
     * To avoid wasting resources, do not create the same database multiple times.
     * First check if the db cache object already contains an instance for this name
     * note: PyDict_GetItem returns borrowed reference
     */
    weakref = PyDict_GetItemString(CFE_MissionLib_Python_DatabaseCache, (char*)dbstr);
    if (weakref != NULL)
    {
        self = (CFE_MissionLib_Python_Database_t *)PyWeakref_GetObject(weakref);
        if (Py_TYPE(self) == &CFE_MissionLib_Python_DatabaseType)
        {
            Py_INCREF(self);
            return (PyObject*)self;
        }
    }
    else if (arg2 == Py_None)
    {
    	PyErr_Format(PyExc_RuntimeError, "EdsDb Python Object argument required");
    	return NULL;
    }

    snprintf(tempstring,sizeof(tempstring),"%s_eds_interfacedb.so", dbstr);

    /* Clear any pending dlerror value */
    dlerror();
    handle = dlopen(tempstring, RTLD_LOCAL|RTLD_NOW);
    errstr = dlerror();

    if (handle == NULL && errstr == NULL)
    {
        errstr = "Unspecified Error - NULL handle";
    }

    if (errstr != NULL)
    {
        if (handle != NULL)
        {
            dlclose(handle);
        }
        PyErr_SetString(PyExc_RuntimeError, errstr);
        return NULL;
    }

    snprintf(tempstring,sizeof(tempstring),"%s_SOFTWAREBUS_INTERFACE", dbstr);
    p = tempstring;
    while (*p != 0)
    {
        *p = toupper(*p);
        ++p;
    }

    symbol = dlsym(handle, tempstring);
    if (symbol == NULL)
    {
        dlclose(handle);
        PyErr_Format(PyExc_RuntimeError, "Symbol %s undefined", tempstring);
        return NULL;
    }

    nameobj = PyUnicode_FromString(dbstr);
    if (nameobj == NULL)
    {
        dlclose(handle);
        return NULL;
    }

    self = CFE_MissionLib_Python_Database_CreateImpl(&CFE_MissionLib_Python_DatabaseType, nameobj, symbol, arg2);
    Py_DECREF(nameobj);
    if (self == NULL)
    {
        dlclose(handle);
        return NULL;
    }

    self->dl = handle;

    return (PyObject*)self;
}

PyObject *CFE_MissionLib_Python_Database_repr(PyObject *obj)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;
    return PyUnicode_FromFormat("%s(%R)", obj->ob_type->tp_name, self->DbName);
}

static PyObject *CFE_MissionLib_Python_Database_GetInterface(PyObject *obj, PyObject *arg)
{
    PyObject *tempargs = NULL;
    PyObject *result = NULL;

    tempargs = PyTuple_Pack(2, obj, arg);
    if (tempargs == NULL)
    {
        return NULL;
    }
    result = PyObject_Call((PyObject*)&CFE_MissionLib_Python_InterfaceType, tempargs, NULL);
    Py_DECREF(tempargs);

    return result;
}

static PyObject *CFE_MissionLib_Python_DecodeEdsId(PyObject *obj, PyObject *args)
{
    CFE_MissionLib_Python_Database_t *IntfDb = (CFE_MissionLib_Python_Database_t *)obj;
    PyObject *arg1;
    EdsLib_Python_Database_t *EdsDb;

    Py_ssize_t BytesSize;
    char *NetworkBuffer;
    CCSDS_SpacePacket_Buffer_t LocalBuffer;

    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Publisher_Component_t PublisherParams;
    CFE_SB_Listener_Component_t ListenerParams;

    EdsLib_Id_t EdsId;
    uint16_t TopicId;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    int32_t Status;

    PyObject *result = NULL;

    if (!PyArg_UnpackTuple(args, "DecodeEdsId", 1, 1, &arg1))
    {
        PyErr_Format(PyExc_RuntimeError, "encoded bytes string argument expected");
        return NULL;
    }

    Py_INCREF(arg1);

    do
    {
    	EdsDb = IntfDb->EdsDbObj;

    	if (!PyBytes_Check(arg1))
    	{
    		PyErr_Format(PyExc_RuntimeError, "DecodeEdsId argument not of bytes string type");
    		break;
    	}
    	BytesSize = PyBytes_Size(arg1);
        PyBytes_AsStringAndSize(arg1, &NetworkBuffer, &BytesSize);

        EdsId = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), CCSDS_TelemetryPacket_DATADICTIONARY);
        Status = EdsLib_DataTypeDB_GetTypeInfo(EdsDb->GD, EdsId, &TypeInfo);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
    	    PyErr_Format(PyExc_RuntimeError, "Unable to get type info for CCSDS_SPACEPACKET: return status = %d", Status);
    	    break;
        }

        Status = EdsLib_DataTypeDB_UnpackPartialObject(EdsDb->GD, &EdsId,
                LocalBuffer.Byte, NetworkBuffer, sizeof(LocalBuffer), 8*TypeInfo.Size.Bytes, 0);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
    	    PyErr_Format(PyExc_RuntimeError, "Unable to unpack partial object: return status = %d", Status);
    	    break;
        }

        CFE_SB_Get_PubSub_Parameters(&PubSubParams, &LocalBuffer.BaseObject);

        if (CFE_SB_PubSub_IsPublisherComponent(&PubSubParams))
        {
            CFE_SB_UnmapPublisherComponent(&PublisherParams, &PubSubParams);
            TopicId = PublisherParams.Telemetry.TopicId;

            Status = CFE_MissionLib_GetArgumentType(IntfDb->IntfDb, CFE_SB_Telemetry_Interface_ID,
                    PublisherParams.Telemetry.TopicId, 1, 1, &EdsId);
        }
        else if (CFE_SB_PubSub_IsListenerComponent(&PubSubParams))
        {
            CFE_SB_UnmapListenerComponent(&ListenerParams, &PubSubParams);
            TopicId = ListenerParams.Telecommand.TopicId;

            Status = CFE_MissionLib_GetArgumentType(IntfDb->IntfDb, CFE_SB_Telecommand_Interface_ID,
                    ListenerParams.Telecommand.TopicId, 1, 1, &EdsId);
        }
        else
        {
            Status = CFE_MISSIONLIB_INVALID_INTERFACE;
        }

        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
    	    PyErr_Format(PyExc_RuntimeError, "Unable to get argument type: return status = %d", Status);
    	    break;
        }

        result = PyTuple_New(2);
        PyTuple_SetItem(result, 0, PyLong_FromLong((long int) EdsId));
        PyTuple_SetItem(result, 1, PyLong_FromLong((long int) TopicId));
    }
    while(0);

    Py_XDECREF(arg1);

    return result;
}

static PyObject *  CFE_MissionLib_Python_Set_PubSub(PyObject *obj, PyObject *args)
{
    PyObject *arg1;
    PyObject *arg2;
    PyObject *arg3;
    PyObject *tempargs;
    PyObject *result = NULL;

    EdsLib_Python_ObjectBase_t *Python_Packet;
    EdsLib_Python_Buffer_t *StorageBuffer;
    EdsLib_Binding_Buffer_Content_t edsbuf;
    CCSDS_SpacePacket_t *Packet;

    CFE_SB_Listener_Component_t Params;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSub;

    if (!PyArg_UnpackTuple(args, "DecodeEdsId", 3, 3, &arg1, &arg2, &arg3))
    {
        PyErr_Format(PyExc_RuntimeError, "Arguments expected: InstanceNumber, TopicId, and SpacePacket Message");
    	return Py_False;
    }

    Py_INCREF(arg1);
    Py_INCREF(arg2);
    Py_INCREF(arg3);

    do
    {
    	if (PyNumber_Check(arg1))
    	{
            tempargs = PyNumber_Long(arg1);

            Params.Telecommand.InstanceNumber = PyLong_AsUnsignedLong(tempargs);
            Py_DECREF(tempargs);
    	}
    	else
    	{
            PyErr_Format(PyExc_RuntimeError, "InstanceNumber needs to be an integer");
            break;
    	}

        if (PyNumber_Check(arg2))
        {
            tempargs = PyNumber_Long(arg2);

            Params.Telecommand.TopicId = PyLong_AsUnsignedLong(tempargs);
            Py_DECREF(tempargs);
        }
        else
    	{
            PyErr_Format(PyExc_RuntimeError, "TopicId needs to be an integer");
            break;
    	}

    	// Dive through an EdsLib python base object to get to the actual EDS data
        Python_Packet = (EdsLib_Python_ObjectBase_t *) arg3;
        StorageBuffer = Python_Packet->StorageBuf;
        edsbuf = StorageBuffer->edsbuf;
        Packet = (CCSDS_SpacePacket_t *) edsbuf.Data;

        CFE_SB_MapListenerComponent(&PubSub, &Params);
        CFE_SB_Set_PubSub_Parameters(Packet, &PubSub);

        Py_INCREF(Py_True);
        result = Py_True;
    } while(0);

    Py_XDECREF(arg1);
    Py_XDECREF(arg2);
    Py_XDECREF(arg3);

    return result;
}

static PyObject *  CFE_MissionLib_Python_Instance_iter(PyObject *obj)
{
	CFE_MissionLib_Python_InstanceIterator_t *InstIter;
    PyObject *result = NULL;

    InstIter = PyObject_GC_New(CFE_MissionLib_Python_InstanceIterator_t, &CFE_MissionLib_Python_InstanceIteratorType);

    if (InstIter == NULL)
    {
        return NULL;
    }

    Py_INCREF(obj);
    InstIter->refobj = obj;
    InstIter->Index = 1;

    result = (PyObject *)InstIter;
    PyObject_GC_Track(result);

    return result;
}

static void CFE_MissionLib_Python_InstanceIterator_dealloc(PyObject * obj)
{
    CFE_MissionLib_Python_InstanceIterator_t *self = (CFE_MissionLib_Python_InstanceIterator_t*)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int CFE_MissionLib_Python_InstanceIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_InstanceIterator_t *self = (CFE_MissionLib_Python_InstanceIterator_t*)obj;
    Py_VISIT(self->refobj);
    return 0;
}

static int CFE_MissionLib_Python_InstanceIterator_clear(PyObject *obj)
{
    CFE_MissionLib_Python_InstanceIterator_t *self = (CFE_MissionLib_Python_InstanceIterator_t*)obj;
    Py_CLEAR(self->refobj);
    return 0;
}

static PyObject *CFE_MissionLib_Python_InstanceIterator_iternext(PyObject *obj)
{
    CFE_MissionLib_Python_InstanceIterator_t *self = (CFE_MissionLib_Python_InstanceIterator_t*)obj;
    CFE_MissionLib_Python_Database_t *dbobj = NULL;
    const char * Label = NULL;
    char Buffer[64];
    char CheckBuffer[64];

    PyObject *key = NULL;
    PyObject *instanceid = NULL;
    PyObject *result = NULL;

    do
    {
    	if (self->refobj == NULL)
        {
            break;
        }

        dbobj = (CFE_MissionLib_Python_Database_t *)self->refobj;

        Label = CFE_MissionLib_GetInstanceName(dbobj->IntfDb, self->Index, Buffer, sizeof(Buffer));

        // If the instance doesn't exist then the return buffer is a string of the input index
        snprintf(CheckBuffer, sizeof(CheckBuffer), "%u", (unsigned int) self->Index);
        if (strcmp(Label,CheckBuffer) == 0)
        {
            break;
        }

        key = PyUnicode_FromString(Label);

        instanceid = PyLong_FromLong((long int)self->Index);

        ++self->Index;
        result = PyTuple_Pack(2, key, instanceid);
    }
    while(0);

    Py_XDECREF(key);
    Py_XDECREF(instanceid);

    return result;
}
