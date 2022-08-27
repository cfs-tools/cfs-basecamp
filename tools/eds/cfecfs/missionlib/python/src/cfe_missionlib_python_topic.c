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
** File:  cfe_missionlib_python_interfaceentry.c
**
** Created on: Feb 11, 2020
** Author: mathew.j.mccaskey
**
** Purpose:
**   Implement the "interface entry" type
**
**   This is a datatype which refers back to an entry in a CFE Interface database.
**   All instances of EDS objects should be types of this type.  (note that in
**   python types are also objects which have a type, recursively)
**
******************************************************************************/

#include "cfe_missionlib_python_internal.h"
#include <dlfcn.h>
#include <structmember.h>
#include "edslib_id.h"

PyObject *CFE_MissionLib_Python_TopicCache = NULL;

static void         CFE_MissionLib_Python_Topic_dealloc(PyObject * obj);
static PyObject *   CFE_MissionLib_Python_Topic_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *   CFE_MissionLib_Python_Topic_repr(PyObject *obj);
static int          CFE_MissionLib_Python_Topic_traverse(PyObject *obj, visitproc visit, void *arg);
static int          CFE_MissionLib_Python_Topic_clear(PyObject *obj);
static PyObject *   CFE_MissionLib_Python_Topic_GetCmdEdsIdFromCode(PyObject *obj, PyObject *args);

static CFE_MissionLib_Python_Topic_t *CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(PyTypeObject *obj, PyObject *intfobj, uint16_t TopicId);

static PyObject *   CFE_MissionLib_Python_Topic_iter(PyObject *obj);
static void         CFE_MissionLib_Python_TopicIterator_dealloc(PyObject * obj);
static int          CFE_MissionLib_Python_TopicIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int          CFE_MissionLib_Python_TopicIterator_clear(PyObject *obj);
static PyObject *   CFE_MissionLib_Python_TopicIterator_iternext(PyObject *obj);

struct CbArg
{
    uint8_t CommandCode;
    EdsLib_Id_t PossibleId;
    EdsLib_Id_t EdsId;
};

typedef struct CbArg CbArg_t;

void SubcommandCallback(const EdsLib_DatabaseObject_t *GD, const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
        EdsLib_GenericValueBuffer_t *ConstraintValue, void *Arg);

static PyMethodDef CFE_MissionLib_Python_Topic_methods[] =
{
        {"GetCmdEdsId", CFE_MissionLib_Python_Topic_GetCmdEdsIdFromCode, METH_VARARGS, "Get a CFE command message EDS Object from a Topic"},
        {NULL}  /* Sentinel */
};

static struct PyMemberDef CFE_MissionLib_Python_Topic_members[] =
{
        {"Name", T_OBJECT_EX, offsetof(CFE_MissionLib_Python_Topic_t, TopicName), READONLY, "Topic Name" },
        {"TopicId", T_SHORT, offsetof(CFE_MissionLib_Python_Topic_t, TopicId), READONLY, "Topic ID" },
        {"EdsId", T_INT, offsetof(CFE_MissionLib_Python_Topic_t, EdsId), READONLY, "EDS ID" },
        {NULL}  /* Sentinel */
};


PyTypeObject CFE_MissionLib_Python_TopicType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("Topic"),
    .tp_basicsize = sizeof(CFE_MissionLib_Python_Topic_t),
    .tp_dealloc = CFE_MissionLib_Python_Topic_dealloc,
    .tp_new = CFE_MissionLib_Python_Topic_new,
    .tp_methods = CFE_MissionLib_Python_Topic_methods,
    .tp_members = CFE_MissionLib_Python_Topic_members,
    .tp_repr = CFE_MissionLib_Python_Topic_repr,
    .tp_traverse = CFE_MissionLib_Python_Topic_traverse,
    .tp_clear = CFE_MissionLib_Python_Topic_clear,
    .tp_iter = CFE_MissionLib_Python_Topic_iter,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,
    .tp_weaklistoffset = offsetof(CFE_MissionLib_Python_Topic_t, WeakRefList),
    .tp_doc = PyDoc_STR("Topic entry")
};

