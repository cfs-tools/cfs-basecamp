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
**   Implement python type which represents a CFE_MissionLib database
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
#include "cfe_hdr_eds_datatypes.h"
#include "cfe_hdr_eds_defines.h"
#include "cfe_sb_eds_datatypes.h"
#include "edslib_binding_objects.h"

static const EdsLib_Id_t CFE_SB_TELECOMMAND_ID =
    EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsCommand_CFE_SB_Telecommand_indication_DECLARATION);
static const EdsLib_Id_t CFE_SB_TELEMETRY_ID =
    EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsCommand_CFE_SB_Telemetry_indication_DECLARATION);

PyObject *CFE_MissionLib_Python_DatabaseCache = NULL;

static void      CFE_MissionLib_Python_Database_dealloc(PyObject *obj);
static PyObject *CFE_MissionLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *CFE_MissionLib_Python_Database_repr(PyObject *obj);
static int       CFE_MissionLib_Python_Database_traverse(PyObject *obj, visitproc visit, void *arg);
static int       CFE_MissionLib_Python_Database_clear(PyObject *obj);

static PyObject *CFE_MissionLib_Python_Database_GetInterface(PyObject *obj, PyObject *key);
static PyObject *CFE_MissionLib_Python_DecodeEdsId(PyObject *obj, PyObject *args);
static PyObject *CFE_MissionLib_Python_Set_PubSub(PyObject *obj, PyObject *args);
static PyObject *CFE_MissionLib_Python_GetMsgId(PyObject *obj, PyObject *args);

static PyObject *CFE_MissionLib_Python_Instance_iter(PyObject *obj);
static void      CFE_MissionLib_Python_InstanceIterator_dealloc(PyObject *obj);
static int       CFE_MissionLib_Python_InstanceIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int       CFE_MissionLib_Python_InstanceIterator_clear(PyObject *obj);
static PyObject *CFE_MissionLib_Python_InstanceIterator_iternext(PyObject *obj);

static PyMethodDef CFE_MissionLib_Python_Database_methods[] = {
    { "Interface", CFE_MissionLib_Python_Database_GetInterface, METH_O, "Lookup an Interface type from DB." },
    { "DecodeEdsId", CFE_MissionLib_Python_DecodeEdsId, METH_VARARGS, "Decode the EdsID from a packed cFE message" },
    { "SetPubSub", CFE_MissionLib_Python_Set_PubSub, METH_VARARGS, "Set the PubSub parameters for a command message" },
    { "GetMsgId", CFE_MissionLib_Python_GetMsgId, METH_VARARGS, "Get the MsgId for a topic" },
    { NULL }  /* Sentinel */
};

static struct PyMemberDef CFE_MissionLib_Python_Database_members[] = {
    { "Name", T_OBJECT_EX, offsetof(CFE_MissionLib_Python_Database_t, DbName), READONLY, "Database Name" },
    { NULL }  /* Sentinel */
};

PyTypeObject CFE_MissionLib_Python_DatabaseType = { PyVarObject_HEAD_INIT(NULL, 0).tp_name =
                                                        CFE_MISSIONLIB_PYTHON_ENTITY_NAME("Database"),
                                                    .tp_basicsize = sizeof(CFE_MissionLib_Python_Database_t),
                                                    .tp_dealloc   = CFE_MissionLib_Python_Database_dealloc,
                                                    .tp_new       = CFE_MissionLib_Python_Database_new,
                                                    .tp_methods   = CFE_MissionLib_Python_Database_methods,
                                                    .tp_members   = CFE_MissionLib_Python_Database_members,
                                                    .tp_repr      = CFE_MissionLib_Python_Database_repr,
                                                    .tp_traverse  = CFE_MissionLib_Python_Database_traverse,
                                                    .tp_clear     = CFE_MissionLib_Python_Database_clear,
                                                    .tp_iter      = CFE_MissionLib_Python_Instance_iter,
                                                    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
                                                    .tp_weaklistoffset =
                                                        offsetof(CFE_MissionLib_Python_Database_t, WeakRefList),
                                                    .tp_doc = PyDoc_STR("Interface database") };

