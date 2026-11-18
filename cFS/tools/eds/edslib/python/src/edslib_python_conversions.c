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
 * \file     edslib_python_conversions.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
 **   Implement conversion routines for EDS/Python objects
 */

#include "edslib_python_internal.h"

/*
 * A local state-tracking object that pairs an EDS descriptor with a python object
 * This is used for to/from conversion between EDS.  It is required to pass both objects
 * through an EdsLib Callback parameter which is a single pointer object.
 */
typedef struct
{
    EdsLib_Binding_DescriptorObject_t desc;
    PyObject                         *pyobj;
} EdsLib_Python_ObjectConversionState_t;

/*
 * These "pair" functions are for internal use and accept a local pair object.
 * They may be used recursively so they accept a pair directly, which lessens stack usage.
 * They do minimal error checking, since they are only called locally where errors are already checked.
 * Also Note that the callbacks also require a pair via the generic void* argument.
 */
static void EdsLib_Python_ConvertEdsObjectToPythonImpl(EdsLib_Python_ObjectConversionState_t *ConvPair);
static void EdsLib_Python_ConvertPythonToEdsObjectImpl(EdsLib_Python_ObjectConversionState_t *ConvPair);

static int EdsLib_Python_InitBindingDescriptorObject(EdsLib_Python_ObjectBase_t        *self,
                                                     EdsLib_Binding_DescriptorObject_t *desc)
{
    EdsLib_Python_DatabaseEntry_t *dbent;
    int32_t                        Status;

    /*
     * EdsLib Binding descriptor objects can only be generated from
     * objects which are based on an actual EDS database entry.  This
     * excludes the dynamic array types and any other "virtual" data type
     * supported by the python library.
     */
    if (Py_TYPE(Py_TYPE(self)) != &EdsLib_Python_DatabaseEntryType)
    {
        PyErr_Format(PyExc_TypeError, "%s is not an EDS type", Py_TYPE(self)->tp_name);
        memset(desc, 0, sizeof(*desc));
        return 0;
    }

    dbent = (EdsLib_Python_DatabaseEntry_t *)Py_TYPE(self);

    Status = EdsLib_DataTypeDB_GetTypeInfo(dbent->EdsDb->GD, dbent->EdsId, &desc->TypeInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "EdsLib error %d getting type info", (int)Status);
        return 0;
    }

    desc->GD        = dbent->EdsDb->GD;
    desc->EdsId     = dbent->EdsId;
    desc->Offset    = self->Offset;
    desc->Length    = self->TotalLength;
    desc->BufferPtr = NULL;

    return 1;
}

static void EdsLib_Python_ConvertEdsMembers_Callback(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc)
{
    EdsLib_Python_ObjectConversionState_t *ParentConv = Arg;
    EdsLib_Python_ObjectConversionState_t  subpair;

    /* Any conversion error should have set an exception/error context */
    if (PyErr_Occurred() == NULL)
    {
        EdsLib_Binding_InitSubObject(&subpair.desc, &ParentConv->desc, &ParamDesc->EntityInfo);
        subpair.pyobj = NULL;

        EdsLib_Python_ConvertEdsObjectToPythonImpl(&subpair);

        if (subpair.pyobj != NULL)
        {
            /*
             * "PyList_SetItem" steals the reference,
             * but "PyDict_SetItemString" does not.
             */
            if (ParentConv->desc.TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY)
            {
                /* If an array then use the numeric index.
                 * The python object should always be a List in this case.
                 * This steals a reference - no need to DECREF */
                PyList_SET_ITEM(ParentConv->pyobj, ParamDesc->SeqNum, subpair.pyobj);
            }
            else if (ParamDesc->FullName != NULL)
            {
                /* If there is a named index then use it
                 * The python object should always be a Dictionary in this case */
                PyDict_SetItemString(ParentConv->pyobj, (char *)ParamDesc->FullName, subpair.pyobj);
                Py_DECREF(subpair.pyobj);
            }
            else
            {
                /* something wrong - cannot store result, discard it */
                Py_DECREF(subpair.pyobj);
            }
        }
    }
}

/*
 * internal function, may be called recursively
 */
