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

static void      EdsLib_Python_Database_dealloc(PyObject *obj);
static PyObject *EdsLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static PyObject *EdsLib_Python_Database_repr(PyObject *obj);
static int       EdsLib_Python_Database_traverse(PyObject *obj, visitproc visit, void *arg);
static int       EdsLib_Python_Database_clear(PyObject *obj);

static PyObject *EdsLib_Python_Database_getentry(PyObject *obj, PyObject *key);
static PyObject *EdsLib_Python_Database_IsContainer(PyObject *obj, PyObject *arg);
static PyObject *EdsLib_Python_Database_IsArray(PyObject *obj, PyObject *arg);
static PyObject *EdsLib_Python_Database_IsNumber(PyObject *obj, PyObject *arg);
static PyObject *EdsLib_Python_Database_IsEnum(PyObject *obj, PyObject *arg);

static PyMethodDef EdsLib_Python_Database_methods[] = {
    { "Entry", EdsLib_Python_Database_getentry, METH_O, "Lookup an EDS type from DB." },
    { "IsContainer", EdsLib_Python_Database_IsContainer, METH_O, "Check if an object is a container." },
    { "IsArray", EdsLib_Python_Database_IsArray, METH_O, "Check if an object is an array." },
    { "IsNumber", EdsLib_Python_Database_IsNumber, METH_O, "Check if an object is a number." },
    { "IsEnum", EdsLib_Python_Database_IsEnum, METH_O, "Check if a database entry is an enumeration." },
    { NULL }  /* Sentinel */
};

static struct PyMemberDef EdsLib_Python_Database_members[] = {
    { "Name", T_OBJECT_EX, offsetof(EdsLib_Python_Database_t, DbName), READONLY, "Database Name" },
    { NULL }  /* Sentinel */
};

PyTypeObject EdsLib_Python_DatabaseType = { PyVarObject_HEAD_INIT(NULL, 0).tp_name =
                                                EDSLIB_PYTHON_ENTITY_NAME("Database"),
                                            .tp_basicsize      = sizeof(EdsLib_Python_Database_t),
                                            .tp_dealloc        = EdsLib_Python_Database_dealloc,
                                            .tp_new            = EdsLib_Python_Database_new,
                                            .tp_methods        = EdsLib_Python_Database_methods,
                                            .tp_members        = EdsLib_Python_Database_members,
                                            .tp_repr           = EdsLib_Python_Database_repr,
                                            .tp_traverse       = EdsLib_Python_Database_traverse,
                                            .tp_clear          = EdsLib_Python_Database_clear,
                                            .tp_flags          = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
                                            .tp_weaklistoffset = offsetof(EdsLib_Python_Database_t, WeakRefList),
                                            .tp_doc            = "EDS database" };

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

static void EdsLib_Python_Database_dealloc(PyObject *obj)
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

PyObject *EdsLib_Python_GetFromCache(PyObject *cachedict, PyObject *idxval, PyTypeObject *reqtype)
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

int EdsLib_Python_SaveToCache(PyObject *cachedict, PyObject *idxval, PyObject *obj)
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

static EdsLib_Python_Database_t *
EdsLib_Python_Database_CreateImpl(PyTypeObject *obj, PyObject *Name, const EdsLib_DatabaseObject_t *GD)
{
    EdsLib_Python_Database_t *self;
    bool                      IsSuccess;

    IsSuccess = false;

    do
    {
        self = (EdsLib_Python_Database_t *)obj->tp_alloc(obj, 0);
        if (self == NULL)
        {
            break;
        }

        self->TypeCache = PyDict_New();
        if (self->TypeCache == NULL)
        {
            break;
        }

        self->GD = GD;
        Py_INCREF(Name);
        self->DbName = Name;

        /* Create a weak reference to store in the local cache in case this
         * database is constructed again. */
        if (EdsLib_Python_SaveToCache(EdsLib_Python_DatabaseCache, Name, (PyObject *)self) < 0)
        {
            /* if something went wrong this raises an error and must return NULL */
            break;
        }

        IsSuccess = true;
    } while (false);

    if (!IsSuccess && self != NULL)
    {
        Py_CLEAR(self->TypeCache);
        Py_CLEAR(self->DbName);
        Py_DECREF(self);
        self = NULL;
    }

    return self;
}

