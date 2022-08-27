/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
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


/**
 * \file     edslib_python_databaseentry.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   Implement the "database entry" type
**
**   This is a dynamic datatype which refers back to an entry in an EDS database.
**   All instances of EDS objects should be types of this type.  (note that in
**   python types are also objects which have a type, recursively)
 */

#include "edslib_python_internal.h"
#include <structmember.h>

static PyObject *   EdsLib_Python_DatabaseEntry_new(PyTypeObject *obj, PyObject *args, PyObject *kwds);
static int          EdsLib_Python_DatabaseEntry_init(PyObject *obj, PyObject *args, PyObject *kwds);
static void         EdsLib_Python_DatabaseEntry_dealloc(PyObject * obj);
static PyObject *   EdsLib_Python_DatabaseEntry_repr(PyObject *obj);
static int          EdsLib_Python_DatabaseEntry_traverse(PyObject *obj, visitproc visit, void *arg);
static int          EdsLib_Python_DatabaseEntry_clear(PyObject *obj);
static Py_ssize_t   EdsLib_Python_DatabaseEntry_len(PyObject *obj);
static PyObject *   EdsLib_Python_DatabaseEntry_seq_item(PyObject *obj, Py_ssize_t idx);
static PyObject *   EdsLib_Python_DatabaseEntry_map_subscript(PyObject *obj, PyObject *subscr);
static PyObject *   EdsLib_Python_DatabaseEntry_iter(PyObject *obj);

static void         EdsLib_Python_EnumEntryIterator_dealloc(PyObject * obj);
static int          EdsLib_Python_EnumEntryIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int          EdsLib_Python_EnumEntryIterator_clear(PyObject *obj);
static PyObject *   EdsLib_Python_EnumEntryIterator_iternext(PyObject *obj);

static void         EdsLib_Python_ContainerEntryIterator_dealloc(PyObject * obj);
static int          EdsLib_Python_ContainerEntryIterator_traverse(PyObject *obj, visitproc visit, void *arg);
static int          EdsLib_Python_ContainerEntryIterator_clear(PyObject *obj);
static PyObject *   EdsLib_Python_ContainerEntryIterator_iternext(PyObject *obj);

static PySequenceMethods EdsLib_Python_DatabaseEntry_SequenceMethods =
{
        .sq_length = EdsLib_Python_DatabaseEntry_len,
        .sq_item = EdsLib_Python_DatabaseEntry_seq_item
};

static PyMappingMethods EdsLib_Python_DatabaseEntry_MappingMethods =
{
        .mp_length = EdsLib_Python_DatabaseEntry_len,
        .mp_subscript = EdsLib_Python_DatabaseEntry_map_subscript
};

static struct PyMemberDef EdsLib_Python_DatabaseEntry_members[] =
{
        {"Name", T_OBJECT_EX, offsetof(EdsLib_Python_DatabaseEntry_t, BaseName), READONLY, "Database Name" },
        {NULL}  /* Sentinel */
};

PyTypeObject EdsLib_Python_DatabaseEntryType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("DatabaseEntry"),
    .tp_basicsize = sizeof(EdsLib_Python_DatabaseEntry_t),
    .tp_dealloc = EdsLib_Python_DatabaseEntry_dealloc,
    .tp_new = EdsLib_Python_DatabaseEntry_new,
    .tp_as_sequence = &EdsLib_Python_DatabaseEntry_SequenceMethods,
    .tp_as_mapping = &EdsLib_Python_DatabaseEntry_MappingMethods,
    .tp_members = EdsLib_Python_DatabaseEntry_members,
    .tp_init = EdsLib_Python_DatabaseEntry_init,
    .tp_repr = EdsLib_Python_DatabaseEntry_repr,
    .tp_traverse = EdsLib_Python_DatabaseEntry_traverse,
    .tp_clear = EdsLib_Python_DatabaseEntry_clear,
    .tp_base = &PyType_Type,
    .tp_iter = EdsLib_Python_DatabaseEntry_iter,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,
    .tp_doc = PyDoc_STR("EDS database entry")
};

