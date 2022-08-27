/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
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


/**
 * \file     edslib_python_database.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement python type which represents an EDS database
**
**   Allows python code to dynamically load a database object and retrieve
**   entries from it.
 */

#include "edslib_python_internal.h"
#include <dlfcn.h>
#include <structmember.h>

PyObject *EdsLib_Python_DatabaseCache = NULL;

static void         EdsLib_Python_Database_dealloc(PyObject * obj);
static PyObject *   EdsLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *   EdsLib_Python_Database_repr(PyObject *obj);
static int          EdsLib_Python_Database_traverse(PyObject *obj, visitproc visit, void *arg);
static int          EdsLib_Python_Database_clear(PyObject *obj);

static PyObject *   EdsLib_Python_Database_getentry(PyObject *obj, PyObject *key);
static PyObject *   EdsLib_Python_Database_IsContainer(PyObject *obj, PyObject *arg);
static PyObject *   EdsLib_Python_Database_IsArray(PyObject *obj, PyObject *arg);
static PyObject *   EdsLib_Python_Database_IsNumber(PyObject *obj, PyObject *arg);
static PyObject *   EdsLib_Python_Database_IsEnum(PyObject *obj, PyObject *arg);

static PyMethodDef EdsLib_Python_Database_methods[] =
{
        {"Entry",       EdsLib_Python_Database_getentry, METH_O, "Lookup an EDS type from DB."},
        {"IsContainer", EdsLib_Python_Database_IsContainer, METH_O, "Check if an object is a container."},
        {"IsArray",     EdsLib_Python_Database_IsArray, METH_O, "Check if an object is an array."},
        {"IsNumber",    EdsLib_Python_Database_IsNumber, METH_O, "Check if an object is a number."},
        {"IsEnum",      EdsLib_Python_Database_IsEnum, METH_O, "Check if a database entry is an enumeration."},
        {NULL}  /* Sentinel */
};

static struct PyMemberDef EdsLib_Python_Database_members[] =
{
        {"Name", T_OBJECT_EX, offsetof(EdsLib_Python_Database_t, DbName), READONLY, "Database Name" },
        {NULL}  /* Sentinel */
};


PyTypeObject EdsLib_Python_DatabaseType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("Database"),
    .tp_basicsize = sizeof(EdsLib_Python_Database_t),
    .tp_dealloc = EdsLib_Python_Database_dealloc,
    .tp_new = EdsLib_Python_Database_new,
    .tp_methods = EdsLib_Python_Database_methods,
    .tp_members = EdsLib_Python_Database_members,
    .tp_repr = EdsLib_Python_Database_repr,
    .tp_traverse = EdsLib_Python_Database_traverse,
    .tp_clear = EdsLib_Python_Database_clear,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,
    .tp_weaklistoffset = offsetof(EdsLib_Python_Database_t, WeakRefList),
    .tp_doc = "EDS database"
};

static int EdsLib_Python_Database_traverse(PyObject *obj, visitproc visit, void *arg)
{
    EdsLib_Python_Database_t *self = (EdsLib_Python_Database_t *)obj;
    Py_VISIT(self->TypeCache);
    Py_VISIT(self->DbName);
    return 0;
}

static int EdsLib_Python_Database_clear(PyObject *obj)
{
    EdsLib_Python_Database_t *self = (EdsLib_Python_Database_t *)obj;
    Py_CLEAR(self->TypeCache);
    Py_CLEAR(self->DbName);
    return 0;
}

static void EdsLib_Python_Database_dealloc(PyObject * obj)
{
    EdsLib_Python_Database_t *self = (EdsLib_Python_Database_t *)obj;

    Py_CLEAR(self->DbName);
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

    EdsLib_Python_DatabaseType.tp_base->tp_dealloc(obj);
}

static EdsLib_Python_Database_t *EdsLib_Python_Database_CreateImpl(PyTypeObject *obj, PyObject *Name, const EdsLib_DatabaseObject_t *GD)
{
    EdsLib_Python_Database_t *self;
    PyObject *weakref;

    self = (EdsLib_Python_Database_t*)obj->tp_alloc(obj, 0);
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

    self->GD = GD;
    Py_INCREF(Name);
    self->DbName = Name;

    /* Create a weak reference to store in the local cache in case this
     * database is constructed again. */
    weakref = PyWeakref_NewRef((PyObject*)self, NULL);
    if (weakref == NULL)
    {
        Py_DECREF(self);
        return NULL;
    }

    PyDict_SetItem(EdsLib_Python_DatabaseCache, Name, weakref);
    Py_DECREF(weakref);

    return self;
}

PyObject *EdsLib_Python_Database_CreateFromStaticDB(const char *Name, const EdsLib_DatabaseObject_t *GD)
{
    PyObject *pyname = PyUnicode_FromString(Name);
    EdsLib_Python_Database_t *self;
    PyObject *weakref;

    if (pyname == NULL)
    {
        return NULL;
    }

    weakref = PyDict_GetItem(EdsLib_Python_DatabaseCache, pyname);
    if (weakref != NULL)
    {
        self = (EdsLib_Python_Database_t *)PyWeakref_GetObject(weakref);
        if (Py_TYPE(self) == &EdsLib_Python_DatabaseType)
        {
            if (self->GD != GD)
            {
                PyErr_SetString(PyExc_RuntimeError, "Database name conflict");
                self = NULL;
            }
            else
            {
                Py_INCREF(self);
            }
            return (PyObject*)self;
        }
    }


    return (PyObject*)EdsLib_Python_Database_CreateImpl(&EdsLib_Python_DatabaseType, pyname, GD);
}