PyTypeObject CFE_MissionLib_Python_TopicIteratorType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = CFE_MISSIONLIB_PYTHON_ENTITY_NAME("TopicIterator"),
    .tp_basicsize = sizeof(CFE_MissionLib_Python_TopicIterator_t),
    .tp_dealloc = CFE_MissionLib_Python_TopicIterator_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = CFE_MissionLib_Python_TopicIterator_traverse,
    .tp_clear = CFE_MissionLib_Python_TopicIterator_clear,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = CFE_MissionLib_Python_TopicIterator_iternext,
    .tp_doc = PyDoc_STR("CFE MissionLib TopicIteratorType")
};

static int CFE_MissionLib_Python_Topic_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    Py_VISIT(self->IntfObj);
    Py_VISIT(self->TopicName);
    Py_VISIT(self->TypeCache);

    return 0;
}

static int CFE_MissionLib_Python_Topic_clear(PyObject *obj)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    Py_CLEAR(self->IntfObj);
    Py_CLEAR(self->TopicName);
    Py_CLEAR(self->TypeCache);

    return 0;
}

static void CFE_MissionLib_Python_Topic_dealloc(PyObject * obj)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    Py_CLEAR(self->IntfObj);
    Py_CLEAR(self->TypeCache);
    Py_CLEAR(self->TopicName);

    if (self->WeakRefList != NULL)
    {
        PyObject_ClearWeakRefs(obj);
        self->WeakRefList = NULL;
    }

    CFE_MissionLib_Python_TopicType.tp_base->tp_dealloc(obj);
}

static CFE_MissionLib_Python_Topic_t *CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(PyTypeObject *obj, PyObject *intfobj, uint16_t TopicId)
{
    CFE_MissionLib_Python_Interface_t *IntfObj = (CFE_MissionLib_Python_Interface_t *)intfobj;
    CFE_MissionLib_Python_Topic_t *self;
    PyObject *TopicIdVal;
    PyObject *weakref;
    const char* TopicName = NULL;

    do
    {
        TopicIdVal = PyLong_FromUnsignedLong(TopicId);
        if (TopicIdVal == NULL)
        {
            break;
        }

        weakref = PyDict_GetItem(CFE_MissionLib_Python_TopicCache, TopicIdVal);
        if (weakref != NULL)
        {
            self = (CFE_MissionLib_Python_Topic_t *)PyWeakref_GetObject(weakref);
            if (Py_TYPE(self) == &CFE_MissionLib_Python_TopicType)
            {
                if (TopicId == self->TopicId)
                {
                    Py_INCREF(self);
                    break;
                }

                /* weakref expired, needs to be recreated */
                self = NULL;
            }
        }

        self = (CFE_MissionLib_Python_Topic_t*)obj->tp_alloc(obj, 0);
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

        Py_INCREF(IntfObj);
        self->IntfObj = IntfObj;
        TopicName = CFE_MissionLib_GetTopicName(IntfObj->DbObj->IntfDb, IntfObj->InterfaceId, TopicId);
        self->TopicName = PyUnicode_FromFormat("%s", TopicName);
        self->TopicId = TopicId;
        CFE_MissionLib_GetArgumentType(IntfObj->DbObj->IntfDb, IntfObj->InterfaceId, TopicId, 1, 1, &self->EdsId);
        CFE_MissionLib_GetIndicationInfo(IntfObj->DbObj->IntfDb, IntfObj->InterfaceId, TopicId, 1, &self->IndInfo);

        /* Create a weak reference to store in the local cache in case this
         * database is constructed again. */
        weakref = PyWeakref_NewRef((PyObject*)self, NULL);
        if (weakref == NULL)
        {
            Py_DECREF(self);
            break;
        }

        PyDict_SetItem(CFE_MissionLib_Python_TopicCache, TopicIdVal, weakref);
        Py_DECREF(weakref);

    }
    while(0);

    Py_XDECREF(TopicIdVal);
    return self;
}

