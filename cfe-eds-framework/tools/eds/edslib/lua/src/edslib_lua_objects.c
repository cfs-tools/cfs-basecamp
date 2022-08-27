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
 * \file     edslib_lua_objects.c
 * \ingroup  lua
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation for EdsLib-Lua binding library
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* compatibility shim to support compilation with Lua5.1 */
#include "edslib_lua51_compatibility.h"

#include "edslib_id.h"
#include "edslib_datatypedb.h"
#include "edslib_displaydb.h"
#include "edslib_binding_objects.h"
#include "edslib_lua_objects.h"

#define EDSLIB_MAX_BUFFER_SIZE          65528

static int EdsLib_LuaBinding_DestroyObject(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *EdsLib_Object;

    EdsLib_Object = luaL_checkudata(lua, 1, "EdsLib_Object");
    if (EdsLib_Object != NULL)
    {
        /* Setting the buffer to NULL will decrement the refcount and possibly free the buffer too */
        EdsLib_Binding_SetDescBuffer(EdsLib_Object, NULL);
    }

    return 0;
}

static EdsLib_Binding_DescriptorObject_t *EdsLib_LuaBinding_NewDescriptor(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *ObjectUserData;

    ObjectUserData = lua_newuserdata(lua, sizeof(EdsLib_Binding_DescriptorObject_t));
    memset(ObjectUserData, 0, sizeof(*ObjectUserData));

    /*
     * Set metatable for the new object
     */
    luaL_getmetatable(lua, "EdsLib_Object");
    lua_setmetatable(lua, -2);

    return ObjectUserData;
}

static EdsLib_Binding_DescriptorObject_t *EdsLib_LuaBinding_NewSubObject(lua_State *lua, const EdsLib_Binding_DescriptorObject_t *ParentObj, const EdsLib_DataTypeDB_EntityInfo_t *Component)
{
    EdsLib_Binding_DescriptorObject_t *SubObject = EdsLib_LuaBinding_NewDescriptor(lua);
    EdsLib_Binding_InitSubObject(SubObject, ParentObj, Component);
    return SubObject;
}

static int EdsLib_LuaBinding_GetField(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *Object;
    EdsLib_DataTypeDB_EntityInfo_t ComponentInfo;
    int32_t Status;

    /*
     * Expected LUA stack:
     *  Object(userdata) @ 1
     *  Index/Field Name @ 2
     *  Field Value      @ 3 -- ONLY FOR SET, ignored here.
     */

    Object = luaL_checkudata(lua, 1, "EdsLib_Object");
    luaL_argcheck(lua, Object->GD != NULL, 1, "Object->GD != NULL");

    switch (Object->TypeInfo.ElemType)
    {
    case EDSLIB_BASICTYPE_CONTAINER:
    {
        /* For containers, the field should be a name */
        const char *Name = luaL_checkstring(lua, 2);
        Status = EdsLib_DisplayDB_LocateSubEntity(Object->GD,
                Object->EdsId,
                Name, &ComponentInfo);
        break;
    }
    case EDSLIB_BASICTYPE_ARRAY:
    {
        /* For arrays, the field can be an integer (normal case)
         * or an enumeration label if the array was defined using an "indexTypeRef" of an enumeration
         */
        uint16_t SubIndex;

        if (lua_type(lua, 2) == LUA_TNUMBER)
        {
            /* In Lua array indices should start at 1, not 0 */
            SubIndex = luaL_checkinteger(lua, 2) - 1;
            Status = EDSLIB_SUCCESS;
        }
        else
        {
            Status = EdsLib_DisplayDB_GetIndexByName(Object->GD, Object->EdsId, lua_tostring(lua, 2), &SubIndex);
        }

        if (Status == EDSLIB_SUCCESS)
        {
            Status = EdsLib_DataTypeDB_GetMemberByIndex(Object->GD, Object->EdsId,
                    SubIndex, &ComponentInfo);
        }
        break;
    }
    default:
    {
        Status = EDSLIB_NAME_NOT_FOUND;
        break;
    }
    }

    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    /* Push a temporary EdsLib userdata object to reflect the sub-structure */
    EdsLib_LuaBinding_NewSubObject(lua, Object, &ComponentInfo);
    return 1;

}