PyTypeObject CFE_MissionLib_Python_InstanceIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("InstanceIterator"),
    .tp_basicsize                          = sizeof(CFE_MissionLib_Python_InstanceIterator_t),
    .tp_dealloc                            = CFE_MissionLib_Python_InstanceIterator_dealloc,
    .tp_getattro                           = PyObject_GenericGetAttr,
    .tp_flags                              = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse                           = CFE_MissionLib_Python_InstanceIterator_traverse,
    .tp_clear                              = CFE_MissionLib_Python_InstanceIterator_clear,
    .tp_iter                               = PyObject_SelfIter,
    .tp_iternext                           = CFE_MissionLib_Python_InstanceIterator_iternext,
    .tp_doc                                = PyDoc_STR("CFE MissionLib InstanceIteratorType")
};

static int CFE_MissionLib_Python_Database_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;
    Py_VISIT(self->DbName);
    Py_VISIT(self->IntfCache);
    return 0;
}

static int CFE_MissionLib_Python_Database_clear(PyObject *obj)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;
    Py_CLEAR(self->DbName);
    Py_CLEAR(self->IntfCache);
    return 0;
}

static void CFE_MissionLib_Python_Database_dealloc(PyObject *obj)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;

    Py_CLEAR(self->DbName);
    Py_CLEAR(self->IntfCache);

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

PyObject *CFE_MissionLib_Python_GetFromCache(PyObject *cachedict, PyObject *idxval, PyTypeObject *reqtype)
{
    PyObject *weakref;
    PyObject *obj;

    obj = NULL;

    /* note: PyDict_GetItem returns borrowed reference */
    weakref = PyDict_GetItem(cachedict, idxval);
    if (weakref != NULL)
    {
        /* In python >= 3.13, PyWeakref_GetObject() is replaced with PyWeakRef_GetRef()
         * The new API differentiates between expired refs and other errors, it also
         * returns a new ref rather than a borrowed ref (so no increment) */
#if (PY_MAJOR_VERSION >= 3) && (PY_MINOR_VERSION >= 13)
        if (PyWeakref_GetRef(weakref, &obj) < 0)
        {
            /* it raised an error */
            return NULL;
        }
#else
        obj = PyWeakref_GetObject(weakref);
        Py_XINCREF(obj);
#endif
    }

    if (obj != NULL && reqtype != NULL)
    {
        /*
         * Confirm that the type is the expected type -
         * In cases where the weak ref expired then the previous call may
         * return Py_None rather than the object we were looking for
         */
        if (Py_TYPE(obj) != reqtype)
        {
            Py_DECREF(obj);
            obj = NULL;
        }
    }

    return obj;
}

int CFE_MissionLib_Python_SaveToCache(PyObject *cachedict, PyObject *idxval, PyObject *obj)
{
    PyObject *weakref;

    /* Create a weak reference to store in the local cache in case this
     * database is constructed again. */
    weakref = PyWeakref_NewRef(obj, NULL);
    if (weakref == NULL)
    {
        return -1;
    }

    PyDict_SetItem(cachedict, idxval, weakref);
    Py_DECREF(weakref);

    return 0;
}

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

    return ((CFE_MissionLib_Python_Database_t *)obj)->IntfDb;
}

