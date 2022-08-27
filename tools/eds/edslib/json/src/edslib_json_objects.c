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
 * \file     edslib_json_objects.c
 * \ingroup  json
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of EdsLib-JSON binding library
 */

#include <json-c/json.h>
#include <json-c/printbuf.h>
#include <stdio.h>

#include "edslib_id.h"
#include "edslib_datatypedb.h"
#include "edslib_displaydb.h"

#include "edslib_binding_objects.h"
#include "edslib_json_objects.h"

typedef struct
{
    const EdsLib_Binding_DescriptorObject_t *ParentObj;
    struct json_object *JsonObject;
} EdsLib_JsonBinding_ObjectConversion_t;

static void EdsLib_JSON_SetJSONObject_Callback(void *Arg, const EdsLib_EntityDescriptor_t *Param)
{
    EdsLib_JsonBinding_ObjectConversion_t *State = Arg;
    EdsLib_JsonBinding_DescriptorObject_t SubDesc;
    struct json_object *ThisElement;

    EdsLib_Binding_InitSubObject(&SubDesc, State->ParentObj, &Param->EntityInfo);

    ThisElement = EdsLib_JSON_EdsObjectToJSON(&SubDesc);

    if (json_object_get_type(State->JsonObject) == json_type_array)
    {
        /*
         * note that EDS supports arrays with named elements (via indextyperef)
         * but JSON does not, at least not without making it an object but
         * then one loses array semantics.
         *
         * The generated JSON array object will be a plain sequence
         */
        json_object_array_add(State->JsonObject, ThisElement);
        ThisElement = NULL;
    }
    else if (Param->FullName != NULL)
    {
        /* Object case where the sub-container has a designated name */
        json_object_object_add(State->JsonObject, Param->FullName, ThisElement);
        ThisElement = NULL;
    }

    if (ThisElement != NULL)
    {
        /*
         * Nowhere to put the object ....
         * Should never reach this, but in case we do,
         * decrease the refcount of the object we created to avoid leaking memory.
         */
        json_object_put(ThisElement);
    }
}

static int EdsLib_JSON_Double2Str(struct json_object *jso,
                        struct printbuf *pb,
                        int level,
                        int flags)
{
    char buf[32];
    int size;
    double value = json_object_get_double(jso);

    if (value == value)
    {
        size = snprintf(buf, sizeof(buf), "%.5lg", value);
    }
    else
    {
        size = snprintf(buf, sizeof(buf), "NaN");
    }

    if (size >= sizeof(buf))
    {
        size = sizeof(buf) - 1;
    }
    printbuf_memappend(pb, buf, size);
    return size;
}

EdsLib_JsonBinding_Object_t *EdsLib_JSON_EdsObjectToJSON(const EdsLib_JsonBinding_DescriptorObject_t *SrcObject)
{
    struct json_object *ThisElement;

    ThisElement = NULL;

    switch(SrcObject->TypeInfo.ElemType)
    {
    case EDSLIB_BASICTYPE_SIGNED_INT:
    case EDSLIB_BASICTYPE_UNSIGNED_INT:
    case EDSLIB_BASICTYPE_FLOAT:
    {
        EdsLib_GenericValueBuffer_t ValueBuffer;
        const char *SymName;

        EdsLib_Binding_LoadValue(SrcObject, &ValueBuffer);

        /*
         * note JSON-C does not have a separate unsigned object type,
         * so convert to a signed type.  Internally this uses a wide type (64 bits)
         * so it is generally OK but will truncate values that are greater than 2^63.
         *
         * This is a limitation of JSON really, so it is what it is.  It is unlikely
         * that such large values will need to be represented in JSON, so this will
         * probably never be noticed.
         */
        if (ValueBuffer.ValueType == EDSLIB_BASICTYPE_UNSIGNED_INT)
        {
            ValueBuffer.Value.SignedInteger = ValueBuffer.Value.UnsignedInteger;
            ValueBuffer.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
        }

        SymName = EdsLib_DisplayDB_GetEnumLabel(SrcObject->GD, SrcObject->EdsId, &ValueBuffer);
        if (SymName != NULL)
        {
            ThisElement = json_object_new_string(SymName);
        }
        else if (ValueBuffer.ValueType == EDSLIB_BASICTYPE_FLOAT)
        {
            ThisElement = json_object_new_double(ValueBuffer.Value.FloatingPoint);
            json_object_set_serializer(ThisElement, EdsLib_JSON_Double2Str, NULL, NULL);
        }
        else if (ValueBuffer.ValueType == EDSLIB_BASICTYPE_SIGNED_INT)
        {
            if (EdsLib_DisplayDB_GetDisplayHint(SrcObject->GD, SrcObject->EdsId) == EDSLIB_DISPLAYHINT_BOOLEAN)
            {
                ThisElement = json_object_new_boolean(ValueBuffer.Value.SignedInteger);
            }
            else
            {
                ThisElement = json_object_new_int64(ValueBuffer.Value.SignedInteger);
            }
        }
        break;
    }
    case EDSLIB_BASICTYPE_BINARY:
    {
        /*
         * JSON objects cannot contain binary data directly;
         * Use the EdsLib scalar conversion to export the data in a safe form.
         *
         * allocate a temporary buffer with 1 byte per 6 bits of actual binary data,
         * to cover the case where the data is base64 encoded.
         *
         * Include a few bytes of extra space in the buffer for e.g. termination.
         */
        uint32_t MaxSize = 4 + ((SrcObject->TypeInfo.Size.Bits + 5) / 6);
        char* Buffer = malloc(MaxSize);

        if (Buffer != NULL)
        {
            if (EdsLib_Scalar_ToString(SrcObject->GD, SrcObject->EdsId, Buffer, MaxSize,
                    EdsLib_Binding_GetNativeObject(SrcObject)) == EDSLIB_SUCCESS)
            {
                ThisElement = json_object_new_string(Buffer);
            }
            free(Buffer);
        }
        break;
    }
    case EDSLIB_BASICTYPE_ARRAY:
    {
        uint16_t Idx;
        EdsLib_DataTypeDB_EntityInfo_t CompInfo;
        EdsLib_JsonBinding_DescriptorObject_t SubDesc;

        ThisElement = json_object_new_array();
        for (Idx=0; Idx < SrcObject->TypeInfo.NumSubElements; ++Idx)
        {
            EdsLib_DataTypeDB_GetMemberByIndex(SrcObject->GD, SrcObject->EdsId, Idx, &CompInfo);
            EdsLib_Binding_InitSubObject(&SubDesc, SrcObject, &CompInfo);
            json_object_array_put_idx(ThisElement, Idx, EdsLib_JSON_EdsObjectToJSON(&SubDesc));
        }
        break;
    }
    case EDSLIB_BASICTYPE_CONTAINER:
    {
        EdsLib_JsonBinding_ObjectConversion_t SubObject;

        SubObject.ParentObj = SrcObject;
        SubObject.JsonObject = json_object_new_object();
        EdsLib_DisplayDB_IterateBaseEntities(SrcObject->GD, SrcObject->EdsId,
                EdsLib_JSON_SetJSONObject_Callback, &SubObject);

        ThisElement = SubObject.JsonObject;
        break;
    }
    default:
        break;
    }

    return ThisElement;
}