static void EdsLib_Python_ConvertEdsObjectToPythonImpl(EdsLib_Python_ObjectConversionState_t *ConvPair)
{
    if (ConvPair->desc.TypeInfo.NumSubElements == 0)
    {
        ConvPair->pyobj = EdsLib_Python_ConvertEdsScalarToPython(&ConvPair->desc);
    }
    else if (ConvPair->desc.TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY)
    {
        ConvPair->pyobj = PyList_New(ConvPair->desc.TypeInfo.NumSubElements);
    }
    else if (ConvPair->desc.TypeInfo.ElemType == EDSLIB_BASICTYPE_CONTAINER
             || ConvPair->desc.TypeInfo.ElemType == EDSLIB_BASICTYPE_COMPONENT)
    {
        ConvPair->pyobj = PyDict_New();
    }
    else
    {
        PyErr_Format(PyExc_RuntimeError,
                     "Cannot map %s/%s to a python type",
                     EdsLib_DisplayDB_GetNamespace(ConvPair->desc.GD, ConvPair->desc.EdsId),
                     EdsLib_DisplayDB_GetBaseName(ConvPair->desc.GD, ConvPair->desc.EdsId));
        ConvPair->pyobj = NULL;
    }

    /*
     * If the python object was created and there are sub-objects, then
     * iterate through all EDS sub-objects to initialize the members.
     */
    if (ConvPair->pyobj != NULL && ConvPair->desc.TypeInfo.NumSubElements > 0)
    {
        EdsLib_DisplayDB_IterateBaseEntities(ConvPair->desc.GD,
                                             ConvPair->desc.EdsId,
                                             EdsLib_Python_ConvertEdsMembers_Callback,
                                             ConvPair);
    }
}

static int EdsLib_Python_CopyEdsObjectPacked(EdsLib_Python_ObjectConversionState_t *ConvPair)
{
    int32_t Status;

    if (!PyObject_TypeCheck(ConvPair->pyobj, &EdsLib_Python_PackedObjectType))
    {
        /* not a packed EDS object -- cannot copy */
        return 0;
    }

    Status = EdsLib_Binding_InitFromPackedBuffer(&ConvPair->desc,
                                                 PyBytes_AsString(ConvPair->pyobj),
                                                 PyBytes_Size(ConvPair->pyobj));

    if (Status != EDSLIB_SUCCESS)
    {
        PyErr_Format(PyExc_RuntimeError, "Error %d unpacking bytes object", (int)Status);
    }

    return 1;
}

static int EdsLib_Python_CopyEdsObjectDirect(EdsLib_Python_ObjectConversionState_t *ConvPair)
{
    EdsLib_Binding_DescriptorObject_t srcobj;

    if (!PyObject_TypeCheck(ConvPair->pyobj, &EdsLib_Python_ObjectBaseType))
    {
        /* Not an EDS object -- direct copy not possible */
        return 0;
    }

    if (!EdsLib_Python_SetupObjectDesciptor((EdsLib_Python_ObjectBase_t *)ConvPair->pyobj, &srcobj, PyBUF_SIMPLE))
    {
        return 0;
    }

    if (!EdsLib_Binding_CheckEdsObjectsCompatible(&ConvPair->desc, &srcobj))
    {
        /* Objects not compatible -- direct copy not possible */
        EdsLib_Python_ReleaseObjectDesciptor(&srcobj);
        return 0;
    }

    /* In this case a direct memcpy can be done - no conversions needed,
     * but they still may have a base/derived relationship so they might be
     * different sizes.  Copy based on the _buffer_ sizes which will include
     * any trailing data. */
    size_t DstSize = EdsLib_Binding_GetNativeSize(&ConvPair->desc);
    size_t SrcSize = EdsLib_Binding_GetNativeSize(&srcobj);
    char  *DstPtr  = EdsLib_Binding_GetNativeObject(&ConvPair->desc);

    if (DstSize < SrcSize)
    {
        SrcSize = DstSize;
    }

    memcpy(DstPtr, EdsLib_Binding_GetNativeObject(&srcobj), SrcSize);

    if (SrcSize < DstSize)
    {
        memset(&DstPtr[SrcSize], 0, DstSize - SrcSize);
    }

    EdsLib_Python_ReleaseObjectDesciptor(&srcobj);

    return 1;
}