static PyObject *CFE_MissionLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject                         *arg1;
    PyObject                         *arg2;
    PyObject                         *dbname;
    const char                       *dbstr;
    char                             *p;
    char                              tempstring[512];
    void                             *handle;
    void                             *symbol;
    const char                       *errstr;
    bool                              IsSuccess;
    CFE_MissionLib_Python_Database_t *self;

    if (!PyArg_UnpackTuple(args, "Database_new", 1, 2, &arg1, &arg2))
    {
        PyErr_Format(PyExc_RuntimeError,
                     "Database arguments: Interface Database Identifier and EdsDb Python Object (If Database "
                     "associated with Identifier has not been created");
        return NULL;
    }

    dbname    = NULL;
    handle    = NULL;
    symbol    = NULL;
    errstr    = NULL;
    self      = NULL;
    IsSuccess = false;

    do
    {
        if (PyUnicode_Check(arg1))
        {
            dbname = PyUnicode_AsASCIIString(arg1);
        }
        else
        {
            dbname = PyObject_ASCII(arg1);
        }

        if (dbname == NULL)
        {
            break;
        }

        /*
         * To avoid wasting resources, do not create the same database multiple times.
         * First check if the db cache object already contains an instance for this name
         * note: PyDict_GetItem returns borrowed reference
         */
        self =
            (CFE_MissionLib_Python_Database_t *)CFE_MissionLib_Python_GetFromCache(CFE_MissionLib_Python_DatabaseCache,
                                                                                   dbname,
                                                                                   &CFE_MissionLib_Python_DatabaseType);
        if (self != NULL)
        {
            IsSuccess = true;
            break;
        }

        dbstr = PyBytes_AsString(dbname);
        if (dbstr == NULL)
        {
            break;
        }

        snprintf(tempstring, sizeof(tempstring), "%s_eds_sb_dispatchdb.so", dbstr);

        /* Clear any pending dlerror value */
        dlerror();
        handle = dlopen(tempstring, RTLD_LOCAL | RTLD_NOW);
        errstr = dlerror();

        if (handle == NULL && errstr == NULL)
        {
            errstr = "Unspecified Error - NULL handle";
        }

        if (errstr != NULL)
        {
            PyErr_Format(PyExc_RuntimeError, "dlopen: %s", errstr);
            break;
        }

        snprintf(tempstring, sizeof(tempstring), "%s_SOFTWAREBUS_INTERFACE", dbstr);
        p = tempstring;
        while (*p != 0)
        {
            *p = toupper(*p);
            ++p;
        }

        symbol = dlsym(handle, tempstring);
        if (symbol == NULL)
        {
            PyErr_Format(PyExc_RuntimeError, "Symbol %s undefined", tempstring);
            break;
        }

        self = (CFE_MissionLib_Python_Database_t *)obj->tp_alloc(obj, 0);
        if (self == NULL)
        {
            break;
        }

        self->IntfDb = (const CFE_MissionLib_SoftwareBus_Interface_t *)symbol;

        Py_INCREF(dbname);
        self->DbName = dbname;

        self->IntfCache = PyDict_New();
        if (self->IntfCache == NULL)
        {
            break;
        }

        /* Create a weak reference to store in the local cache in case this
         * database is constructed again. */
        if (CFE_MissionLib_Python_SaveToCache(CFE_MissionLib_Python_DatabaseCache, dbname, (PyObject *)self) < 0)
        {
            /* if something went wrong this raises an error and must return NULL */
            break;
        }

        self->dl = handle;

        IsSuccess = true;
    } while (false);

    Py_XDECREF(dbname);
    if (!IsSuccess)
    {
        if (handle != NULL)
        {
            dlclose(handle);
            handle = NULL;
        }
        if (self != NULL)
        {
            Py_CLEAR(self->DbName);
            Py_CLEAR(self->IntfCache);
            Py_DECREF(self);
            self = NULL;
        }
    }

    return (PyObject *)self;
}

PyObject *CFE_MissionLib_Python_Database_repr(PyObject *obj)
{
    CFE_MissionLib_Python_Database_t *self = (CFE_MissionLib_Python_Database_t *)obj;
    return PyUnicode_FromFormat("%s(%R)", obj->ob_type->tp_name, self->DbName);
}

static PyObject *CFE_MissionLib_Python_Database_GetInterface(PyObject *obj, PyObject *arg)
{
    PyObject *tempargs = NULL;
    PyObject *result   = NULL;

    tempargs = PyTuple_Pack(2, obj, arg);
    if (tempargs == NULL)
    {
        return NULL;
    }
    result = PyObject_Call((PyObject *)&CFE_MissionLib_Python_InterfaceType, tempargs, NULL);
    Py_DECREF(tempargs);

    return result;
}