const EdsLib_DatabaseObject_t *EdsLib_Python_Database_GetDB(PyObject *obj)
{
    if (obj == NULL)
    {
        return NULL;
    }
    if (!PyObject_TypeCheck(obj, &EdsLib_Python_DatabaseType))
    {
    	PyErr_SetObject(PyExc_TypeError, obj);
        return NULL;
    }

    return ((EdsLib_Python_Database_t*)obj)->GD;
}

static PyObject *EdsLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    const char *dbstr;
    PyObject *nameobj;
    char *p;
    char tempstring[512];
    void *handle;
    void *symbol;
    const char *errstr;
    EdsLib_Python_Database_t *self;
    PyObject *weakref;

    if (!PyArg_ParseTuple(args, "s:Database_new", &dbstr))
    {
        return NULL;
    }

    /*
     * To avoid wasting resources, do not create the same database multiple times.
     * First check if the db cache object already contains an instance for this name
     * note: PyDict_GetItem returns borrowed reference
     */
    weakref = PyDict_GetItemString(EdsLib_Python_DatabaseCache, (char*)dbstr);
    if (weakref != NULL)
    {
        self = (EdsLib_Python_Database_t *)PyWeakref_GetObject(weakref);
        if (Py_TYPE(self) == &EdsLib_Python_DatabaseType)
        {
            Py_INCREF(self);
            return (PyObject*)self;
        }
    }

    snprintf(tempstring,sizeof(tempstring),"%s_eds_db.so", dbstr);

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

    snprintf(tempstring,sizeof(tempstring),"%s_DATABASE", dbstr);
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

    self = EdsLib_Python_Database_CreateImpl(&EdsLib_Python_DatabaseType, nameobj, symbol);
    Py_DECREF(nameobj);
    if (self == NULL)
    {
        dlclose(handle);
        return NULL;
    }

    self->dl = handle;

    return (PyObject*)self;
}

PyObject *   EdsLib_Python_Database_repr(PyObject *obj)
{
    EdsLib_Python_Database_t *self = (EdsLib_Python_Database_t *)obj;

    return PyUnicode_FromFormat("%s(\'%U\')", obj->ob_type->tp_name, self->DbName);
}

static PyObject *EdsLib_Python_Database_getentry(PyObject *obj, PyObject *key)
{
    EdsLib_Python_Database_t *dbobj = (EdsLib_Python_Database_t *)obj;
    PyObject *keybytes = NULL;
    PyObject *result = NULL;

    if (PyUnicode_Check(key))
    {
        keybytes = PyUnicode_AsUTF8String(key);
    }
    else
    {
        /* this should work for python2 style strings */
        keybytes = PyObject_Bytes(key);
    }

    if (keybytes == NULL)
    {
        return NULL;
    }

    result = (PyObject*)EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl(dbobj,
            EdsLib_DisplayDB_LookupTypeName(dbobj->GD, PyBytes_AsString(keybytes)));

    Py_DECREF(keybytes);

    return result;
}

static PyObject *   EdsLib_Python_Database_IsContainer(PyObject *obj, PyObject *arg)
{
    PyObject *result;

    if (Py_TYPE(arg)->tp_base == &EdsLib_Python_ObjectContainerType)
    {
        Py_INCREF(Py_True);
        result = Py_True;
    }
    else
    {
        Py_INCREF(Py_False);
        result = Py_False;
    }

    return result;
}

static PyObject *   EdsLib_Python_Database_IsArray(PyObject *obj, PyObject *arg)
{
    PyObject *result;

    if (Py_TYPE(arg)->tp_base == &EdsLib_Python_ObjectArrayType)
    {
        Py_INCREF(Py_True);
        result = Py_True;
    }
    else
    {
        Py_INCREF(Py_False);
        result = Py_False;
    }

    return result;
}

static PyObject *   EdsLib_Python_Database_IsNumber(PyObject *obj, PyObject *arg)
{
    PyObject *result;

    if (Py_TYPE(arg)->tp_base == &EdsLib_Python_ObjectNumberType)
    {
        Py_INCREF(Py_True);
        result = Py_True;
    }
    else
    {
        Py_INCREF(Py_False);
        result = Py_False;
    }

    return result;
}

static PyObject *   EdsLib_Python_Database_IsEnum(PyObject *obj, PyObject *arg)
{
    EdsLib_Python_DatabaseEntry_t *dbent;
    EdsLib_DisplayHint_t DisplayHint;
    PyTypeObject *base_type;

    PyObject *result = NULL;

    if (Py_TYPE(arg) == &EdsLib_Python_DatabaseEntryType)
    {
        dbent = (EdsLib_Python_DatabaseEntry_t *)arg;
        DisplayHint = EdsLib_DisplayDB_GetDisplayHint(dbent->EdsDb->GD, dbent->EdsId);
        base_type = dbent->type_base.ht_type.tp_base;

        if ((DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE) && (base_type == &EdsLib_Python_ObjectNumberType))
        {
            Py_INCREF(Py_True);
            result = Py_True;
        }
        else
        {
            Py_INCREF(Py_False);
            result = Py_False;
        }
    }
    else
    {
        PyErr_Format(PyExc_TypeError, "Input argument must be an EdsLib Database Entry type");
    }

    return result;
}