static void EdsLib_Python_ConvertPythonSubObjectToEdsImpl(EdsLib_Binding_DescriptorObject_t *ParentDesc,
                                                          uint16_t                           SubIndex,
                                                          PyObject                          *MemberValue)
{
    EdsLib_DataTypeDB_EntityInfo_t        MemberInfo;
    EdsLib_Python_ObjectConversionState_t subpair;
    int32_t                               EdsStatus;

    EdsStatus = EdsLib_DataTypeDB_GetMemberByIndex(ParentDesc->GD, ParentDesc->EdsId, SubIndex, &MemberInfo);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        /* This means the subindex is not valid for this object */
        PyErr_Format(PyExc_IndexError, "Invalid Index: %u, Status=%d", (unsigned int)SubIndex, (int)EdsStatus);
    }
    else
    {
        EdsLib_Binding_InitSubObject(&subpair.desc, ParentDesc, &MemberInfo);
        subpair.pyobj = MemberValue;
        EdsLib_Python_ConvertPythonToEdsObjectImpl(&subpair);
    }
}

/*
 * internal function, may be called recursively
 */
static void EdsLib_Python_ConvertPythonSequenceToEdsObjImpl(EdsLib_Python_ObjectConversionState_t *ConvPair)
{
    Py_ssize_t SeqSize;
    Py_ssize_t idx;
    PyObject  *MemberValue;

    /* Any conversion error should have set an exception/error context.
     * To avoid triggering multiple exceptions, this stops after the first error */
    SeqSize = PySequence_Size(ConvPair->pyobj);
    for (idx = 0; PyErr_Occurred() == NULL && idx < SeqSize; ++idx)
    {
        MemberValue = PySequence_GetItem(ConvPair->pyobj, idx);
        if (MemberValue == NULL)
        {
            /* not expected */
            break;
        }

        EdsLib_Python_ConvertPythonSubObjectToEdsImpl(&ConvPair->desc, idx, MemberValue);
        Py_DECREF(MemberValue);
    }
}

/*
 * internal function, may be called recursively
 */
static void EdsLib_Python_ConvertPythonMappingToEdsObjImpl(EdsLib_Python_ObjectConversionState_t *ConvPair)
{
    PyObject  *MemberList;
    PyObject  *MemberKey;
    PyObject  *MemberValue;
    Py_ssize_t MapSize;
    Py_ssize_t idx;
    int32_t    EdsStatus;
    uint16_t   SubIndex;

    MemberList = PyMapping_Keys(ConvPair->pyobj);
    if (MemberList != NULL)
    {
        MapSize = PyList_GET_SIZE(MemberList);
        /* Any conversion error should have set an exception/error context.
         * To avoid triggering multiple exceptions, this stops after the first error */
        for (idx = 0; PyErr_Occurred() == NULL && idx < MapSize; ++idx)
        {
            /* NOTE: this gets a borrowed ref */
            MemberKey   = PyList_GET_ITEM(MemberList, idx);
            MemberValue = PyObject_GetItem(ConvPair->pyobj, MemberKey);
            if (MemberValue == NULL)
            {
                /* not expected */
                break;
            }

            /* Do an EDS lookup but make sure the string is plain ASCII */
            /* this drops the borrowed ref and replaces it with a strong ref */
            if (PyUnicode_Check(MemberKey))
            {
                MemberKey = PyUnicode_AsASCIIString(MemberKey);
            }
            else
            {
                MemberKey = PyObject_ASCII(MemberKey);
            }

            if (MemberKey != NULL)
            {
                EdsStatus = EdsLib_DisplayDB_GetIndexByName(ConvPair->desc.GD,
                                                            ConvPair->desc.EdsId,
                                                            PyBytes_AsString(MemberKey),
                                                            &SubIndex);
                if (EdsStatus != EDSLIB_SUCCESS)
                {
                    /* This means the python object contained a key that does not exist */
                    PyErr_Format(PyExc_KeyError, "Key does not exist in EDS object: %R", MemberKey);
                }
                else
                {
                    EdsLib_Python_ConvertPythonSubObjectToEdsImpl(&ConvPair->desc, SubIndex, MemberValue);
                }

                Py_DECREF(MemberKey);
            }

            Py_DECREF(MemberValue);
        }

        Py_DECREF(MemberList);
    }
}