static PyObject *CFE_MissionLib_Python_DecodeEdsId(PyObject *obj, PyObject *args)
{
    CFE_MissionLib_Python_Database_t *IntfDb = (CFE_MissionLib_Python_Database_t *)obj;
    PyObject                         *arg1;

    Py_ssize_t BytesSize;
    char      *NetworkBuffer;

    EdsNativeBuffer_CFE_HDR_Message_t LocalBuffer;

    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSubParams;
    EdsComponent_CFE_SB_Publisher_t          PublisherParams;
    EdsComponent_CFE_SB_Listener_t           ListenerParams;

    EdsLib_Id_t                          EdsId;
    EdsLib_Id_t                          CmdEdsId;
    uint16_t                             TopicId;
    EdsLib_DataTypeDB_TypeInfo_t         TypeInfo;
    CFE_MissionLib_TopicInfo_t           TopicInfo;
    const EdsDataType_CFE_HDR_Message_t *MsgPtr;
    int32_t                              Status;

    PyObject *result = NULL;

    if (!PyArg_UnpackTuple(args, "DecodeEdsId", 1, 1, &arg1))
    {
        PyErr_Format(PyExc_RuntimeError, "encoded bytes string argument expected");
        return NULL;
    }

    Py_INCREF(arg1);

    do
    {
        if (!PyBytes_Check(arg1))
        {
            PyErr_Format(PyExc_RuntimeError, "DecodeEdsId argument not of bytes string type");
            break;
        }
        BytesSize = PyBytes_Size(arg1);
        PyBytes_AsStringAndSize(arg1, &NetworkBuffer, &BytesSize);

        EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), EdsContainer_CFE_HDR_TelemetryHeader_DATADICTIONARY);
        Status = EdsLib_DataTypeDB_GetTypeInfo(CFE_MissionLib_GetParent(IntfDb->IntfDb), EdsId, &TypeInfo);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            PyErr_Format(PyExc_RuntimeError,
                         "Unable to get type info for CCSDS_SPACEPACKET: return status = %d",
                         Status);
            break;
        }

        Status = EdsLib_DataTypeDB_UnpackPartialObject(CFE_MissionLib_GetParent(IntfDb->IntfDb),
                                                       &EdsId,
                                                       LocalBuffer.Byte,
                                                       NetworkBuffer,
                                                       sizeof(LocalBuffer),
                                                       8 * TypeInfo.Size.Bytes,
                                                       0);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            PyErr_Format(PyExc_RuntimeError, "Unable to unpack partial object: return status = %d", Status);
            break;
        }

        MsgPtr = (const EdsDataType_CFE_HDR_Message_t *)&LocalBuffer;

        CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, MsgPtr);

        if (CFE_MissionLib_PubSub_IsPublisherComponent(&PubSubParams))
        {
            CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);
            TopicId  = PublisherParams.Telemetry.TopicId;
            CmdEdsId = CFE_SB_TELEMETRY_ID;
        }
        else if (CFE_MissionLib_PubSub_IsListenerComponent(&PubSubParams))
        {
            CFE_MissionLib_UnmapListenerComponent(&ListenerParams, &PubSubParams);
            TopicId  = ListenerParams.Telecommand.TopicId;
            CmdEdsId = CFE_SB_TELECOMMAND_ID;
        }
        else
        {
            PyErr_Format(PyExc_RuntimeError,
                         "Unable to identify component for MsgId=0x%x",
                         (unsigned int)PubSubParams.MsgId.Value);
            break;
        }

        Status = CFE_MissionLib_GetTopicInfo(IntfDb->IntfDb, TopicId, &TopicInfo);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            PyErr_Format(PyExc_RuntimeError, "CFE_MissionLib_GetTopicInfo(%d) rc=%d", (int)TopicId, (int)Status);
            break;
        }

        Status = EdsLib_IntfDB_FindAllArgumentTypes(CFE_MissionLib_GetParent(IntfDb->IntfDb),
                                                    CmdEdsId,
                                                    TopicInfo.ParentIntfId,
                                                    &EdsId,
                                                    1);
        if (Status != EDSLIB_SUCCESS)
        {
            PyErr_Format(PyExc_RuntimeError, "Unable to get argument type: return status = %d", Status);
            break;
        }

        result = PyTuple_New(2);
        PyTuple_SetItem(result, 0, PyLong_FromLong((long int)EdsId));
        PyTuple_SetItem(result, 1, PyLong_FromLong((long int)TopicId));
    } while (0);

    Py_XDECREF(arg1);

    return result;
}

static uint16_t CFE_MissionLib_Python_ArgToInstanceNum(const CFE_MissionLib_SoftwareBus_Interface_t *IntfDb,
                                                       PyObject                                     *arg)
{
    uint16_t  Result;
    PyObject *temparg;

    Result = 0;

    if (PyNumber_Check(arg))
    {
        temparg = PyNumber_Long(arg);
    }
    else if (PyUnicode_Check(arg))
    {
        /* Do a lookup but make sure the string is plain ASCII */
        temparg = PyUnicode_AsASCIIString(arg);
    }
    else
    {
        temparg = PyObject_ASCII(arg);
    }

    if (temparg != NULL)
    {
        if (PyNumber_Check(arg))
        {
            Result = PyLong_AsUnsignedLong(temparg);
        }
        else
        {
            Result = CFE_MissionLib_GetInstanceNumber(IntfDb, PyBytes_AsString(temparg));
        }

        Py_DECREF(temparg);
        temparg = NULL;
    }

    if (Result == 0)
    {
        PyErr_Format(PyExc_ValueError, "Cannot convert %R to an instance number", arg);
    }

    return Result;
}