PyTypeObject EdsLib_Python_EnumEntryIteratorType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("EnumerationEntryIterator"),
    .tp_basicsize = sizeof(EdsLib_Python_EnumerationIterator_t),
    .tp_dealloc = EdsLib_Python_EnumEntryIterator_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = EdsLib_Python_EnumEntryIterator_traverse,
    .tp_clear = EdsLib_Python_EnumEntryIterator_clear,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = EdsLib_Python_EnumEntryIterator_iternext,
    .tp_doc = PyDoc_STR("EDS EnumerationIteratorType")
};

PyTypeObject EdsLib_Python_ContainerEntryIteratorType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = EDSLIB_PYTHON_ENTITY_NAME("ContainerEntryIterator"),
    .tp_basicsize = sizeof(EdsLib_Python_ContainerIterator_t),
    .tp_dealloc = EdsLib_Python_ContainerEntryIterator_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = EdsLib_Python_ContainerEntryIterator_traverse,
    .tp_clear = EdsLib_Python_ContainerEntryIterator_clear,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = EdsLib_Python_ContainerEntryIterator_iternext,
    .tp_doc = PyDoc_STR("EDS ContainerIteratorType")
};

static void EdsLib_Python_DatabaseEntry_GetFormatCodes(char *buffer, const EdsLib_Python_Database_t *refdb, EdsLib_Id_t EdsId)
{
    EdsLib_DataTypeDB_EntityInfo_t EntityInfo;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    Py_ssize_t RepeatCount;
    char code = 0;

    /*
     * For arrays, get the format for the actual item.
     * Otherwise get the format for the element itself
     */
    EdsLib_DataTypeDB_GetTypeInfo(refdb->GD, EdsId, &TypeInfo);
    if (TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY)
    {
        RepeatCount = TypeInfo.NumSubElements;
        EdsLib_DataTypeDB_GetMemberByIndex(refdb->GD, EdsId, 0, &EntityInfo);
        EdsLib_DataTypeDB_GetTypeInfo(refdb->GD, EntityInfo.EdsId, &TypeInfo);
    }
    else
    {
        RepeatCount = 1;
    }

    switch(TypeInfo.ElemType)
    {
    case EDSLIB_BASICTYPE_UNSIGNED_INT:
        if (TypeInfo.Size.Bytes == sizeof(unsigned char))
        {
            code = 'B';
        }
        else if (TypeInfo.Size.Bytes == sizeof(unsigned short))
        {
            code = 'H';
        }
        else if (TypeInfo.Size.Bytes == sizeof(unsigned int))
        {
            code = 'I';
        }
        else if (TypeInfo.Size.Bytes == sizeof(unsigned long))
        {
            code = 'L';
        }
        else if (TypeInfo.Size.Bytes == sizeof(unsigned long long))
        {
            code = 'Q';
        }
        break;
    case EDSLIB_BASICTYPE_SIGNED_INT:
        if (TypeInfo.Size.Bytes == sizeof(signed char))
        {
            code = 'b';
        }
        else if (TypeInfo.Size.Bytes == sizeof(signed short))
        {
            code = 'h';
        }
        else if (TypeInfo.Size.Bytes == sizeof(signed int))
        {
            code = 'i';
        }
        else if (TypeInfo.Size.Bytes == sizeof(signed long))
        {
            code = 'l';
        }
        else if (TypeInfo.Size.Bytes == sizeof(signed long long))
        {
            code = 'q';
        }
        break;
    case EDSLIB_BASICTYPE_FLOAT:
        if (TypeInfo.Size.Bytes == sizeof(float))
        {
            code = 'f';
        }
        else if (TypeInfo.Size.Bytes == sizeof(double))
        {
            code = 'd';
        }
        break;
    default:
        break;
    }

    if (code == 0)
    {
        RepeatCount *= TypeInfo.Size.Bytes;
        code = 'B';
    }

    if (RepeatCount > 1)
    {
        snprintf(buffer, EDSLIB_PYTHON_FORMATCODE_LEN, "%u%c",
                (unsigned int)RepeatCount, code);
    }
    else
    {
        buffer[0] = code;
        buffer[1] = 0;
    }
}