/*
 * internal function, may be called recursively
 */
static void EdsLib_Python_ConvertPythonToEdsObjectImpl(EdsLib_Python_ObjectConversionState_t *ConvPair)
{
    /*
     * optimization: if the python object is itself an EDS object
     * then check if the data can be copied directly between them.
     * If this is successful then nothing else is needed.
     *
     * For all other types of python objects, call the regular conversion routine.
     */
    if (ConvPair->pyobj == Py_None || EdsLib_Python_CopyEdsObjectPacked(ConvPair)
        || EdsLib_Python_CopyEdsObjectDirect(ConvPair))
    {
        /* no-op */
    }
    else if (PyList_Check(ConvPair->pyobj) || PyTuple_Check(ConvPair->pyobj))
    {
        /* Note that this is specifically checking for List or Tuple here because
         * the PySequence check is too broad; it also includes strings and Bytes
         * objects which we do NOT want to interpret in this way */
        EdsLib_Python_ConvertPythonSequenceToEdsObjImpl(ConvPair);
    }
    else if (PyAnySet_Check(ConvPair->pyobj) || PyDict_Check(ConvPair->pyobj))
    {
        /* As for sequences, the PyMapping_Check is too broad and will match
         * unicode objects as well */
        EdsLib_Python_ConvertPythonMappingToEdsObjImpl(ConvPair);
    }
    else
    {
        /* Note - This function will raise error context if there is an issue */
        EdsLib_Python_ConvertPythonToEdsScalar(&ConvPair->desc, ConvPair->pyobj);
    }
}

PyObject *EdsLib_Python_ConvertEdsScalarToPython(EdsLib_Binding_DescriptorObject_t *edsobj)
{
    EdsLib_DisplayHint_t        DispHint;
    const char                 *Ptr;
    size_t                      ActualSize;
    EdsLib_GenericValueBuffer_t ValueBuff;
    char                        StringBuffer[256];
    PyObject                   *result;

    Ptr        = EdsLib_Binding_GetNativeObject(edsobj);
    ActualSize = EdsLib_Binding_GetNativeSize(edsobj);
    DispHint   = EdsLib_DisplayDB_GetDisplayHint(edsobj->GD, edsobj->EdsId);
    result     = NULL;

    if (edsobj->TypeInfo.ElemType == EDSLIB_BASICTYPE_BINARY)
    {
        if (DispHint == EDSLIB_DISPLAYHINT_STRING)
        {
            /* Strings should stop at the first null */
            size_t StrSize = 0;
            while (StrSize < ActualSize && Ptr[StrSize] != 0)
            {
                ++StrSize;
            }
            result = PyUnicode_FromStringAndSize(Ptr, StrSize);
        }
        else
        {
            /* Non-string binary data should include all, including embedded nulls */
            result = PyBytes_FromStringAndSize(Ptr, ActualSize);
        }
    }
    else
    {
        if (DispHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE
            && EdsLib_Scalar_ToString(edsobj->GD, edsobj->EdsId, StringBuffer, sizeof(StringBuffer), Ptr)
                   == EDSLIB_SUCCESS)
        {
            result = PyUnicode_FromString(StringBuffer);
        }
        else
        {
            EdsLib_Binding_LoadValue(edsobj, &ValueBuff);

            switch (ValueBuff.ValueType)
            {
                case EDSLIB_BASICTYPE_SIGNED_INT:
                    if (DispHint == EDSLIB_DISPLAYHINT_BOOLEAN)
                    {
                        /* preserve the boolean nature of the integer field */
                        result = PyBool_FromLong(ValueBuff.Value.SignedInteger);
                    }
                    else
                    {
                        result = PyLong_FromLongLong(ValueBuff.Value.SignedInteger);
                    }
                    break;
                case EDSLIB_BASICTYPE_UNSIGNED_INT:
                    if (DispHint == EDSLIB_DISPLAYHINT_BOOLEAN)
                    {
                        /* preserve the boolean nature of the integer field */
                        result = PyBool_FromLong(ValueBuff.Value.UnsignedInteger);
                    }
                    else
                    {
                        result = PyLong_FromUnsignedLongLong(ValueBuff.Value.UnsignedInteger);
                    }
                    break;
                case EDSLIB_BASICTYPE_FLOAT:
                    result = PyFloat_FromDouble(ValueBuff.Value.FloatingPoint);
                    break;
                default:
                    break;
            }
        }
    }

    return result;
}

