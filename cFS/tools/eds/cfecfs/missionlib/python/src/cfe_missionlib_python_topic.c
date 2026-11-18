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
#include "cfe_hdr_eds_datatypes.h"
#include <dlfcn.h>
#include <structmember.h>
#include "edslib_id.h"

static void      CFE_MissionLib_Python_Topic_dealloc(PyObject *obj);
static PyObject *CFE_MissionLib_Python_Topic_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *CFE_MissionLib_Python_Topic_repr(PyObject *obj);
static int       CFE_MissionLib_Python_Topic_traverse(PyObject *obj, visitproc visit, void *arg);
static int       CFE_MissionLib_Python_Topic_clear(PyObject *obj);
static PyObject *CFE_MissionLib_Python_Topic_GetCmdEdsIdFromCode(PyObject *obj, PyObject *args);

static CFE_MissionLib_Python_Topic_t *
CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(PyTypeObject *obj, PyObject *intfobj, uint16_t TopicId);

static PyObject *CFE_MissionLib_Python_Topic_iter(PyObject *obj);
static void      CFE_MissionLib_Python_TopicIterator_dealloc(PyObject *obj);
static int       CFE_MissionLib_Python_TopicIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int       CFE_MissionLib_Python_TopicIterator_clear(PyObject *obj);
static PyObject *CFE_MissionLib_Python_TopicIterator_iternext(PyObject *obj);

struct CbArg
{
    uint8_t     CommandCode;
    EdsLib_Id_t PossibleId;
    EdsLib_Id_t EdsId;
};

typedef struct CbArg CbArg_t;

void SubcommandCallback(const EdsLib_DatabaseObject_t        *GD,
                        const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
                        EdsLib_GenericValueBuffer_t          *ConstraintValue,
                        void                                 *Arg);

static PyMethodDef CFE_MissionLib_Python_Topic_methods[] = {
    { "GetCmdEdsId",
     CFE_MissionLib_Python_Topic_GetCmdEdsIdFromCode, METH_VARARGS,
     "Get a CFE command message EDS Object from a Topic" },
    { NULL }  /* Sentinel */
};

static struct PyMemberDef CFE_MissionLib_Python_Topic_members[] = {
    { "Name", T_OBJECT_EX, offsetof(CFE_MissionLib_Python_Topic_t, TopicName), READONLY, "Topic Name" },
    { "TopicId", T_SHORT, offsetof(CFE_MissionLib_Python_Topic_t, TopicId), READONLY, "Topic ID" },
    { "EdsId", T_INT, offsetof(CFE_MissionLib_Python_Topic_t, DataEdsId), READONLY, "EDS ID" },
    { NULL }  /* Sentinel */
};

PyTypeObject CFE_MissionLib_Python_TopicType = { PyVarObject_HEAD_INIT(NULL, 0).tp_name =
                                                     CFE_MISSIONLIB_PYTHON_ENTITY_NAME("Topic"),
                                                 .tp_basicsize = sizeof(CFE_MissionLib_Python_Topic_t),
                                                 .tp_dealloc   = CFE_MissionLib_Python_Topic_dealloc,
                                                 .tp_new       = CFE_MissionLib_Python_Topic_new,
                                                 .tp_methods   = CFE_MissionLib_Python_Topic_methods,
                                                 .tp_members   = CFE_MissionLib_Python_Topic_members,
                                                 .tp_repr      = CFE_MissionLib_Python_Topic_repr,
                                                 .tp_traverse  = CFE_MissionLib_Python_Topic_traverse,
                                                 .tp_clear     = CFE_MissionLib_Python_Topic_clear,
                                                 .tp_iter      = CFE_MissionLib_Python_Topic_iter,
                                                 .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
                                                 .tp_weaklistoffset =
                                                     offsetof(CFE_MissionLib_Python_Topic_t, WeakRefList),
                                                 .tp_doc = PyDoc_STR("Topic entry") };