//const CFE_MissionLib_TopicId_Entry_t *CFE_MissionLib_Python_Topic_GetEntry(PyObject *obj)
//{
//    if (obj == NULL)
//    {
//        return NULL;
//    }
//    if (!PyObject_TypeCheck(obj, &CFE_MissionLib_Python_TopicType))
//    {
//        PyErr_SetObject(PyExc_TypeError, obj);
//        return NULL;
//    }
//
//    return ((CFE_MissionLib_Python_Topic_t*)obj)->TopicEntry;
//}

static PyObject *CFE_MissionLib_Python_Topic_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject *arg1;
    PyObject *arg2;
    PyObject *arg3;
    PyObject *arg4 = NULL;
    CFE_MissionLib_Python_Database_t *DbObj;
    CFE_MissionLib_Python_Interface_t *IntfObj;
    uint16_t TopicId = 0;      // 0 is an invalid TopicId

    CFE_MissionLib_TopicInfo_t TopicInfo;
    int32_t Status;

    uint16_t status;
    PyObject *tempargs = NULL;
    PyObject *result = NULL;

    /*
     * Topic entries are constructed from three values:
     *   - An Interface Database
     *   - An Interface Identifier
     *   - A Topic Identifier
     *   - An EdsDb Python Object (only needed if Database associated with the Interface Database Identifier has not been set up)
     *
     * Databases are natively of the CFE_MissionLib_Python_Database_t type
     * Interface Identifiers are indices which are natively unsigned integers (uint16_t specifically)
     * Topic Identifiers are also indices which are natively unsigned integers (uint16_t specifically)
     *
     * However, any value can alternatively be supplied as a string,
     * in which case it can be looked up and converted to the native value.
     */
    if (!PyArg_UnpackTuple(args, "Topic_new", 3, 4, &arg1, &arg2, &arg3, &arg4))
    {
        PyErr_Format(PyExc_RuntimeError, "Topic arguments expected: Interface Database, Interface Identifier, Topic Identifier, and EdsDb Python Object (optional)");
    	return NULL;
    }

    do
    {
    	// Set up the Interface Database
        if (Py_TYPE(arg1) == &CFE_MissionLib_Python_DatabaseType)
        {
            DbObj = (CFE_MissionLib_Python_Database_t*)arg1;
            Py_INCREF(DbObj);
        }
        else
        {
            if (arg4 != NULL)
            {
            	tempargs = PyTuple_Pack(2, arg1, arg4);
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

        // Set up the interface
        if (Py_TYPE(arg2) == &CFE_MissionLib_Python_InterfaceType)
        {
            IntfObj = (CFE_MissionLib_Python_Interface_t*)arg2;
            Py_INCREF(IntfObj);
        }
        else
        {
            tempargs = PyTuple_Pack(2, (PyObject *)DbObj, arg2);
            if (tempargs == NULL)
            {
                break;
            }
            IntfObj = (CFE_MissionLib_Python_Interface_t*)PyObject_Call((PyObject*)&CFE_MissionLib_Python_InterfaceType, tempargs, NULL);
            Py_DECREF(tempargs);
            tempargs = NULL;
        }

        if (IntfObj == NULL)
        {
            break;
        }

        // Set up the topic with either a TopicId or Topic Name
        if (PyNumber_Check(arg3))
        {
            tempargs = PyNumber_Long(arg3);
            if (tempargs == NULL)
            {
                break;
            }

            TopicId = PyLong_AsUnsignedLong(tempargs);
            Status = CFE_MissionLib_GetTopicInfo(DbObj->IntfDb, IntfObj->InterfaceId, TopicId, &TopicInfo);
            if (Status != CFE_MISSIONLIB_SUCCESS)
            {
                PyErr_Format(PyExc_RuntimeError, "Invalid Topic Id: %u\tStatus: %d", TopicId, Status);
                break;
            }
        }
        else
        {
            if (PyUnicode_Check(arg3))
            {
                tempargs = PyUnicode_AsUTF8String(arg3);
            }
            else
            {
                tempargs = PyObject_Bytes(arg3);
            }

            if (tempargs == NULL)
            {
                break;
            }

            status = CFE_MissionLib_FindTopicByName(DbObj->IntfDb, IntfObj->InterfaceId, PyBytes_AsString(tempargs), &TopicId);

            if (status != CFE_MISSIONLIB_SUCCESS)
            {
                PyErr_Format(PyExc_RuntimeError, "Topic %s undefined", PyBytes_AsString(tempargs));
                break;
            }
        }

        result = (PyObject*)CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(&CFE_MissionLib_Python_TopicType, (PyObject *)IntfObj, TopicId);
    }
    while(0);

    /* decrement refcount for all temporary objects created */
    Py_XDECREF(tempargs);

    return result;
}

PyObject *CFE_MissionLib_Python_Topic_repr(PyObject *obj)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    return PyUnicode_FromFormat("%s(%R,%R,%R)", obj->ob_type->tp_name, self->IntfObj->DbObj->DbName, self->IntfObj->IntfName, self->TopicName);
}