bool EdsLib_Python_ConvertPythonToEdsScalar(EdsLib_Binding_DescriptorObject_t *edsobj, PyObject *pyobj)
{
    char                       *Ptr;
    size_t                      ActualSize;
    EdsLib_GenericValueBuffer_t ValueBuff;
    PyObject                   *ReprObj   = NULL;
    PyObject                   *BytesObj  = NULL;
    PyObject                   *NumberObj = NULL;
    bool                        success   = false;

    Ptr        = EdsLib_Binding_GetNativeObject(edsobj);
    ActualSize = EdsLib_Binding_GetNativeSize(edsobj);
    memset(&ValueBuff, 0, sizeof(ValueBuff));

    /*
     * First check if a string or string-like object was supplied by the caller.
     * If so, use the EdsLib string conversion routine to set the value.  This
     * routine implements basic coercions from a string into whatever value is needed.
     */
    if (PyBytes_Check(pyobj))
    {
        BytesObj = pyobj;
        Py_INCREF(BytesObj);
    }
    else if (PyUnicode_Check(pyobj))
    {
        BytesObj = PyUnicode_AsUTF8String(pyobj);
    }
#if PY_MAJOR_VERSION < 3
    /* Additional types that Python2 had but are not in Python3 */
    else if (PyString_Check(pyobj))
    {
        BytesObj = PyObject_Bytes(pyobj);
    }
    else if (PyInt_Check(pyobj))
    {
        if (edsobj->TypeInfo.ElemType == EDSLIB_BASICTYPE_UNSIGNED_INT)
        {
            ValueBuff.Value.UnsignedInteger = PyInt_AsUnsignedLongMask(pyobj);
            ValueBuff.ValueType             = EDSLIB_BASICTYPE_UNSIGNED_INT;
        }
        else
        {
            ValueBuff.Value.SignedInteger = PyInt_AsLong(pyobj);
            ValueBuff.ValueType           = EDSLIB_BASICTYPE_SIGNED_INT;
        }
    }
#endif
    else if (PyBool_Check(pyobj))
    {
        ValueBuff.Value.UnsignedInteger = (pyobj == Py_True);
        ValueBuff.ValueType             = EDSLIB_BASICTYPE_UNSIGNED_INT;
    }
    else if (PyLong_Check(pyobj))
    {
        if (edsobj->TypeInfo.ElemType == EDSLIB_BASICTYPE_UNSIGNED_INT)
        {
            ValueBuff.Value.UnsignedInteger = PyLong_AsUnsignedLongLong(pyobj);
            ValueBuff.ValueType             = EDSLIB_BASICTYPE_UNSIGNED_INT;
        }
        else
        {
            ValueBuff.Value.SignedInteger = PyLong_AsLongLong(pyobj);
            ValueBuff.ValueType           = EDSLIB_BASICTYPE_SIGNED_INT;
        }
    }
    else if (PyFloat_Check(pyobj))
    {
        ValueBuff.Value.FloatingPoint = PyFloat_AsDouble(pyobj);
        ValueBuff.ValueType           = EDSLIB_BASICTYPE_FLOAT;
    }

    /*
     * Depending on the desired/wanted data type, if the simple conversion above
     * did not produce a usable value, then try a backup approach.
     * For numbers, see if it supports the Python "number protocol"
     */
    if (ValueBuff.ValueType == EDSLIB_BASICTYPE_NONE && PyNumber_Check(pyobj))
    {
        switch (edsobj->TypeInfo.ElemType)
        {
            case EDSLIB_BASICTYPE_SIGNED_INT:
                NumberObj = PyNumber_Long(pyobj);
                if (NumberObj != NULL)
                {
                    ValueBuff.Value.SignedInteger = PyLong_AsLongLong(NumberObj);
                    ValueBuff.ValueType           = EDSLIB_BASICTYPE_SIGNED_INT;
                }
                break;
            case EDSLIB_BASICTYPE_UNSIGNED_INT:
                NumberObj = PyNumber_Long(pyobj);
                if (NumberObj != NULL)
                {
                    ValueBuff.Value.UnsignedInteger = PyLong_AsUnsignedLongLong(NumberObj);
                    ValueBuff.ValueType             = EDSLIB_BASICTYPE_UNSIGNED_INT;
                }
                break;
            case EDSLIB_BASICTYPE_FLOAT:
                NumberObj = PyNumber_Float(pyobj);
                if (NumberObj != NULL)
                {
                    ValueBuff.Value.FloatingPoint = PyFloat_AsDouble(NumberObj);
                    ValueBuff.ValueType           = EDSLIB_BASICTYPE_FLOAT;
                }
                break;
            default:
                break;
        }
    }

    /*
     * As a last resort, if all else fails, try using the Python "repr"
     * function to convert the object into a string, in the chance that
     * the EdsLib FromString function can parse it.  This should, for instance,
     * handle coercion of Python numbers into EDS strings where needed.
     */
    if (BytesObj == NULL
        && (ValueBuff.ValueType == EDSLIB_BASICTYPE_NONE || edsobj->TypeInfo.ElemType == EDSLIB_BASICTYPE_BINARY))
    {
        ReprObj = PyObject_Repr(pyobj);
        if (ReprObj != NULL)
        {
            if (PyUnicode_Check(ReprObj))
            {
                BytesObj = PyUnicode_AsUTF8String(ReprObj);
            }
            else
            {
                BytesObj = PyObject_Bytes(ReprObj);
            }
        }
    }

    /*
     * Now try to take whatever "best" value was gathered above, and
     * use it to populate the EDS data structure
     */
    if (BytesObj != NULL)
    {
        if (edsobj->TypeInfo.ElemType == EDSLIB_BASICTYPE_BINARY)
        {
            /*
             * Copy the value here --
             * it is done this way to include any embedded NUL chars
             * (the "FromString" method would truncate at the first NUL)
             */
            size_t CopySize = PyBytes_Size(BytesObj);
            if (ActualSize < CopySize)
            {
                CopySize = ActualSize;
            }
            memcpy(Ptr, PyBytes_AsString(BytesObj), CopySize);
            if (CopySize < ActualSize)
            {
                memset(&Ptr[CopySize], 0, ActualSize - CopySize);
            }
            success = true;
        }
        else if (EdsLib_Scalar_FromString(edsobj->GD, edsobj->EdsId, Ptr, PyBytes_AsString(BytesObj)) == EDSLIB_SUCCESS)
        {
            success = true;
        }
    }
    else if (ValueBuff.ValueType != EDSLIB_BASICTYPE_NONE)
    {
        if (EdsLib_Binding_StoreValue(edsobj, &ValueBuff) == EDSLIB_SUCCESS)
        {
            success = true;
        }
    }

    /* Finally, decrement the refcount of any temporary objects that
     * may have been created during the conversion process  */
    Py_XDECREF(NumberObj);
    Py_XDECREF(BytesObj);
    Py_XDECREF(ReprObj);

    if (!success)
    {
        PyErr_Format(PyExc_TypeError,
                     "Cannot initialize %s from %s",
                     EdsLib_DisplayDB_GetBaseName(edsobj->GD, edsobj->EdsId),
                     pyobj->ob_type->tp_name);
    }

    return success;
}