static void EdsLib_JSON_SetEDSObject_Callback(void *Arg, const EdsLib_EntityDescriptor_t *Param)
{
    EdsLib_JsonBinding_ObjectConversion_t *State = Arg;
    EdsLib_JsonBinding_DescriptorObject_t SubDesc;
    struct json_object *SubObj;

    if (json_object_object_get_ex(State->JsonObject, Param->FullName, &SubObj) &&
            SubObj != NULL)
    {
        EdsLib_Binding_InitSubObject(&SubDesc, State->ParentObj, &Param->EntityInfo);
        EdsLib_JSON_EdsObjectFromJSON(&SubDesc, SubObj);
    }
}

void EdsLib_JSON_EdsObjectFromJSON(EdsLib_JsonBinding_DescriptorObject_t *DstObject, EdsLib_JsonBinding_Object_t *SrcObject)
{
    enum json_type SrcType;

    /*
     * subelement can only be stored if parent is an array or object.
     */
    SrcType = json_object_get_type(SrcObject);

    if (DstObject->TypeInfo.NumSubElements == 0)
    {
        EdsLib_GenericValueBuffer_t ValueBuffer;

        /* scalar quantities */
        if (SrcType == json_type_double)
        {
            ValueBuffer.Value.FloatingPoint = json_object_get_double(SrcObject);
            ValueBuffer.ValueType = EDSLIB_BASICTYPE_FLOAT;
        }
        else if (SrcType == json_type_int)
        {
            ValueBuffer.Value.SignedInteger = json_object_get_int64(SrcObject);
            ValueBuffer.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
        }
        else if (SrcType == json_type_boolean)
        {
            ValueBuffer.Value.UnsignedInteger = json_object_get_boolean(SrcObject);
            ValueBuffer.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
        }
        else
        {
            ValueBuffer.Value.UnsignedInteger = 0;
            ValueBuffer.ValueType = EDSLIB_BASICTYPE_NONE;
        }

        if (ValueBuffer.ValueType != EDSLIB_BASICTYPE_NONE)
        {
            EdsLib_Binding_StoreValue(DstObject, &ValueBuffer);
        }
        else if (SrcType == json_type_string)
        {
            /*
             * If the JSON value is a string, then just use the EdsLib parser to import it.
             * This should coerce most strings into the correct type, whatever it is.
             */
            EdsLib_Scalar_FromString(DstObject->GD, DstObject->EdsId,
                    EdsLib_Binding_GetNativeObject(DstObject), json_object_get_string(SrcObject));
        }
    }
    else if (DstObject->TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY &&
            SrcType == json_type_array)
    {
        uint16_t Idx;
        EdsLib_DataTypeDB_EntityInfo_t CompInfo;
        EdsLib_JsonBinding_DescriptorObject_t SubDesc;

        for (Idx=0; Idx < DstObject->TypeInfo.NumSubElements; ++Idx)
        {
            if (EdsLib_DataTypeDB_GetMemberByIndex(DstObject->GD, DstObject->EdsId, Idx, &CompInfo) == EDSLIB_SUCCESS)
            {
                EdsLib_Binding_InitSubObject(&SubDesc, DstObject, &CompInfo);
                EdsLib_JSON_EdsObjectFromJSON(&SubDesc, json_object_array_get_idx(SrcObject, Idx));
            }
        }
    }
    else if (SrcType == json_type_object)
    {
        EdsLib_JsonBinding_ObjectConversion_t SubObject;

        SubObject.ParentObj = DstObject;
        SubObject.JsonObject = SrcObject;

        EdsLib_DisplayDB_IterateBaseEntities(DstObject->GD, DstObject->EdsId,
                EdsLib_JSON_SetEDSObject_Callback, &SubObject);
    }
}