PyTypeObject CFE_MissionLib_Python_TopicIteratorType = { PyVarObject_HEAD_INIT(NULL, 0).tp_name =
                                                             CFE_MISSIONLIB_PYTHON_ENTITY_NAME("TopicIterator"),
                                                         .tp_basicsize = sizeof(CFE_MissionLib_Python_TopicIterator_t),
                                                         .tp_dealloc   = CFE_MissionLib_Python_TopicIterator_dealloc,
                                                         .tp_getattro  = PyObject_GenericGetAttr,
                                                         .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
                                                         .tp_traverse  = CFE_MissionLib_Python_TopicIterator_traverse,
                                                         .tp_clear     = CFE_MissionLib_Python_TopicIterator_clear,
                                                         .tp_iter      = PyObject_SelfIter,
                                                         .tp_iternext  = CFE_MissionLib_Python_TopicIterator_iternext,
                                                         .tp_doc = PyDoc_STR("CFE MissionLib TopicIteratorType") };

static int CFE_MissionLib_Python_Topic_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    Py_VISIT(self->IntfObj);
    Py_VISIT(self->TopicName);
    return 0;
}

static int CFE_MissionLib_Python_Topic_clear(PyObject *obj)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    Py_CLEAR(self->IntfObj);
    Py_CLEAR(self->TopicName);
    return 0;
}

static void CFE_MissionLib_Python_Topic_dealloc(PyObject *obj)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    Py_CLEAR(self->IntfObj);
    Py_CLEAR(self->TopicName);

    if (self->WeakRefList != NULL)
    {
        PyObject_ClearWeakRefs(obj);
        self->WeakRefList = NULL;
    }

    CFE_MissionLib_Python_TopicType.tp_base->tp_dealloc(obj);
}

static CFE_MissionLib_Python_Topic_t *
CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(PyTypeObject *obj, PyObject *intfobj, uint16_t TopicId)
{
    CFE_MissionLib_Python_Interface_t *IntfObj = (CFE_MissionLib_Python_Interface_t *)intfobj;
    CFE_MissionLib_Python_Topic_t     *self    = NULL;
    char                               Buffer[128];
    int32_t                            Status;
    PyObject                          *TopicIdVal;

    do
    {
        TopicIdVal = PyLong_FromUnsignedLong(TopicId);
        if (TopicIdVal == NULL)
        {
            break;
        }

        self = (CFE_MissionLib_Python_Topic_t *)CFE_MissionLib_Python_GetFromCache(IntfObj->TopicCache,
                                                                                   TopicIdVal,
                                                                                   &CFE_MissionLib_Python_TopicType);
        if (self != NULL)
        {
            if (TopicId == self->TopicId)
            {
                /* matches */
                break;
            }

            /* not a match */
            Py_DECREF(self);
            self = NULL;
        }

        self = (CFE_MissionLib_Python_Topic_t *)obj->tp_alloc(obj, 0);
        if (self == NULL)
        {
            break;
        }

        Py_INCREF(IntfObj);
        self->IntfObj = IntfObj;

        Status = CFE_MissionLib_GetTopicInfo(IntfObj->DbObj->IntfDb, TopicId, &self->TopicInfo);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            PyErr_Format(PyExc_RuntimeError,
                         "Failure in CFE_MissionLib_GetTopicInfo(%d) rc=%d",
                         (int)TopicId,
                         (int)Status);
            Py_CLEAR(self);
            break;
        }

        Status = EdsLib_IntfDB_GetFullName(CFE_MissionLib_GetParent(IntfObj->DbObj->IntfDb),
                                           self->TopicInfo.ParentIntfId,
                                           Buffer,
                                           sizeof(Buffer));
        if (Status != EDSLIB_SUCCESS)
        {
            /* Intf ID does not appear to be valid, this is not a valid topic ID */
            PyErr_Format(PyExc_RuntimeError, "No interface mapping for topicid=%d", (int)TopicId);
            Py_CLEAR(self);
            break;
        }

        self->TopicName = PyUnicode_FromFormat("%s", Buffer);
        self->TopicId   = TopicId;

        /* All SB topics should map to an interface type with a single command on it */
        Status = EdsLib_IntfDB_FindAllArgumentTypes(CFE_MissionLib_GetParent(IntfObj->DbObj->IntfDb),
                                                    IntfObj->CmdEdsId,
                                                    self->TopicInfo.ParentIntfId,
                                                    &self->DataEdsId,
                                                    1);
        if (Status != EDSLIB_SUCCESS)
        {
            /* Unable to determine data type on this topic */
            PyErr_Format(PyExc_RuntimeError,
                         "EdsLib_IntfDB_FindAllArgumentTypes(%x) rc=%d",
                         (unsigned int)IntfObj->CmdEdsId,
                         (int)Status);
            Py_CLEAR(self);
            break;
        }

        /* Create a weak reference to store in the local cache in case this
         * database is constructed again. */
        if (CFE_MissionLib_Python_SaveToCache(IntfObj->TopicCache, TopicIdVal, (PyObject *)self) < 0)
        {
            /* if something went wrong this raises an error and must return NULL */
            Py_DECREF(self);
            self = NULL;
        }
    } while (0);

    Py_XDECREF(TopicIdVal);
    return self;
}