static uint16_t CFE_MissionLib_Python_ArgToTopicId(const CFE_MissionLib_SoftwareBus_Interface_t *IntfDb, PyObject *arg)
{
    uint16_t                       Result;
    PyObject                      *temparg;
    CFE_MissionLib_Python_Topic_t *topic_obj;

    Result = 0;

    if (PyObject_TypeCheck(arg, &CFE_MissionLib_Python_TopicType))
    {
        topic_obj = (CFE_MissionLib_Python_Topic_t *)arg;
        Result    = topic_obj->TopicId;
    }
    else if (PyNumber_Check(arg))
    {
        temparg = PyNumber_Long(arg);

        Result = PyLong_AsUnsignedLong(temparg);
        Py_DECREF(temparg);
    }

    if (Result == 0)
    {
        /* In theory this could accept a named topic as well, if needed */
        PyErr_Format(PyExc_ValueError, "Cannot convert %R to a topic ID", arg);
    }

    return Result;
}

static PyObject *CFE_MissionLib_Python_Set_PubSub(PyObject *obj, PyObject *args)
{
    PyObject *arg1;
    PyObject *arg2;
    PyObject *arg3;
    PyObject *result;

    EdsLib_Python_ObjectBase_t     *Python_Packet;
    EdsLib_Python_Buffer_t         *StorageBuffer;
    EdsLib_Binding_Buffer_Content_t edsbuf;
    EdsDataType_CFE_HDR_Message_t  *Packet;

    EdsComponent_CFE_SB_Listener_t           Params;
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSub;

    CFE_MissionLib_Python_Database_t *IntfDb = (CFE_MissionLib_Python_Database_t *)obj;

    if (!PyArg_UnpackTuple(args, "SetPubSub", 3, 3, &arg1, &arg2, &arg3))
    {
        PyErr_Format(PyExc_RuntimeError, "Arguments expected: InstanceNumber, TopicId, and SpacePacket Message");
        return Py_False;
    }

    Py_INCREF(arg1);
    Py_INCREF(arg2);
    Py_INCREF(arg3);

    result = NULL;
    memset(&Params, 0, sizeof(Params));

    do
    {
        Params.Telecommand.InstanceNumber = CFE_MissionLib_Python_ArgToInstanceNum(IntfDb->IntfDb, arg1);
        if (Params.Telecommand.InstanceNumber == 0)
        {
            /* error already raised */
            break;
        }

        Params.Telecommand.TopicId = CFE_MissionLib_Python_ArgToTopicId(IntfDb->IntfDb, arg2);
        if (Params.Telecommand.TopicId == 0)
        {
            /* error already raised */
            break;
        }

        // Dive through an EdsLib python base object to get to the actual EDS data
        Python_Packet = (EdsLib_Python_ObjectBase_t *)arg3;
        StorageBuffer = Python_Packet->StorageBuf;
        edsbuf        = StorageBuffer->edsbuf;
        Packet        = (EdsDataType_CFE_HDR_Message_t *)edsbuf.Data;

        CFE_MissionLib_MapListenerComponent(&PubSub, &Params);
        CFE_MissionLib_Set_PubSub_Parameters(Packet, &PubSub);

        Py_INCREF(Py_True);
        result = Py_True;
    } while (0);

    Py_XDECREF(arg1);
    Py_XDECREF(arg2);
    Py_XDECREF(arg3);

    return result;
}