PyObject *CFE_MissionLib_Python_Topic_GetFromTopicName(CFE_MissionLib_Python_Interface_t *intfobj, PyObject *TopicName)
{
    CFE_MissionLib_Python_Topic_t *TopicEntry;

    PyObject *weakref;
    int32_t status;
    uint16_t TopicId;

    weakref = PyDict_GetItem(CFE_MissionLib_Python_TopicCache, TopicName);
    if (weakref != NULL)
    {
        TopicEntry = (CFE_MissionLib_Python_Topic_t *)PyWeakref_GetObject(weakref);
        if (Py_TYPE(TopicEntry) == &CFE_MissionLib_Python_TopicType)
        {
            if (TopicEntry->TopicName != TopicName)
            {
                PyErr_SetString(PyExc_RuntimeError, "Topic name conflict");
                TopicEntry = NULL;
            }
            else
            {
                Py_INCREF(TopicEntry);
            }
            return (PyObject*)TopicEntry;
        }
    }

    status = CFE_MissionLib_FindTopicByName(intfobj->DbObj->IntfDb, intfobj->InterfaceId, PyBytes_AsString(TopicName), &TopicId);

    if (status != CFE_MISSIONLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "Topic %s undefined:%u", PyBytes_AsString(TopicName));
        return NULL;
    }

    return (PyObject*)CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(&CFE_MissionLib_Python_TopicType, (PyObject *)intfobj, TopicId);
}

static PyObject *  CFE_MissionLib_Python_Topic_iter(PyObject *obj)
{
    CFE_MissionLib_Python_Topic_t *Topic = (CFE_MissionLib_Python_Topic_t *) obj;
    CFE_MissionLib_Python_TopicIterator_t *TopicIter;
    PyObject *result = NULL;

    if (Topic->IndInfo.NumSubcommands > 0)
    {
    	TopicIter = PyObject_GC_New(CFE_MissionLib_Python_TopicIterator_t, &CFE_MissionLib_Python_TopicIteratorType);

    	if (TopicIter == NULL)
    	{
            return NULL;
    	}

    	Py_INCREF(obj);
    	TopicIter->Index = 0;
    	TopicIter->refobj = obj;

    	result = (PyObject *)TopicIter;
    	PyObject_GC_Track(result);
    }
    else
    {
    	PyErr_Format(PyExc_RuntimeError, "Not an iterable Topic");
    }
    return result;
}