static void EdsLib_Python_DatabaseEntry_build_dict_callback(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc)
{
    PyObject *args = Arg;
    PyObject *MemberDictionary = PyTuple_GET_ITEM(args, 1);
    PyObject *MemberList = PyTuple_GET_ITEM(args, 2);
    PyObject *MemberName = NULL;
    PyObject *accessor = NULL;

    if (PyErr_Occurred())
    {
        return;
    }

    if (ParamDesc->FullName != NULL)
    {
        do
        {
            MemberName = PyUnicode_FromString(ParamDesc->FullName);
            if (MemberName == NULL)
            {
                /* error */
                break;
            }

            accessor = EdsLib_Python_ElementAccessor_CreateFromEntityInfo(&ParamDesc->EntityInfo);
            if (accessor == NULL)
            {
                /* error */
                break;
            }

            PyList_Append(MemberList, MemberName);
            PyDict_SetItem(MemberDictionary, MemberName, accessor);
        }
        while(0);
    }

    /*
     * decrement all refcounts of temporary objects
     */
    Py_XDECREF(accessor);
    Py_XDECREF(MemberName);
}

PyTypeObject *EdsLib_Python_DatabaseEntry_GetFromEdsId(PyObject *obj, EdsLib_Id_t EdsId)
{
    /* Sanity check on object */
    if (obj == NULL || !PyObject_TypeCheck(obj, &EdsLib_Python_DatabaseType))
    {
        PyErr_SetObject(PyExc_TypeError, obj);
        return NULL;
    }

    return EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl((EdsLib_Python_Database_t *)obj, EdsId);
}