static PyObject *CFE_MissionLib_Python_Topic_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject                          *arg1;
    PyObject                          *arg2;
    PyObject                          *arg3;
    PyObject                          *arg4 = NULL;
    CFE_MissionLib_Python_Database_t  *DbObj;
    CFE_MissionLib_Python_Interface_t *IntfObj;
    uint16_t                           TopicId = 0; // 0 is an invalid TopicId
    EdsLib_Id_t                        CompIntfId;
    uint16_t                           status;
    PyObject                          *tempargs = NULL;
    PyObject                          *result   = NULL;

    /*
     * Topic entries are constructed from three values:
     *   - An Interface Database
     *   - An Interface Identifier
     *   - A Topic Identifier
     *   - An EdsDb Python Object (only needed if Database associated with the Interface Database Identifier has not
     * been set up)
     *
     * Databases are natively of the CFE_MissionLib_Python_Database_t type
     * Interface Identifiers are indices which are natively unsigned integers (EdsLib_Id_t specifically)
     * Topic Identifiers are also indices which are natively unsigned integers (uint16_t specifically)
     *
     * However, any value can alternatively be supplied as a string,
     * in which case it can be looked up and converted to the native value.
     */
    if (!PyArg_UnpackTuple(args, "Topic_new", 3, 4, &arg1, &arg2, &arg3, &arg4))
    {
        PyErr_Format(PyExc_RuntimeError,
                     "Topic arguments expected: Interface Database, Interface Identifier, Topic Identifier, and EdsDb "
                     "Python Object (optional)");
        return NULL;
    }

    do
    {
        // Set up the Interface Database
        if (Py_TYPE(arg1) == &CFE_MissionLib_Python_DatabaseType)
        {
            DbObj = (CFE_MissionLib_Python_Database_t *)arg1;
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

        // Set up the interface
        if (Py_TYPE(arg2) == &CFE_MissionLib_Python_InterfaceType)
        {
            IntfObj = (CFE_MissionLib_Python_Interface_t *)arg2;
            Py_INCREF(IntfObj);
        }
        else
        {
            tempargs = PyTuple_Pack(2, (PyObject *)DbObj, arg2);
            if (tempargs == NULL)
            {
                break;
            }
            IntfObj =
                (CFE_MissionLib_Python_Interface_t *)PyObject_Call((PyObject *)&CFE_MissionLib_Python_InterfaceType,
                                                                   tempargs,
                                                                   NULL);
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
        }
        else
        {
            // Set up the topic with either a TopicId or Topic Name
            CompIntfId = CFE_MissionLib_Python_ConvertArgToEdsId(EdsLib_IntfDB_FindComponentInterfaceByFullName,
                                                                 CFE_MissionLib_GetParent(DbObj->IntfDb),
                                                                 arg3);

            /* if the lookup failed it should have already set an exception */
            if (!EdsLib_Is_Valid(CompIntfId))
            {
                break;
            }

            status = CFE_MissionLib_FindTopicIdFromIntfId(DbObj->IntfDb, CompIntfId, &TopicId);
            if (status != CFE_MISSIONLIB_SUCCESS)
            {
                PyErr_Format(PyExc_RuntimeError, "No TopicID maps to %s", PyBytes_AsString(tempargs));
                break;
            }
        }

        result = (PyObject *)CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(&CFE_MissionLib_Python_TopicType,
                                                                             (PyObject *)IntfObj,
                                                                             TopicId);
    } while (0);

    /* decrement refcount for all temporary objects created */
    Py_XDECREF(tempargs);

    return result;
}