static void CFE_MissionLib_Python_TopicIterator_dealloc(PyObject * obj)
{
    CFE_MissionLib_Python_TopicIterator_t *self = (CFE_MissionLib_Python_TopicIterator_t*)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int CFE_MissionLib_Python_TopicIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_TopicIterator_t *self = (CFE_MissionLib_Python_TopicIterator_t*)obj;
    Py_VISIT(self->refobj);

    return 0;
}

static int CFE_MissionLib_Python_TopicIterator_clear(PyObject *obj)
{
    CFE_MissionLib_Python_TopicIterator_t *self = (CFE_MissionLib_Python_TopicIterator_t*)obj;
    Py_CLEAR(self->refobj);

    return 0;
}

static PyObject *CFE_MissionLib_Python_TopicIterator_iternext(PyObject *obj)
{
    CFE_MissionLib_Python_TopicIterator_t *self = (CFE_MissionLib_Python_TopicIterator_t*)obj;
    CFE_MissionLib_Python_Topic_t *topic = NULL;
    EdsLib_Python_Database_t *EdsDb;
    uint16_t idx;
    EdsLib_Id_t PossibleId;

    PyObject *key = NULL;
    PyObject *edsid = NULL;
    PyObject *result = NULL;

    do
    {
    	if (self->refobj == NULL)
        {
            break;
        }

        topic = (CFE_MissionLib_Python_Topic_t *)self->refobj;
        EdsDb = topic->IntfObj->DbObj->EdsDbObj;
        idx = self->Index;

        if (EdsLib_DataTypeDB_GetDerivedTypeById(EdsDb->GD, topic->EdsId, idx, &PossibleId) == EDSLIB_SUCCESS)
        {

            key = PyUnicode_FromString(EdsLib_DisplayDB_GetBaseName(EdsDb->GD, PossibleId));
            edsid = PyLong_FromLong((long int)PossibleId);

            if ((key == NULL) || (edsid == NULL))
            {
                Py_CLEAR(self->refobj);
                break;
            }

            ++self->Index;
            result = PyTuple_Pack(2, key, edsid);
        }
    }
    while(0);

    Py_XDECREF(key);
    Py_XDECREF(edsid);

    return result;
}

void SubcommandCallback(const EdsLib_DatabaseObject_t *GD, const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
        EdsLib_GenericValueBuffer_t *ConstraintValue, void *Arg)
{
    CbArg_t *Argument = (CbArg_t *) Arg;

    if (ConstraintValue->Value.u8 == Argument->CommandCode )
    {
        Argument->EdsId = Argument->PossibleId;
    }
}

static PyObject *CFE_MissionLib_Python_Topic_GetCmdEdsIdFromCode(PyObject *obj, PyObject *args)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t*) obj;
    EdsLib_Python_Database_t *EdsDb;
    PyObject *arg1;
    uint8_t CommandCode;
    int32_t idx;
    EdsLib_Id_t PossibleId;
    EdsLib_ConstraintCallback_t Callback = SubcommandCallback;
    CbArg_t Argument;
    PyObject *result = NULL;

    do
    {
        if (!PyArg_UnpackTuple(args, "Command Code", 1, 1, &arg1))
        {
            break;
        }

        // Get the command code from the input argument
        if (PyLong_Check(arg1))
        {
            CommandCode = PyLong_AsUnsignedLong(arg1);
        }
        else
        {
            PyErr_SetString(PyExc_TypeError, "Command Code argument: long expected");
            break;
        }

        EdsDb = self->IntfObj->DbObj->EdsDbObj;
        Argument.CommandCode = CommandCode;

        for (idx = 0; idx < self->IndInfo.NumSubcommands; idx++)
        {
            EdsLib_DataTypeDB_GetDerivedTypeById(EdsDb->GD, self->EdsId, idx, &PossibleId);
            Argument.PossibleId = PossibleId;
            EdsLib_DataTypeDB_ConstraintIterator(EdsDb->GD, self->EdsId, PossibleId, Callback, (void *)&Argument);
        }

        result = PyLong_FromLong((long int) Argument.EdsId);
    } while(0);

    return result;
}