static PyObject *CFE_MissionLib_Python_GetMsgId(PyObject *obj, PyObject *args)
{
    PyObject *arg1;
    PyObject *arg2;
    PyObject *result;

    union
    {
        EdsComponent_CFE_SB_Listener_t  L;
        EdsComponent_CFE_SB_Publisher_t P;
    } Params;
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSub;
    uint16_t                                 TopicId;
    uint16_t                                 InstanceNum;

    CFE_MissionLib_Python_Database_t *IntfDb = (CFE_MissionLib_Python_Database_t *)obj;

    if (!PyArg_UnpackTuple(args, "GetMsgId", 2, 2, &arg1, &arg2))
    {
        return PyErr_Format(PyExc_RuntimeError, "Arguments expected: InstanceNumber, TopicId");
    }

    Py_INCREF(arg1);
    Py_INCREF(arg2);

    result = NULL;
    memset(&Params, 0, sizeof(Params));

    do
    {
        InstanceNum = CFE_MissionLib_Python_ArgToInstanceNum(IntfDb->IntfDb, arg1);
        if (InstanceNum == 0)
        {
            /* error already raised */
            break;
        }

        TopicId = CFE_MissionLib_Python_ArgToTopicId(IntfDb->IntfDb, arg2);
        if (TopicId == 0)
        {
            /* error already raised */
            break;
        }

        /*
         * There is not a direct way to tell if a topic is CMD or TLM, for now
         * the approach has been to try both
         */
        Params.L.Telecommand.InstanceNumber = InstanceNum;
        Params.L.Telecommand.TopicId        = TopicId;
        if (!CFE_MissionLib_MapListenerComponent(&PubSub, &Params.L))
        {
            Params.P.Telemetry.InstanceNumber = InstanceNum;
            Params.P.Telemetry.TopicId        = TopicId;
            if (!CFE_MissionLib_MapPublisherComponent(&PubSub, &Params.P))
            {
                /* If neither worked, the TopicId must be bad */
                PyErr_Format(PyExc_ValueError, "TopicID %R cannot be converted to MsgID", arg2);
                break;
            }
        }

        result = PyLong_FromLong(PubSub.MsgId.Value);

    } while (0);

    Py_XDECREF(arg1);
    Py_XDECREF(arg2);

    return result;
}

static PyObject *CFE_MissionLib_Python_Instance_iter(PyObject *obj)
{
    CFE_MissionLib_Python_InstanceIterator_t *InstIter;
    PyObject                                 *result = NULL;

    InstIter = PyObject_GC_New(CFE_MissionLib_Python_InstanceIterator_t, &CFE_MissionLib_Python_InstanceIteratorType);

    if (InstIter == NULL)
    {
        return NULL;
    }

    Py_INCREF(obj);
    InstIter->refobj = obj;
    InstIter->Index  = 1;

    result = (PyObject *)InstIter;
    PyObject_GC_Track(result);

    return result;
}

static void CFE_MissionLib_Python_InstanceIterator_dealloc(PyObject *obj)
{
    CFE_MissionLib_Python_InstanceIterator_t *self = (CFE_MissionLib_Python_InstanceIterator_t *)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int CFE_MissionLib_Python_InstanceIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_InstanceIterator_t *self = (CFE_MissionLib_Python_InstanceIterator_t *)obj;
    Py_VISIT(self->refobj);
    return 0;
}

static int CFE_MissionLib_Python_InstanceIterator_clear(PyObject *obj)
{
    CFE_MissionLib_Python_InstanceIterator_t *self = (CFE_MissionLib_Python_InstanceIterator_t *)obj;
    Py_CLEAR(self->refobj);
    return 0;
}

static PyObject *CFE_MissionLib_Python_InstanceIterator_iternext(PyObject *obj)
{
    CFE_MissionLib_Python_InstanceIterator_t *self  = (CFE_MissionLib_Python_InstanceIterator_t *)obj;
    CFE_MissionLib_Python_Database_t         *dbobj = NULL;
    const char                               *Label = NULL;

    PyObject *key        = NULL;
    PyObject *instanceid = NULL;
    PyObject *result     = NULL;

    if (self->refobj == NULL)
    {
        /* bad object */
        return NULL;
    }

    dbobj = (CFE_MissionLib_Python_Database_t *)self->refobj;

    do
    {
        if (self->Index >= CFE_MissionLib_GetNumInstances(dbobj->IntfDb))
        {
            /* Reached the end */
            break;
        }

        Label = CFE_MissionLib_GetInstanceNameNonNull(dbobj->IntfDb, self->Index);

        key = PyUnicode_FromString(Label);

        instanceid = PyLong_FromLong((long int)self->Index);

        ++self->Index;
        result = PyTuple_Pack(2, key, instanceid);
    } while (0);

    Py_XDECREF(key);
    Py_XDECREF(instanceid);

    return result;
}