PyTypeObject *EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl(EdsLib_Python_Database_t *refdb, EdsLib_Id_t EdsId)
{
    union obj
    {
        EdsLib_Python_DatabaseEntry_t dbent;
        PyTypeObject typeobj;
        PyObject pyobj;
    } *selfptr = NULL;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    PyObject *weakref = NULL;
    PyObject *edsidval = NULL;
    PyObject *tempargs = NULL;
    PyObject *typename = NULL;
    PyObject *typedict = NULL;
    PyObject *typelist = NULL;
    PyTypeObject *basetype = NULL;

    do
    {
        edsidval = PyLong_FromUnsignedLong(EdsId);
        if (edsidval == NULL)
        {
            break;
        }

        /*
         * To avoid wasting resources, type objects are only created on demand.
         * First check if the db cache object already contains an instance for this ID
         * note: PyDict_GetItem returns borrowed reference
         */
        weakref = PyDict_GetItem(refdb->TypeCache, edsidval);
        if (weakref != NULL)
        {
            selfptr = (union obj*)PyWeakref_GetObject(weakref); /* borrowed ref */
            if (Py_TYPE(selfptr) == &EdsLib_Python_DatabaseEntryType)
            {
                Py_INCREF(selfptr);
                break;
            }
            /* weakref expired, needs to be recreated */
            selfptr = NULL;
        }

        /*
         * Obtain the type info, which also serves to validate the EdsId.
         * If the EdsId is not valid, the TypeInfo.ElemType will be returned as NONE,
         * which in turn will result in the basetype being NULL.
         */
        EdsLib_DataTypeDB_GetTypeInfo(refdb->GD, EdsId, &TypeInfo);

        /*
         * enumerate the sub-entities for arrays and containers
         * for arrays the number of sub entities is always the same.
         * for containers it is necessary to iterate, as to include any hidden
         * basetype entries in the count (the db typeinfo does not reflect this)
         */
        if (TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY)
        {
            basetype = &EdsLib_Python_ObjectArrayType;
        }
        else if (TypeInfo.ElemType == EDSLIB_BASICTYPE_CONTAINER)
        {
            basetype = &EdsLib_Python_ObjectContainerType;
        }
        else if (TypeInfo.ElemType == EDSLIB_BASICTYPE_BINARY)
        {
            basetype = &EdsLib_Python_ObjectScalarType;
        }
        else if (TypeInfo.ElemType == EDSLIB_BASICTYPE_FLOAT ||
                TypeInfo.ElemType == EDSLIB_BASICTYPE_SIGNED_INT ||
                TypeInfo.ElemType == EDSLIB_BASICTYPE_UNSIGNED_INT)
        {
            basetype = &EdsLib_Python_ObjectNumberType;
        }
        else
        {
            PyErr_Format(PyExc_ValueError, "Invalid EDS type");
            break;
        }

        /* Create a dictionary for the new type (always needed, even if ends up empty) */
        typedict = PyDict_New();
        if (typedict == NULL)
        {
            break;
        }

        /* populate the dictionary, but only for compound types */
        if (TypeInfo.NumSubElements > 0)
        {
            typelist = PyList_New(0);
            if (typelist == NULL)
            {
                break;
            }
            tempargs = PyTuple_Pack(3, refdb, typedict, typelist);
            if (tempargs == NULL)
            {
                break;
            }

            EdsLib_DisplayDB_IterateBaseEntities(refdb->GD, EdsId,
                    EdsLib_Python_DatabaseEntry_build_dict_callback, tempargs);
            Py_DECREF(tempargs);
            tempargs = NULL;

            if (PyErr_Occurred())
            {
                break;
            }
        }

        /*
         * For the type name, use a simplified representation that
         * only uses dot separators.  This is because Python internally
         * expects certain constraints regarding a type name.
         */
        typename = PyUnicode_FromFormat("%s.%U.%s.%s",
                EdsLib_Python_DatabaseEntryType.tp_name,
                refdb->DbName,
                EdsLib_DisplayDB_GetNamespace(refdb->GD, EdsId),
                EdsLib_DisplayDB_GetBaseName(refdb->GD, EdsId));

        if (typename == NULL)
        {
            break;
        }

#if PY_MAJOR_VERSION < 3
        /* In older python2, the name must be a String aka bytes, not unicode */
        {
            PyObject *temp = PyUnicode_AsASCIIString(typename);
            if (temp == NULL)
            {
                break;
            }
            Py_DECREF(typename);
            typename = temp;
        }
#endif

        tempargs = Py_BuildValue("(O(O)O)", typename, basetype, typedict);
        if (tempargs == NULL)
        {
            break;
        }

        /*
         * Call the "new" function from the base type to properly
         * create a heaptype object (it is complicated)
         */
        selfptr = (union obj*)EdsLib_Python_DatabaseEntryType.tp_base->tp_new(&EdsLib_Python_DatabaseEntryType, tempargs, NULL);
        Py_DECREF(tempargs);
        tempargs = NULL;
        if (selfptr == NULL)
        {
            break;
        }

        Py_INCREF(refdb);
        selfptr->dbent.EdsDb = refdb;
        selfptr->dbent.EdsId = EdsId;
        selfptr->dbent.BaseName = PyUnicode_FromFormat("%s", EdsLib_DisplayDB_GetBaseName(refdb->GD, EdsId));
        selfptr->dbent.EdsTypeName = PyUnicode_FromFormat("%s/%s",
                EdsLib_DisplayDB_GetNamespace(refdb->GD, EdsId),
                EdsLib_DisplayDB_GetBaseName(refdb->GD, EdsId));

        EdsLib_Python_DatabaseEntry_GetFormatCodes(selfptr->dbent.FormatInfo, refdb, EdsId);

        /* "steal" the reference to typelist, if valid */
        selfptr->dbent.SubEntityList = typelist;
        typelist = NULL;

        /* Create a weak reference to store in the local cache in case this
         * type is needed again. */
        weakref = PyWeakref_NewRef(&selfptr->pyobj, NULL);
        if (weakref == NULL)
        {
            Py_DECREF(selfptr);
            selfptr = NULL;
            break;
        }

        PyDict_SetItem(refdb->TypeCache, edsidval, weakref);
        Py_DECREF(weakref);
        weakref = NULL;

    }
    while(0);

    /* decrement refcount for all temporary objects created */
    Py_XDECREF(edsidval);
    Py_XDECREF(typename);
    Py_XDECREF(typedict);
    Py_XDECREF(typelist);
    Py_XDECREF(tempargs);

    return &selfptr->typeobj;
}