PyObject *CFE_MissionLib_Python_Topic_repr(PyObject *obj)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    return PyUnicode_FromFormat("%s(%R,%R,%R)",
                                obj->ob_type->tp_name,
                                self->IntfObj->DbObj->DbName,
                                self->IntfObj->IntfName,
                                self->TopicName);
}

PyObject *CFE_MissionLib_Python_Topic_GetFromTopicName(CFE_MissionLib_Python_Interface_t *obj, PyObject *TopicName)
{
    CFE_MissionLib_Python_Topic_t *TopicEntry;

    int32_t     status;
    uint16_t    TopicId;
    EdsLib_Id_t CompIntfId;

    TopicEntry = (CFE_MissionLib_Python_Topic_t *)CFE_MissionLib_Python_GetFromCache(obj->TopicCache,
                                                                                     TopicName,
                                                                                     &CFE_MissionLib_Python_TopicType);
    if (TopicEntry != NULL)
    {
        if (TopicEntry->TopicName != TopicName)
        {
            PyErr_SetString(PyExc_RuntimeError, "Topic name conflict");
            Py_DECREF(TopicEntry);
            TopicEntry = NULL;
        }
        return (PyObject *)TopicEntry;
    }

    /* this is slightly more complicated; the name is actually associated with a component interface,
     * and once that is found, then we need to reverse lookup that intf back to a topic ID */
    status = EdsLib_IntfDB_FindComponentInterfaceByFullName(CFE_MissionLib_GetParent(obj->DbObj->IntfDb),
                                                            PyBytes_AsString(TopicName),
                                                            &CompIntfId);
    if (status != EDSLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "Component Intf %s not defined", PyBytes_AsString(TopicName));
        return NULL;
    }

    status = CFE_MissionLib_FindTopicIdFromIntfId(obj->DbObj->IntfDb, CompIntfId, &TopicId);
    if (status != CFE_MISSIONLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "No TopicID maps to %s", PyBytes_AsString(TopicName));
        return NULL;
    }

    return (PyObject *)CFE_MissionLib_Python_Topic_GetFromTopicId_Impl(&CFE_MissionLib_Python_TopicType,
                                                                       (PyObject *)obj,
                                                                       TopicId);
}