static void EdsLib_LuaBinding_EnumerateMembers_Callback(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc)
{
    lua_State *lua = Arg;
    int idx;
    EdsLib_Binding_DescriptorObject_t *BaseObj = luaL_checkudata(lua, -3, "EdsLib_Object");

    luaL_checktype(lua, -2, LUA_TTABLE);
    luaL_checktype(lua, -1, LUA_TTABLE);

    idx = 1 + lua_rawlen(lua, -2);

    if (ParamDesc->FullName)
    {
        /* store name (string) in first table */
        lua_pushstring(lua, ParamDesc->FullName);
    }
    else
    {
        /* If an array then use the numeric index */
        lua_pushinteger(lua, idx);
    }
    lua_rawseti(lua, -3, idx);

    EdsLib_LuaBinding_NewSubObject(lua, BaseObj, &ParamDesc->EntityInfo);

    /* store subobject userdata in second table */
    lua_rawseti(lua, -2, idx);
}

static int EdsLib_LuaBinding_EdsObjectToLuaObject(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *Object;
    EdsLib_DisplayHint_t DispHint;
    int start_top = lua_gettop(lua);

    Object = luaL_testudata(lua, start_top, "EdsLib_Object");

    /*
     * If the parameter is not an EDS object, then return the same value back to the caller.
     * This makes it transparent if e.g. the value had already been converted back to a Lua value.
     */
    if (Object == NULL)
    {
        return 1;
    }

    if (!EdsLib_Binding_IsDescBufferValid(Object))
    {
        return 0;
    }

    DispHint = EdsLib_DisplayDB_GetDisplayHint(Object->GD, Object->EdsId);

    if (Object->TypeInfo.NumSubElements == 0)
    {
        if (Object->TypeInfo.ElemType == EDSLIB_BASICTYPE_BINARY)
        {
            const char *Str = (const char*)EdsLib_Binding_GetNativeObject(Object);
            uint32_t StrSize = 0;

            if (DispHint == EDSLIB_DISPLAYHINT_STRING)
            {
                /* Strings should stop at the first null */
                StrSize = 0;
                while (StrSize < Object->TypeInfo.Size.Bytes && Str[StrSize] != 0)
                {
                    ++StrSize;
                }
            }
            else
            {
                /* Non-string binary data should include all, including embedded nulls */
                StrSize = Object->TypeInfo.Size.Bytes;
            }

            lua_pushlstring(lua, Str, StrSize);
        }
        else if (DispHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
        {
            char StringBuffer[256];

            if (EdsLib_Scalar_ToString(Object->GD, Object->EdsId,
                    StringBuffer, sizeof(StringBuffer),
                    EdsLib_Binding_GetNativeObject(Object)) == EDSLIB_SUCCESS)
            {
                lua_pushstring(lua, StringBuffer);
            }
        }
        else
        {
            EdsLib_GenericValueBuffer_t ValueBuff;

            EdsLib_Binding_LoadValue(Object, &ValueBuff);

            switch(ValueBuff.ValueType)
            {
            case EDSLIB_BASICTYPE_SIGNED_INT:
                if (DispHint == EDSLIB_DISPLAYHINT_BOOLEAN)
                {
                    /* preserve the boolean nature of the integer field */
                    lua_pushboolean(lua, ValueBuff.Value.SignedInteger);
                }
                else
                {
                    lua_pushinteger(lua, ValueBuff.Value.SignedInteger);
                }
                break;
            case EDSLIB_BASICTYPE_UNSIGNED_INT:
                if (DispHint == EDSLIB_DISPLAYHINT_BOOLEAN)
                {
                    /* preserve the boolean nature of the integer field */
                    lua_pushboolean(lua, ValueBuff.Value.UnsignedInteger);
                }
                else
                {
                    lua_pushinteger(lua, ValueBuff.Value.UnsignedInteger);
                }
                break;
            case EDSLIB_BASICTYPE_FLOAT:
                lua_pushnumber(lua, ValueBuff.Value.FloatingPoint);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        int idx;

        /* Create a temporary table with the name name/userdata pairs */
        lua_newtable(lua);  /* index 2 -- contains the names */
        lua_newtable(lua);  /* index 3 -- contains the value types (userdata) */
        EdsLib_DisplayDB_IterateBaseEntities(Object->GD, Object->EdsId,
                EdsLib_LuaBinding_EnumerateMembers_Callback, lua);

        /* Create the table that will be returned */
        lua_newtable(lua);  /* index 4 -- final result */

        /* Attempt to convert each userdata into a Lua object, recursing as necessary */
        for (idx = 1; idx <= lua_rawlen(lua, 2); ++idx)
        {
            lua_rawgeti(lua, 2, idx);   /* get name from first temp table */
            lua_pushcfunction(lua, EdsLib_LuaBinding_EdsObjectToLuaObject);
            lua_rawgeti(lua, 3, idx);   /* get userdata from second temp table */
            lua_call(lua, 1, 1);        /* recursive call to convert sub-userdata to Lua object */
            lua_rawset(lua, 4);         /* store in final table */
        }

        /* note that the two intermediate tables will be dropped; only the top is returned */
    }

    /*
     * Note - if none of the above code pushed anything onto the stack,
     * this should return the same (original) value that was passed in i.e. the userdata.
     * Generally this shouldn't happen, but if it does this means that values that cannot
     * be converted will pass-through this function unchanged which is what we want.
     */
    return 1;
}

static int EdsLib_LuaBinding_LuaObjectToEdsObject(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *Object;
    int start_top = lua_gettop(lua);

    Object = luaL_checkudata(lua, start_top - 1, "EdsLib_Object");

    if (Object->TypeInfo.NumSubElements == 0)
    {
        if (lua_type(lua, start_top) == LUA_TSTRING)
        {
            const char *SrcString = lua_tostring(lua, start_top);
            uint32_t SrcLength = lua_rawlen(lua, start_top);

            /*
             * If the target is a binary field (which includes EDS strings)
             * copy data directly.
             */
            if (Object->TypeInfo.ElemType == EDSLIB_BASICTYPE_BINARY)
            {
                /*
                 * Need to make sure the data does not exceed the designated field,
                 * which may end up truncating it, but that is a user error if that happens.
                 */
                if (SrcLength > Object->TypeInfo.Size.Bytes)
                {
                    SrcLength = Object->TypeInfo.Size.Bytes;
                }
                memcpy(EdsLib_Binding_GetNativeObject(Object), SrcString, SrcLength);
            }
            else
            {
                /*
                 * As a fallback call the EdsLib conversion routine.
                 * This will attempt to parse the string as the appropriate EDS type.
                 * (therefore slower than the simple case above)
                 */
                EdsLib_Scalar_FromString(Object->GD, Object->EdsId,
                        EdsLib_Binding_GetNativeObject(Object),
                        SrcString);
            }

        }
        else
        {
            EdsLib_GenericValueBuffer_t ValueBuff;

            if (lua_type(lua, start_top) == LUA_TNUMBER)
            {
                /* in Lua all numbers are floats */
                ValueBuff.Value.FloatingPoint = lua_tonumber(lua, start_top);
                ValueBuff.ValueType = EDSLIB_BASICTYPE_FLOAT;
            }
            else if (lua_type(lua, start_top) == LUA_TBOOLEAN)
            {
                ValueBuff.Value.SignedInteger = lua_toboolean(lua, start_top);
                ValueBuff.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
            }
            else
            {
                ValueBuff.ValueType = EDSLIB_BASICTYPE_NONE;
            }

            EdsLib_Binding_StoreValue(Object, &ValueBuff);
        }
    }
    else
    {
        if (lua_istable(lua, start_top))
        {
            lua_pushnil(lua);
            while(lua_next(lua, start_top))
            {
                lua_pushcfunction(lua, EdsLib_LuaBinding_LuaObjectToEdsObject);
                lua_pushcfunction(lua, EdsLib_LuaBinding_GetField);
                lua_pushvalue(lua, start_top - 1);
                lua_pushvalue(lua, start_top + 1);
                lua_call(lua, 2, 1);

                if (!lua_isnil(lua, -1))
                {
                    lua_pushvalue(lua, start_top + 2);
                    lua_call(lua, 2, 0);
                }

                lua_settop(lua, start_top + 1);
            }
            lua_pop(lua, 1);
        }
    }

    lua_settop(lua, start_top);
    return 0;
}

static int EdsLib_LuaBinding_EdsValueAccessor(lua_State *lua)
{
    /*
     * Argument 1 is the userdata object itself.
     * If additional arguments given, assume assignment (set),
     * else if no additional arguments then assume retrieval (get)
     */
    if (lua_gettop(lua) >= 2)
    {
        lua_settop(lua, 2);
        if (lua_type(lua, 2) == LUA_TUSERDATA)
        {
            lua_pushcfunction(lua, EdsLib_LuaBinding_EdsObjectToLuaObject);
            lua_insert(lua, 2);
            lua_call(lua, 1, 1);
        }
        return EdsLib_LuaBinding_LuaObjectToEdsObject(lua);
    }
    else
    {
        return EdsLib_LuaBinding_EdsObjectToLuaObject(lua);
    }
}

static int EdsLib_LuaBinding_EdsObjectAssignValue(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *DestObject;
    EdsLib_Binding_DescriptorObject_t *SrcObject;

    /*
     * Expected input stack:
     *  Object descriptor   @1  (EdsLib_Object userdata)
     *  Value to assign     @2  (may be another EdsLib_Object, or native Lua value)
     */

    DestObject = luaL_checkudata(lua, 1, "EdsLib_Object");
    luaL_argcheck(lua, EdsLib_Binding_IsDescBufferValid(DestObject), 1, "Destination object has no buffer");

    /*
     * If the source object itself is an EDS object and there is binary compatibility between
     * the two objects, then do a direct memory copy.  This avoids unnecessary conversions and
     * in some cases could also avoid loss of resolution.
     */
    if (lua_type(lua, 2) == LUA_TUSERDATA)
    {
        SrcObject = luaL_checkudata(lua, 2, "EdsLib_Object");
    }
    else
    {
        SrcObject = NULL;
    }

    if (SrcObject != NULL && EdsLib_Binding_IsDescBufferValid(SrcObject) &&
            EdsLib_Binding_CheckEdsObjectsCompatible(DestObject, SrcObject) !=
                    EDSLIB_BINDING_COMPATIBILITY_NONE)
    {
        /*
         * In the simple case, the objects are binary compatible, and the
         * value can be simply memcpy()'ed to the destination object
         */
        memcpy(EdsLib_Binding_GetNativeObject(DestObject),
                EdsLib_Binding_GetNativeObject(SrcObject),
                SrcObject->TypeInfo.Size.Bytes);
    }
    else
    {
        /* Assigning EDS value from a Lua value */
        lua_pushcfunction(lua, EdsLib_LuaBinding_LuaObjectToEdsObject);
        lua_insert(lua, 1);

        if (SrcObject != NULL)
        {
            /*
             * If original source is another EDS object, this means direct copy was not possible
             * Backup plan is to coerce the value into the correct type by converting to a Lua value first
             *
             * This should handle most different-but-compatible cases, i.e.:
             *  different size EDS field, such as uint16 into uint32, strings of different lengths
             *  string from integer or integer from string
             *  float from integer or integer from float
             * etc etc.
             */
            lua_pushcfunction(lua, EdsLib_LuaBinding_EdsObjectToLuaObject);
            lua_insert(lua, 3);
            lua_call(lua, 1, 1);
        }

        lua_call(lua, 2, 0);        /* Calling LuaObjectToEdsObject */
    }

    return 0;
}

static int EdsLib_LuaBinding_SetField(lua_State *lua)
{
    /*
     * Expected LUA stack:
     *  Base Object(userdata) @ 1
     *  Index/Field Name      @ 2
     *  Field Value           @ 3
     */

    lua_settop(lua, 3);  /* adjust stack in case different number of args were passed in */

    lua_pushcfunction(lua, EdsLib_LuaBinding_EdsObjectAssignValue);

    /* GetField should push a subobject onto the stack @5 referring to the actual field */
    if (EdsLib_LuaBinding_GetField(lua) != 1)
    {
        /* On a set operation a nonexistent field is an error */
        return luaL_error(lua, "Field %s does not exist", lua_tostring(lua, 2));
    }

    lua_pushvalue(lua, 3);
    lua_call(lua, 2, 0);
    return 0;
}

static void EdsLib_LuaBinding_PushBufferAsString(lua_State *lua, const char *Desc, const uint8_t *Buffer, uint32_t ContentBitSize)
{
    luaL_Buffer lbuf;
    char TempString[5];
    uint32_t i;
    uint32_t ContentByteSize = (ContentBitSize + 7) / 8;

    lua_pushfstring(lua, "%s(%d", Desc, (int)ContentBitSize);
    luaL_buffinit(lua, &lbuf);

    if (ContentByteSize > 0)
    {
        luaL_addstring(&lbuf, ":");
        for(i = 0; i < ContentByteSize; ++i)
        {
            snprintf(TempString, sizeof(TempString), " %02x", Buffer[i]);
            luaL_addstring(&lbuf, TempString);
        }
    }

    luaL_pushresult(&lbuf);
    lua_pushstring(lua, ")");
    lua_concat(lua, 3);

}

static int EdsLib_LuaBinding_BufferToString(lua_State *lua)
{
    int nret = 0;

    if (lua_type(lua, 1) == LUA_TSTRING)
    {
        const uint8_t *Buffer = (const uint8_t*)lua_tostring(lua, 1);
        EdsLib_LuaBinding_PushBufferAsString(lua, "BITSTREAM",
                Buffer, 8 * lua_rawlen(lua, 1));
        nret = 1;
    }
    else if (lua_type(lua, 1) == LUA_TUSERDATA)
    {
        const EdsLib_Binding_DescriptorObject_t *Object = luaL_checkudata(lua, 1, "EdsLib_Object");
        if (EdsLib_Binding_IsDescBufferValid(Object))
        {
            char TypeName[128];
            EdsLib_LuaBinding_PushBufferAsString(lua,
                    EdsLib_DisplayDB_GetTypeName(Object->GD, Object->EdsId, TypeName, sizeof(TypeName)),
                    EdsLib_Binding_GetNativeObject(Object),
                    8 * Object->TypeInfo.Size.Bytes);
            nret = 1;
        }
    }

    return nret;
}

static int EdsLib_LuaBinding_ObjectToString(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *ObjectUserData;
    char StringBuffer[256];

    ObjectUserData = luaL_checkudata(lua, 1, "EdsLib_Object");

    /*
     * If the entity is actually a scalar, then use the EdsLib_Scalar_ToString function
     * as this will return a more correct representation of the field.
     */
    if (!EdsLib_Binding_IsDescBufferValid(ObjectUserData))
    {
        /* Object is a descriptor only; return a string containing the EDS type descriptor */
        lua_pushstring(lua, EdsLib_DisplayDB_GetTypeName(ObjectUserData->GD, ObjectUserData->EdsId, StringBuffer, sizeof(StringBuffer)));
    }
    else if (ObjectUserData->TypeInfo.NumSubElements == 0 && ObjectUserData->TypeInfo.Size.Bytes > 0 &&
            EdsLib_Scalar_ToString(ObjectUserData->GD, ObjectUserData->EdsId, StringBuffer, sizeof(StringBuffer),
                    EdsLib_Binding_GetNativeObject(ObjectUserData)) == EDSLIB_SUCCESS)
    {
        lua_pushstring(lua, StringBuffer);
    }
    else
    {
        EdsLib_LuaBinding_PushBufferAsString(lua,
                EdsLib_DisplayDB_GetTypeName(ObjectUserData->GD, ObjectUserData->EdsId, StringBuffer, sizeof(StringBuffer)),
                EdsLib_Binding_GetNativeObject(ObjectUserData),
                8 * ObjectUserData->TypeInfo.Size.Bytes);
    }

    return 1;
}

static int EdsLib_LuaBinding_NewObject(lua_State *lua)
{
    EdsLib_Lua_Database_Userdata_t *DbObj = luaL_checkudata(lua, lua_upvalueindex(1), "EdsDb");
    EdsLib_Binding_DescriptorObject_t *ObjectUserData;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    EdsLib_Binding_DescriptorObject_t *SrcObject;

    /*
     * The second argument to this function is an optional initializer.
     * In case the caller did not supply it, this will push a nil into that stack position.
     *
     * Must do this before creating the new object, to tell the difference between
     * the newly created object and a passed-in object used as an initializer.
     *
     * The newly created object will always be at stack position 3.
     */
    lua_settop(lua, 2);
    SrcObject = NULL;

    ObjectUserData = EdsLib_LuaBinding_NewDescriptor(lua);

    if (lua_type(lua, 1) == LUA_TSTRING)
    {
        EdsLib_Binding_InitDescriptor(ObjectUserData, DbObj->GD,
                EdsLib_DisplayDB_LookupTypeName(DbObj->GD, luaL_checkstring(lua, 1)));
    }
    else if (lua_type(lua, 1) == LUA_TUSERDATA)
    {
        SrcObject = luaL_checkudata(lua, 1, "EdsLib_Object");
        EdsLib_Binding_InitDescriptor(ObjectUserData, SrcObject->GD, SrcObject->EdsId);
        if (lua_isnil(lua, 2))
        {
            /*
             * Also use this object as an initializer for the new object.
             * This implements the "copy constructor" paradigm.
             */
            lua_pushvalue(lua, 1);
            lua_replace(lua, 2);
        }
    }

    if (EdsLib_DataTypeDB_GetDerivedInfo(ObjectUserData->GD, ObjectUserData->EdsId, &DerivInfo) != EDSLIB_SUCCESS)
    {
        /* EDS type parameter (1st argument) is not valid.
         * Could make case for silently returning nil here, but the likelihood
         * is high that this is actually a code bug in the caller. */
        lua_pushstring(lua, "Invalid type identifier, cannot instantiate: ");
        lua_pushvalue(lua, 1);
        lua_concat(lua, 2);
        return luaL_argerror(lua, 1, lua_tostring(lua, -1));
    }

    EdsLib_Binding_SetDescBuffer(ObjectUserData, EdsLib_Binding_AllocManagedBuffer(DerivInfo.MaxSize.Bytes));

    if (!EdsLib_Binding_IsDescBufferValid(ObjectUserData))
    {
        return luaL_error(lua, "Memory allocation error obtaining buffer for %d bytes",
                (int)DerivInfo.MaxSize.Bytes);
    }


    if (ObjectUserData->TypeInfo.NumSubElements > 0 &&
            lua_type(lua, 2) == LUA_TSTRING)
    {
        /*
         * When initializing a compound object (array or container) from a string,
         * then assume it is a packed binary (serialized) version of that object.
         *
         * Initialize the new object from the serialized object
         */
        EdsLib_DataTypeDB_UnpackCompleteObject(DbObj->GD, &ObjectUserData->EdsId,
                EdsLib_Binding_GetNativeObject(ObjectUserData),
                lua_tostring(lua, 2), DerivInfo.MaxSize.Bytes, 8 * lua_rawlen(lua, 2));

        /*
         * The unpack operation may modify the EdsId, so look up details after completion.
         */
        EdsLib_DataTypeDB_GetTypeInfo(DbObj->GD, ObjectUserData->EdsId,
                &ObjectUserData->TypeInfo);
    }
    else
    {
        EdsLib_DataTypeDB_GetTypeInfo(DbObj->GD, ObjectUserData->EdsId,
                &ObjectUserData->TypeInfo);

        /*
         * Initialize the new object from scratch, optionally using a Lua value to prepare internal fields
         */
        EdsLib_Binding_InitStaticFields(ObjectUserData);

        if (!lua_isnil(lua, 2))
        {
            lua_pushcfunction(lua, EdsLib_LuaBinding_EdsObjectAssignValue);
            lua_pushvalue(lua, 3);  /* new object */
            lua_pushvalue(lua, 2);  /* initializer object */
            lua_call(lua, 2, 0);
        }
    }


    return 1;
}

static int EdsLib_LuaBinding_EncodeObject(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *ObjectUserData;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    EdsLib_DataTypeDB_TypeInfo_t PackedInfo;
    EdsLib_Id_t PackMsg;
    uint32_t MaxByteSize;
    luaL_Buffer PackBuf;

    ObjectUserData = luaL_checkudata(lua, 1, "EdsLib_Object");

    if (EdsLib_DataTypeDB_GetDerivedInfo(ObjectUserData->GD, ObjectUserData->EdsId,
            &DerivInfo) == EDSLIB_SUCCESS)
    {
        /* allocate enough buffer storage for the largest derivative type */
        MaxByteSize = DerivInfo.MaxSize.Bytes;
    }
    else
    {
        /* object has no derivative types, so allocate based on object size alone */
        MaxByteSize = ObjectUserData->TypeInfo.Size.Bytes;
    }

    PackMsg = ObjectUserData->EdsId;

    luaL_buffinit(lua, &PackBuf);
    EdsLib_DataTypeDB_PackCompleteObject(ObjectUserData->GD,
            &PackMsg,
            luaL_prepbuffer(&PackBuf),
            EdsLib_Binding_GetNativeObject(ObjectUserData),
            8 * LUAL_BUFFERSIZE,
            MaxByteSize);

    EdsLib_DataTypeDB_GetTypeInfo(ObjectUserData->GD, PackMsg, &PackedInfo);
    luaL_addsize(&PackBuf, (PackedInfo.Size.Bits + 7) / 8);
    luaL_pushresult(&PackBuf);
    return 1;
}

static int EdsLib_LuaBinding_GetMetaData(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *ObjectUserData =
            luaL_checkudata(lua, 1, "EdsLib_Object");
    char StringBuffer[128];

    lua_newtable(lua);
    lua_pushinteger(lua, ObjectUserData->TypeInfo.Size.Bits);
    lua_setfield(lua, -2, "BitSize");
    lua_pushinteger(lua, ObjectUserData->TypeInfo.Size.Bytes);
    lua_setfield(lua, -2, "ByteSize");
    lua_pushinteger(lua, ObjectUserData->Length);
    lua_setfield(lua, -2, "MaxByteSize");
    lua_pushinteger(lua, ObjectUserData->Offset);
    lua_setfield(lua, -2, "BytePosition");
    lua_pushinteger(lua, ObjectUserData->EdsId);
    lua_setfield(lua, -2, "TypeId");
    lua_pushstring(lua, EdsLib_DisplayDB_GetTypeName(ObjectUserData->GD, ObjectUserData->EdsId, StringBuffer, sizeof(StringBuffer)));
    lua_setfield(lua, -2, "TypeName");

    switch (ObjectUserData->TypeInfo.ElemType)
    {
    case EDSLIB_BASICTYPE_SIGNED_INT:
        lua_pushstring(lua, "SIGNED_INT");
        break;
    case EDSLIB_BASICTYPE_UNSIGNED_INT:
        lua_pushstring(lua, "UNSIGNED_INT");
        break;
    case EDSLIB_BASICTYPE_FLOAT:
        lua_pushstring(lua, "IEEE_FLOAT");
        break;
    case EDSLIB_BASICTYPE_BINARY:
        lua_pushstring(lua, "BINARY");
        break;
    case EDSLIB_BASICTYPE_CONTAINER:
        lua_pushstring(lua, "CONTAINER");
        break;
    case EDSLIB_BASICTYPE_ARRAY:
        lua_pushstring(lua, "ARRAY");
        break;
    default:
        lua_pushnil(lua);
        break;
    }
    lua_setfield(lua, -2, "TypeClass");

    return 1;
}

static int EdsLib_LuaBinding_MemberIterator_Impl(lua_State *lua)
{
    lua_Integer idx = 1 + lua_tointeger(lua, lua_upvalueindex(3));
    lua_pushinteger(lua, idx);
    lua_replace(lua, lua_upvalueindex(3));
    lua_rawgeti(lua, lua_upvalueindex(1), idx); /* Stack index 1 */
    lua_rawgeti(lua, lua_upvalueindex(2), idx); /* Stack index 2 */
    return 2;
}

static int EdsLib_LuaBinding_GetMemberIterator(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *Object = luaL_checkudata(lua, 1, "EdsLib_Object");

    lua_newtable(lua);      /* table to hold names */
    lua_newtable(lua);      /* table to hold values (userdata) */

    /* fill in the two temporary tables */
    EdsLib_DisplayDB_IterateBaseEntities(Object->GD, Object->EdsId,
            EdsLib_LuaBinding_EnumerateMembers_Callback, lua);

    lua_pushinteger(lua, 0); /* "next index" into the table */
    lua_pushcclosure(lua, EdsLib_LuaBinding_MemberIterator_Impl, 3);
    return 1;
}

static void EdsLib_LuaBinding_CountMembers_Callback(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc)
{
    lua_Integer *ResultLen = Arg;
    ++(*ResultLen);
}

static int EdsLib_LuaBinding_GetObjectLength(lua_State *lua)
{
    EdsLib_Binding_DescriptorObject_t *Object = luaL_checkudata(lua, 1, "EdsLib_Object");
    lua_Integer ResultLen = 0;

    if (Object->TypeInfo.ElemType == EDSLIB_BASICTYPE_ARRAY)
    {
        ResultLen = Object->TypeInfo.NumSubElements;
    }
    else if (Object->TypeInfo.ElemType == EDSLIB_BASICTYPE_CONTAINER)
    {
        /*
         * Note for containers this is not always the same as NumSubElements due to
         * the possible presence of unnamed base types.  The number returned here
         * reflect the same number that one would get from an iterator.
         */
        EdsLib_DisplayDB_IterateBaseEntities(Object->GD, Object->EdsId,
                EdsLib_LuaBinding_CountMembers_Callback, &ResultLen);
    }

    lua_pushinteger(lua, ResultLen);
    return 1;
}

static int EdsLib_LuaBinding_TypeEqualCheck(lua_State *lua)
{
    /*
     * in general this should usually be called with two arguments
     */
    EdsLib_Binding_DescriptorObject_t *Obj1 = luaL_testudata(lua, 1, "EdsLib_Object");
    EdsLib_Binding_DescriptorObject_t *Obj2 = luaL_testudata(lua, 2, "EdsLib_Object");

    /*
     * This compares the _descriptors_, not the values themselves.
     * descriptors are comparable if they are of the same basic type
     */
    if (Obj1 == NULL || Obj2 == NULL)
    {
        lua_pushboolean(lua, 0);
    }
    else
    {
        lua_pushboolean(lua, EdsLib_Binding_CheckEdsObjectsCompatible(Obj1, Obj2) ==
                EDSLIB_BINDING_COMPATIBILITY_EXACT);
    }

    return 1;
}

static int EdsLib_LuaBinding_API(lua_State *lua)
{
    luaL_checkudata(lua, 1, "EdsDb");
    lua_getuservalue(lua, 1);
    lua_pushvalue(lua, 2);
    lua_rawget(lua, -2);
    return 1;
}



/* *********************************************************
 * PUBLIC API FUNCTIONS BELOW HERE
 * *********************************************************/

void EdsLib_LuaBinding_GetNativeObject(EdsLib_LuaBinding_State_t *lua, int narg, void **OutPtr, size_t *SizeBuf)
{
    EdsLib_Binding_DescriptorObject_t *ObjectUserData = luaL_testudata(lua, narg, "EdsLib_Object");
    if (ObjectUserData != NULL)
    {
        if (OutPtr != NULL)
        {
            *OutPtr = EdsLib_Binding_GetNativeObject(ObjectUserData);
        }
        if (SizeBuf != NULL)
        {
            *SizeBuf = ObjectUserData->TypeInfo.Size.Bytes;
        }
    }
    else
    {
        if (OutPtr != NULL)
        {
            *OutPtr = NULL;
        }
        if (SizeBuf != NULL)
        {
            *SizeBuf = 0;
        }
    }
}

EdsLib_LuaBinding_DescriptorObject_t *EdsLib_LuaBinding_CreateEmptyObject(EdsLib_LuaBinding_State_t *lua, size_t MaxSize)
{
    EdsLib_Binding_DescriptorObject_t *ObjectUserData;

    ObjectUserData = EdsLib_LuaBinding_NewDescriptor(lua);
    EdsLib_Binding_SetDescBuffer(ObjectUserData, EdsLib_Binding_AllocManagedBuffer(MaxSize));
    ObjectUserData->Length = MaxSize;

    return ObjectUserData;
}

void EdsLib_Lua_Attach(EdsLib_LuaBinding_State_t *lua, const EdsLib_LuaBinding_DatabaseObject_t *MissionObj)
{
    int Obj;
    if (MissionObj == NULL)
    {
        /* This is a bug in the caller, better to complain loudly than silently returning */
        abort();
    }

    EdsLib_Lua_Database_Userdata_t *DbObj = lua_newuserdata(lua, sizeof(EdsLib_Lua_Database_Userdata_t));
    DbObj->GD = MissionObj;
    Obj = lua_gettop(lua);

    lua_newtable(lua);

    lua_pushstring(lua, "NewObject");
    lua_pushvalue(lua, Obj);
    lua_pushcclosure(lua, EdsLib_LuaBinding_NewObject, 1);
    lua_rawset(lua, -3);

    lua_pushstring(lua, "GetMetaData");
    lua_pushcfunction(lua, EdsLib_LuaBinding_GetMetaData);
    lua_rawset(lua, -3);

    lua_pushstring(lua, "Encode");
    lua_pushcfunction(lua, EdsLib_LuaBinding_EncodeObject);
    lua_rawset(lua, -3);

    lua_pushstring(lua, "IterateFields");
    lua_pushcfunction(lua, EdsLib_LuaBinding_GetMemberIterator);
    lua_rawset(lua, -3);

    lua_pushstring(lua, "ToHexString");
    lua_pushcfunction(lua, EdsLib_LuaBinding_BufferToString);
    lua_rawset(lua, -3);

    lua_setuservalue(lua, Obj);

    if (luaL_newmetatable(lua, "EdsDb"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, EdsLib_LuaBinding_API);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, Obj);

    /*
     * Create a metatable for EDS objects (userdata blobs)
     * This also has the hook to call our routine when old objects are collected
     */
    if (luaL_newmetatable(lua, "EdsLib_Object"))
    {
        lua_pushstring(lua, "__gc");
        lua_pushcfunction(lua, EdsLib_LuaBinding_DestroyObject);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, EdsLib_LuaBinding_GetField);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, EdsLib_LuaBinding_SetField);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, EdsLib_LuaBinding_ObjectToString);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__call");
        lua_pushcfunction(lua, EdsLib_LuaBinding_EdsValueAccessor);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__len");
        lua_pushcfunction(lua, EdsLib_LuaBinding_GetObjectLength);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__eq");
        lua_pushcfunction(lua, EdsLib_LuaBinding_TypeEqualCheck);
        lua_rawset(lua, -3);
    }
    lua_pop(lua, 1);
}