static int EdsLib_Python_DatabaseEntry_traverse(PyObject *obj, visitproc visit, void *arg)
{
    EdsLib_Python_DatabaseEntry_t *self = (EdsLib_Python_DatabaseEntry_t *)obj;
    Py_VISIT(self->EdsDb);
    Py_VISIT(self->BaseName);
    Py_VISIT(self->EdsTypeName);
    Py_VISIT(self->SubEntityList);

    return EdsLib_Python_DatabaseEntryType.tp_base->tp_traverse(obj, visit, arg);
}

static int EdsLib_Python_DatabaseEntry_clear(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *self = (EdsLib_Python_DatabaseEntry_t *)obj;
    Py_CLEAR(self->EdsDb);
    Py_CLEAR(self->BaseName);
    Py_CLEAR(self->EdsTypeName);
    Py_CLEAR(self->SubEntityList);

    return EdsLib_Python_DatabaseEntryType.tp_base->tp_clear(obj);
}

static PyObject *EdsLib_Python_DatabaseEntry_new(PyTypeObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject *arg1;
    PyObject *arg2;
    EdsLib_Python_Database_t *refdb = NULL;
    PyObject *result = NULL;
    PyObject *tempargs = NULL;
    EdsLib_Id_t EdsId = EDSLIB_ID_INVALID;

    /*
     * Database entries are constructed from two values:
     *   - A database
     *   - An index into the database
     *
     * Databases are natively of the EdsLib_Python_Database_t type
     * Indices are natively unsigned integers (EdsLib_Id_t specifically)
     *
     * However, either value can alternatively be supplied as a string,
     * in which case it can be looked up and converted to the native value.
     */
    if (!PyArg_UnpackTuple(args, "DatabaseEntry_new", 2, 2, &arg1, &arg2))
    {
        return NULL;
    }

    do
    {
        if (Py_TYPE(arg1) == &EdsLib_Python_DatabaseType)
        {
            refdb = (EdsLib_Python_Database_t*)arg1;
            Py_INCREF(refdb);
        }
        else
        {
            tempargs = PyTuple_Pack(1, arg1);
            if (tempargs == NULL)
            {
                break;
            }
            refdb = (EdsLib_Python_Database_t*)PyObject_Call((PyObject*)&EdsLib_Python_DatabaseType, tempargs, NULL);
            Py_DECREF(tempargs);
            tempargs = NULL;
        }

        if (refdb == NULL)
        {
            break;
        }

        if (PyNumber_Check(arg2))
        {
            tempargs = PyNumber_Long(arg2);
            if (tempargs == NULL)
            {
                break;
            }

            EdsId = PyLong_AsUnsignedLong(tempargs);
            Py_DECREF(tempargs);
            tempargs = NULL;
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

            EdsId = EdsLib_DisplayDB_LookupTypeName(refdb->GD, PyBytes_AsString(tempargs));
            Py_DECREF(tempargs);
            tempargs = NULL;
        }

        result = (PyObject*)EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl(refdb, EdsId);
    }
    while(0);

    /* decrement refcount for all temporary objects created */
    Py_XDECREF(refdb);
    Py_XDECREF(tempargs);

    return result;
}

static int EdsLib_Python_DatabaseEntry_init(PyObject *obj, PyObject *args, PyObject *kwds)
{
    EdsLib_Python_DatabaseEntry_t *self = (EdsLib_Python_DatabaseEntry_t *)obj;
    PyObject *subargs = PyTuple_Pack(1, self->type_base.ht_name);

    if (subargs == NULL)
    {
        return -1;
    }

    EdsLib_Python_DatabaseEntryType.tp_base->tp_init(obj, subargs, NULL);

    Py_DECREF(subargs);

    return 0;
}