/*
 * Externally callable API -- do full error checking
 */
PyObject *EdsLib_Python_ConvertEdsObjectToPython(EdsLib_Python_ObjectBase_t *self)
{
    EdsLib_Python_ObjectConversionState_t ConvPair;
    EdsLib_Binding_Buffer_Content_t      *content = NULL;

    memset(&ConvPair, 0, sizeof(ConvPair));

    do
    {
        /* If there is already an error context set, do nothing */
        if (PyErr_Occurred())
        {
            break;
        }

        if (self->StorageBuf == NULL)
        {
            PyErr_SetString(PyExc_BufferError, "No buffer object");
            break;
        }

        /* Obtain a buffer window reference for the conversion */
        content = EdsLib_Python_Buffer_GetContentRef(self->StorageBuf, PyBUF_SIMPLE);
        if (content == NULL)
        {
            break;
        }

        if (!EdsLib_Python_InitBindingDescriptorObject(self, &ConvPair.desc))
        {
            break;
        }

        EdsLib_Binding_SetDescBuffer(&ConvPair.desc, content);

        /*
         * The implementation function below will loop through the internals
         * and convert as necessary.  It does not return a value directly
         * but it will set the "pyobj" member to a valid pointer if successful.
         * If unsuccessful, it will set an exception before returning.
         */
        EdsLib_Python_ConvertEdsObjectToPythonImpl(&ConvPair);
    } while (0);

    /* be sure to release window reference before returning */
    if (content != NULL)
    {
        EdsLib_Binding_SetDescBuffer(&ConvPair.desc, NULL);
        EdsLib_Python_Buffer_ReleaseContentRef(content);
    }

    return ConvPair.pyobj;
}