PyObject *EdsLib_Python_Database_CreateFromStaticDB(const char *Name, const EdsLib_DatabaseObject_t *GD)
{
    PyObject                 *pyname = PyUnicode_FromString(Name);
    EdsLib_Python_Database_t *self;

    if (pyname == NULL)
    {
        return NULL;
    }

    self = (EdsLib_Python_Database_t *)EdsLib_Python_GetFromCache(EdsLib_Python_DatabaseCache,
                                                                  pyname,
                                                                  &EdsLib_Python_DatabaseType);
    if (self != NULL)
    {
        if (self->GD != GD)
        {
            PyErr_SetString(PyExc_RuntimeError, "Database name conflict");
            Py_DECREF(self);
            self = NULL;
        }
    }
    else
    {
        self = EdsLib_Python_Database_CreateImpl(&EdsLib_Python_DatabaseType, pyname, GD);
    }

    return (PyObject *)self;
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

    return ((EdsLib_Python_Database_t *)obj)->GD;
}

static PyObject *EdsLib_Python_Database_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    const char               *dbstr;
    PyObject                 *nameobj;
    PyObject                 *argobj;
    char                     *p;
    char                      tempstring[512];
    void                     *handle;
    void                     *symbol;
    const char               *errstr;
    EdsLib_Python_Database_t *self;
    bool                      IsSuccess;

    /* note: the object returned here is a borrowed ref */
    if (!PyArg_ParseTuple(args, "O:Database_new", &argobj))
    {
        return NULL;
    }

    handle    = NULL;
    symbol    = NULL;
    errstr    = NULL;
    nameobj   = NULL;
    self      = NULL;
    IsSuccess = false;

    do
    {
        /* The database name used for EdsLib should be a plain ASCII string, not unicode */
        if (PyUnicode_Check(argobj))
        {
            /* this gets a ref to a Bytes object from the borrowed ref */
            nameobj = PyUnicode_AsASCIIString(argobj);
        }
        else
        {
            /* must be representable as a string */
            nameobj = PyObject_ASCII(argobj);
        }

        /* if something went wrong with the conversion, an error should already be raised */
        if (nameobj == NULL)
        {
            break;
        }

        /*
         * To avoid wasting resources, do not create the same database multiple times.
         * First check if the db cache object already contains an instance for this name
         */
        self = (EdsLib_Python_Database_t *)EdsLib_Python_GetFromCache(EdsLib_Python_DatabaseCache,
                                                                      nameobj,
                                                                      &EdsLib_Python_DatabaseType);
        if (self != NULL)
        {
            IsSuccess = true;
            break;
        }

        dbstr = PyBytes_AsString(nameobj);
        if (dbstr == NULL)
        {
            break;
        }

        snprintf(tempstring, sizeof(tempstring), "%s_eds_db.so", dbstr);

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

        snprintf(tempstring, sizeof(tempstring), "%s_DATABASE", dbstr);
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

        self = EdsLib_Python_Database_CreateImpl(&EdsLib_Python_DatabaseType, nameobj, symbol);
        if (self == NULL)
        {
            break;
        }

        self->dl  = handle;
        IsSuccess = true;

    } while (false);

    Py_XDECREF(nameobj);
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
            Py_CLEAR(self->TypeCache);
            Py_DECREF(self);
            self = NULL;
        }
    }

    return (PyObject *)self;
}

PyObject *EdsLib_Python_Database_repr(PyObject *obj)
{
    EdsLib_Python_Database_t *self = (EdsLib_Python_Database_t *)obj;

    return PyUnicode_FromFormat("%s(%R)", obj->ob_type->tp_name, self->DbName);
}

static PyObject *EdsLib_Python_Database_getentry(PyObject *obj, PyObject *key)
{
    EdsLib_Python_Database_t *dbobj  = (EdsLib_Python_Database_t *)obj;
    PyObject                 *result = NULL;
    EdsLib_Id_t               EdsId;

    EdsId = EdsLib_Python_ConvertArgToEdsId(dbobj->GD, key);

    /* if not valid this should have raised an exception */
    if (EdsLib_Is_Valid(EdsId))
    {
        result = (PyObject *)EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl(dbobj, EdsId);
    }

    return result;
}

static PyObject *EdsLib_Python_Database_IsContainer(PyObject *obj, PyObject *arg)
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

static PyObject *EdsLib_Python_Database_IsArray(PyObject *obj, PyObject *arg)
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

static PyObject *EdsLib_Python_Database_IsNumber(PyObject *obj, PyObject *arg)
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

static PyObject *EdsLib_Python_Database_IsEnum(PyObject *obj, PyObject *arg)
{
    EdsLib_Python_DatabaseEntry_t *dbent;
    EdsLib_DisplayHint_t           DisplayHint;
    PyTypeObject                  *base_type;

    PyObject *result = NULL;

    if (Py_TYPE(arg) == &EdsLib_Python_DatabaseEntryType)
    {
        dbent       = (EdsLib_Python_DatabaseEntry_t *)arg;
        DisplayHint = EdsLib_DisplayDB_GetDisplayHint(dbent->EdsDb->GD, dbent->EdsId);
        base_type   = dbent->type_base.ht_type.tp_base;

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