static void EdsLib_Python_DatabaseEntry_dealloc(PyObject * obj)
{
    EdsLib_Python_DatabaseEntry_t *self = (EdsLib_Python_DatabaseEntry_t *)obj;
    Py_XDECREF(self->EdsDb);
    Py_XDECREF(self->BaseName);
    Py_XDECREF(self->EdsTypeName);
    Py_XDECREF(self->SubEntityList);

    /*
     * Call the base type dealloc in case there is complicated logic
     * (for simple objects this just calls tp_free(), but it might do more)
     */
    EdsLib_Python_DatabaseEntryType.tp_base->tp_dealloc(obj);
}

static Py_ssize_t EdsLib_Python_DatabaseEntry_len(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *self = (EdsLib_Python_DatabaseEntry_t*)obj;
    Py_ssize_t result;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    EdsLib_DataTypeDB_GetTypeInfo(self->EdsDb->GD, self->EdsId, &TypeInfo);
    if (TypeInfo.ElemType == EDSLIB_BASICTYPE_CONTAINER &&
            self->SubEntityList != NULL)
    {
        /*
         * containers are a bit special in that they may have unnamed
         * base type entities, so go by the subentity list length instead.
         */
        result = PyList_Size(self->SubEntityList);
    }
    else
    {
        result = TypeInfo.NumSubElements;
    }

    return result;
}

static PyObject *EdsLib_Python_DatabaseEntry_seq_item(PyObject *obj, Py_ssize_t idx)
{
    EdsLib_Python_DatabaseEntry_t *self = (EdsLib_Python_DatabaseEntry_t*)obj;
    EdsLib_DataTypeDB_EntityInfo_t EntityInfo;
    PyObject *attribute = NULL;
    PyObject *result = NULL;

    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    EdsLib_DataTypeDB_GetTypeInfo(self->EdsDb->GD, self->EdsId, &TypeInfo);

    /*
     * for containers, grab the entry name from the list, and then
     * return the corresponding attribute by that name.  This two advantages:
     *  - base entities are transparently included
     *  - accessors are not re-created
     */
    if (TypeInfo.NumSubElements == 0)
    {
        PyErr_Format(PyExc_TypeError, "Attempt to index \'%s\' which is not an EDS sequence type",
                self->type_base.ht_type.tp_name);
    }
    else if (TypeInfo.ElemType == EDSLIB_BASICTYPE_CONTAINER &&
            self->SubEntityList != NULL)
    {
        attribute = PyList_GetItem(self->SubEntityList, idx);
        if (attribute == NULL)
        {
            return NULL;
        }
        Py_INCREF(attribute);
        result = PyObject_GetAttr(obj, attribute);
    }
    else if (idx < 0 || idx >= TypeInfo.NumSubElements)
    {
        PyErr_Format(PyExc_IndexError, "Attempt to access index %zd out of range", idx);
    }
    else if (EdsLib_DataTypeDB_GetMemberByIndex(self->EdsDb->GD, self->EdsId, idx, &EntityInfo) != EDSLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "Error looking up array member info");
    }
    else
    {
        result = EdsLib_Python_ElementAccessor_CreateFromEntityInfo(&EntityInfo);
    }
    Py_XDECREF(attribute);

    return result;
}

static PyObject * EdsLib_Python_DatabaseEntry_map_subscript(PyObject *obj, PyObject *subscr)
{
    PyObject *result;

    if (PyNumber_Check(subscr))
    {
        result = EdsLib_Python_DatabaseEntry_seq_item(obj, PyNumber_AsSsize_t(subscr, NULL));
    }
    else
    {
        result = PyObject_GetAttr(obj, subscr);
    }

    return result;
}