static PyObject *CFE_MissionLib_Python_Topic_iter(PyObject *obj)
{
    CFE_MissionLib_Python_Topic_t         *Topic = (CFE_MissionLib_Python_Topic_t *)obj;
    CFE_MissionLib_Python_TopicIterator_t *TopicIter;
    PyObject                              *result = NULL;

    if (Topic->TopicInfo.NumSubcommands > 0)
    {
        TopicIter = PyObject_GC_New(CFE_MissionLib_Python_TopicIterator_t, &CFE_MissionLib_Python_TopicIteratorType);

        if (TopicIter == NULL)
        {
            return NULL;
        }

        Py_INCREF(obj);
        TopicIter->Index  = 0;
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

static void CFE_MissionLib_Python_TopicIterator_dealloc(PyObject *obj)
{
    CFE_MissionLib_Python_TopicIterator_t *self = (CFE_MissionLib_Python_TopicIterator_t *)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int CFE_MissionLib_Python_TopicIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    CFE_MissionLib_Python_TopicIterator_t *self = (CFE_MissionLib_Python_TopicIterator_t *)obj;
    Py_VISIT(self->refobj);

    return 0;
}

static int CFE_MissionLib_Python_TopicIterator_clear(PyObject *obj)
{
    CFE_MissionLib_Python_TopicIterator_t *self = (CFE_MissionLib_Python_TopicIterator_t *)obj;
    Py_CLEAR(self->refobj);

    return 0;
}

static PyObject *CFE_MissionLib_Python_TopicIterator_iternext(PyObject *obj)
{
    CFE_MissionLib_Python_TopicIterator_t *self  = (CFE_MissionLib_Python_TopicIterator_t *)obj;
    CFE_MissionLib_Python_Topic_t         *topic = NULL;
    const EdsLib_DatabaseObject_t         *GD;
    uint16_t                               idx;
    EdsLib_Id_t                            PossibleId;

    PyObject *key    = NULL;
    PyObject *edsid  = NULL;
    PyObject *result = NULL;

    do
    {
        if (self->refobj == NULL)
        {
            break;
        }

        topic = (CFE_MissionLib_Python_Topic_t *)self->refobj;
        GD    = CFE_MissionLib_GetParent(topic->IntfObj->DbObj->IntfDb);
        idx   = self->Index;

        if (EdsLib_DataTypeDB_GetDerivedTypeById(GD, topic->DataEdsId, idx, &PossibleId) == EDSLIB_SUCCESS)
        {
            key   = PyUnicode_FromString(EdsLib_DisplayDB_GetBaseName(GD, PossibleId));
            edsid = PyLong_FromLong((long int)PossibleId);

            if ((key == NULL) || (edsid == NULL))
            {
                Py_CLEAR(self->refobj);
                break;
            }

            ++self->Index;
            result = PyTuple_Pack(2, key, edsid);
        }
    } while (0);

    Py_XDECREF(key);
    Py_XDECREF(edsid);

    return result;
}

void SubcommandCallback(const EdsLib_DatabaseObject_t        *GD,
                        const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
                        EdsLib_GenericValueBuffer_t          *ConstraintValue,
                        void                                 *Arg)
{
    CbArg_t *Argument = (CbArg_t *)Arg;

    if (MemberInfo->Offset.Bytes == offsetof(EdsDataType_CFE_HDR_CommandHeader_t, Sec.FunctionCode)
        && ConstraintValue->Value.u8 == Argument->CommandCode)
    {
        Argument->EdsId = Argument->PossibleId;
    }
}

static PyObject *CFE_MissionLib_Python_Topic_GetCmdEdsIdFromCode(PyObject *obj, PyObject *args)
{
    CFE_MissionLib_Python_Topic_t *self = (CFE_MissionLib_Python_Topic_t *)obj;
    const EdsLib_DatabaseObject_t *GD;
    PyObject                      *arg1;
    uint8_t                        CommandCode;
    int32_t                        idx;
    EdsLib_Id_t                    PossibleId;
    EdsLib_ConstraintCallback_t    Callback = SubcommandCallback;
    CbArg_t                        Argument;
    PyObject                      *result = NULL;

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

        GD = CFE_MissionLib_GetParent(self->IntfObj->DbObj->IntfDb);

        Argument.CommandCode = CommandCode;
        Argument.EdsId       = EDSLIB_ID_INVALID;

        for (idx = 0; idx < self->TopicInfo.NumSubcommands && !EdsLib_Is_Valid(Argument.EdsId); idx++)
        {
            EdsLib_DataTypeDB_GetDerivedTypeById(GD, self->DataEdsId, idx, &PossibleId);
            Argument.PossibleId = PossibleId;
            EdsLib_DataTypeDB_ConstraintIterator(GD, self->DataEdsId, PossibleId, Callback, (void *)&Argument);
        }

        if (EdsLib_Is_Valid(Argument.EdsId))
        {
            result = PyLong_FromLong((long int)Argument.EdsId);
        }
        else
        {
            result = Py_None;
            Py_INCREF(result);
        }
    } while (0);

    return result;
}