/*
 * Externally callable API -- do full error checking
 */
bool EdsLib_Python_ConvertPythonToEdsObject(EdsLib_Python_ObjectBase_t *self, PyObject *pyobj)
{
    EdsLib_Python_ObjectConversionState_t ConvPair;
    EdsLib_Binding_Buffer_Content_t      *content = NULL;
    bool                                  success = false;

    memset(&ConvPair, 0, sizeof(ConvPair));

    do
    {
        /* If there is already an error context set, do nothing */
        if (PyErr_Occurred())
        {
            break;
        }

        if (self->StorageBuf == NULL)
        {
            PyErr_SetString(PyExc_BufferError, "No buffer object");
            break;
        }

        if (pyobj == NULL)
        {
            PyErr_SetString(PyExc_ValueError, "Invalid source object");
            break;
        }

        /* Obtain a buffer window reference for the conversion */
        content = EdsLib_Python_Buffer_GetContentRef(self->StorageBuf, PyBUF_WRITABLE);
        if (content == NULL)
        {
            break;
        }

        if (!EdsLib_Python_InitBindingDescriptorObject(self, &ConvPair.desc))
        {
            break;
        }

        EdsLib_Binding_SetDescBuffer(&ConvPair.desc, content);
        ConvPair.pyobj = pyobj;

        /*
         * The implementation function below will loop through the internals
         * and initialize as necessary.  It does not return a value directly.
         * If unsuccessful, it will set an exception before returning.
         */
        EdsLib_Python_ConvertPythonToEdsObjectImpl(&ConvPair);
        success = (PyErr_Occurred() == NULL);
    } while (0);

    /* be sure to release window reference before returning */
    if (content != NULL)
    {
        EdsLib_Binding_SetDescBuffer(&ConvPair.desc, NULL);
        EdsLib_Python_Buffer_ReleaseContentRef(content);
    }

    return success;
}

int EdsLib_Python_SetupObjectDesciptor(EdsLib_Python_ObjectBase_t        *self,
                                       EdsLib_Binding_DescriptorObject_t *descobj,
                                       int                                flags)
{
    EdsLib_Binding_Buffer_Content_t *content;

    if (self->StorageBuf == NULL)
    {
        PyErr_SetString(PyExc_BufferError, "No buffer object");
        return 0;
    }

    content = EdsLib_Python_Buffer_GetContentRef(self->StorageBuf, flags);
    if (content == NULL)
    {
        return 0;
    }

    if (!EdsLib_Python_InitBindingDescriptorObject(self, descobj))
    {
        return 0;
    }

    EdsLib_Binding_SetDescBuffer(descobj, content);

    return 1;
}

void EdsLib_Python_ReleaseObjectDesciptor(EdsLib_Binding_DescriptorObject_t *descobj)
{
    EdsLib_Binding_Buffer_Content_t *content = descobj->BufferPtr;
    EdsLib_Binding_SetDescBuffer(descobj, NULL);
    if (content != NULL)
    {
        EdsLib_Python_Buffer_ReleaseContentRef(content);
    }
}

EdsLib_Id_t EdsLib_Python_ConvertArgToEdsId(const EdsLib_DatabaseObject_t *GD, PyObject *arg)
{
    EdsLib_Id_t Result;
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
            Result = EdsLib_DisplayDB_LookupTypeName(GD, PyBytes_AsString(temp_id));
            if (!EdsLib_Is_Valid(Result))
            {
                PyErr_Format(PyExc_ValueError, "Type not found in EDS DB: %s", PyBytes_AsString(temp_id));
            }
        }
    }

    Py_XDECREF(temp_id);

    return Result;
}