static PyObject *   EdsLib_Python_DatabaseEntry_repr(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *self = (EdsLib_Python_DatabaseEntry_t *)obj;
    PyObject *result = NULL;

    result = PyUnicode_FromFormat("%s(%R,%R)",
            obj->ob_type->tp_name, self->EdsDb->DbName, self->EdsTypeName);

    return result;
}

Py_ssize_t EdsLib_Python_DatabaseEntry_GetMaxSize(PyTypeObject* objtype)
{
    EdsLib_Python_DatabaseEntry_t *dbent = (EdsLib_Python_DatabaseEntry_t *)objtype;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;

    if (Py_TYPE(objtype) != &EdsLib_Python_DatabaseEntryType)
    {
        PyErr_SetObject(PyExc_TypeError, (PyObject*)objtype);
        return -1;
    }

    if (EdsLib_DataTypeDB_GetDerivedInfo(dbent->EdsDb->GD, dbent->EdsId, &DerivInfo) != EDSLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "Error getting derived type info");
        return -1;
    }

    return DerivInfo.MaxSize.Bytes;
}

static PyObject * EdsLib_Python_DatabaseEntry_iter(PyObject *obj)
{
    EdsLib_Python_DatabaseEntry_t *dbent = (EdsLib_Python_DatabaseEntry_t *)obj;
    EdsLib_Python_EnumerationIterator_t *EnumIter;
    EdsLib_Python_ContainerIterator_t *ContainerIter;
    EdsLib_DisplayHint_t DisplayHint;
    PyObject *result;

    /* sanity check that the db entry is an EDS entry is iterable (Container or Enumeration) */
    DisplayHint = EdsLib_DisplayDB_GetDisplayHint(dbent->EdsDb->GD, dbent->EdsId);
    switch(DisplayHint)
    {
    case EDSLIB_DISPLAYHINT_ENUM_SYMTABLE:
        EnumIter = PyObject_GC_New(EdsLib_Python_EnumerationIterator_t, &EdsLib_Python_EnumEntryIteratorType);

        if (EnumIter == NULL)
        {
            return NULL;
        }

        EnumIter->Index = 0;
        Py_INCREF(obj);
        EnumIter->refobj = obj;

        result = (PyObject *)EnumIter;
        PyObject_GC_Track(result);

        break;
    case EDSLIB_DISPLAYHINT_MEMBER_NAMETABLE:
        /* sanity check that the db entry has a valid subentry list (it should for all containers) */
        if (dbent->SubEntityList == NULL || !PyList_Check(dbent->SubEntityList))
        {
            return PyErr_Format(PyExc_TypeError, "%s is not an mappable EDS type", Py_TYPE(obj)->tp_name);
        }
    	ContainerIter = PyObject_GC_New(EdsLib_Python_ContainerIterator_t, &EdsLib_Python_ContainerEntryIteratorType);
    	if (ContainerIter == NULL)
    	{
    		return NULL;
    	}
        ContainerIter->Position = 0;
        Py_INCREF(obj);
        ContainerIter->refobj = obj;
        result = (PyObject *)ContainerIter;
        PyObject_GC_Track(result);
    	break;
    default:
        return PyErr_Format(PyExc_TypeError, "%s is not an EDS Iterable type (Container or Enumeration)", Py_TYPE(obj)->tp_name);
    }

    return result;
}


static void EdsLib_Python_EnumEntryIterator_dealloc(PyObject * obj)
{
    EdsLib_Python_EnumerationIterator_t *self = (EdsLib_Python_EnumerationIterator_t*)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int EdsLib_Python_EnumEntryIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    EdsLib_Python_EnumerationIterator_t *self = (EdsLib_Python_EnumerationIterator_t*)obj;
    Py_VISIT(self->refobj);
    return 0;
}

static int EdsLib_Python_EnumEntryIterator_clear(PyObject *obj)
{
    EdsLib_Python_EnumerationIterator_t *self = (EdsLib_Python_EnumerationIterator_t*)obj;
    Py_CLEAR(self->refobj);
    return 0;
}

static PyObject *EdsLib_Python_EnumEntryIterator_iternext(PyObject *obj)
{
	EdsLib_Python_EnumerationIterator_t *self = (EdsLib_Python_EnumerationIterator_t*)obj;
    EdsLib_Python_DatabaseEntry_t *dbent = NULL;
    const char * Label;
    char LabelBuffer[64];
    intmax_t Value;
    PyObject *key = NULL;
    PyObject *value = NULL;
    PyObject *result = NULL;

    do
    {
    	if (self->refobj == NULL)
        {
            break;
        }

        dbent = (EdsLib_Python_DatabaseEntry_t *)self->refobj;
        Label = EdsLib_DisplayDB_GetEnumLabelByIndex(dbent->EdsDb->GD, dbent->EdsId, self->Index, LabelBuffer, sizeof(LabelBuffer));
        Value = EdsLib_DisplayDB_GetEnumValueByIndex(dbent->EdsDb->GD, dbent->EdsId, self->Index);

        if (strcmp(Label,"UNDEFINED") != 0)
        {
            key = PyUnicode_FromString(Label);
            value = PyLong_FromLong(Value);

            if ((key == NULL) || (value == NULL))
            {
            	/* end */
            	Py_CLEAR(self->refobj);
            	break;
            }

            ++self->Index;
            result = PyTuple_Pack(2, key, value);
        }
    }
    while(0);

    Py_XDECREF(key);
    Py_XDECREF(value);

    return result;
}

static void EdsLib_Python_ContainerEntryIterator_dealloc(PyObject * obj)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->refobj);
    PyObject_GC_Del(self);
}

static int EdsLib_Python_ContainerEntryIterator_traverse(PyObject *obj, visitproc visit, void *arg)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    Py_VISIT(self->refobj);
    return 0;
}

static int EdsLib_Python_ContainerEntryIterator_clear(PyObject *obj)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    Py_CLEAR(self->refobj);
    return 0;
}

static PyObject *EdsLib_Python_ContainerEntryIterator_iternext(PyObject *obj)
{
    EdsLib_Python_ContainerIterator_t *self = (EdsLib_Python_ContainerIterator_t*)obj;
    EdsLib_Python_DatabaseEntry_t *dbent = NULL;

    const char *keystr;

    EdsLib_DataTypeDB_EntityInfo_t CompInfo;
    PyObject *sub_obj = NULL;
    EdsLib_Python_DatabaseEntry_t *sub_dbent = NULL;
    PyObject *key = NULL;
    PyObject *dbname = NULL;
    PyObject *entry = NULL;
    PyObject *result = NULL;

    do
    {
    	if (self->refobj == NULL)
        {
            break;
        }

        dbent = (EdsLib_Python_DatabaseEntry_t *)self->refobj;
        if (self->Position < PyList_GET_SIZE(dbent->SubEntityList))
        {
            key = PyList_GET_ITEM(dbent->SubEntityList, self->Position); /* borrowed ref */
        }

        if (key == NULL)
        {
            /* end */
            Py_CLEAR(self->refobj);
            break;
        }
        Py_INCREF(key);
        keystr = (const char *)PyUnicode_DATA(key);

        if (EdsLib_DisplayDB_LocateSubEntity(dbent->EdsDb->GD, dbent->EdsId, keystr, &CompInfo) == EDSLIB_SUCCESS)
        {
        	sub_obj = (PyObject *)EdsLib_Python_DatabaseEntry_GetFromEdsId_Impl(dbent->EdsDb, CompInfo.EdsId);
        	sub_dbent = (EdsLib_Python_DatabaseEntry_t *) sub_obj;
        	dbname = sub_dbent->EdsDb->DbName;
        	entry = sub_dbent->EdsTypeName;
        }

        if ((entry == NULL) || (dbname == NULL))
        {
            break;
        }
        ++self->Position;
        Py_INCREF(dbname);
        Py_INCREF(entry);
        result = PyTuple_Pack(3, key, dbname, entry);
    }
    while(0);

    Py_XDECREF(key);
    Py_XDECREF(dbname);
    Py_XDECREF(entry);

    return result;
}
