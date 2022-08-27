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
 * \file     cfe_missionlib_lua_softwarebus.c
 * \ingroup  lua
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of the CFE-EDS mission integration library Lua bindings
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

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
#include "cfe_missionlib_lua_softwarebus.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

static const char CFE_MISSIONLIB_INTFDB_KEY;

static void CFE_MissionLib_Lua_MapPubSubParams(CFE_SB_SoftwareBus_PubSub_Interface_t *PubSub, const CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj)
{
    switch (IntfObj->IntfId)
    {
    case CFE_SB_Telecommand_Interface_ID:
    {
        CFE_SB_Listener_Component_t Params;
        Params.Telecommand.InstanceNumber = IntfObj->InstanceNumber;
        Params.Telecommand.TopicId = IntfObj->TopicId;
        CFE_MissionLib_MapListenerComponent(PubSub, &Params);
        break;
    }
    case CFE_SB_Telemetry_Interface_ID:
    {
        CFE_SB_Publisher_Component_t Params;
        Params.Telemetry.InstanceNumber = IntfObj->InstanceNumber;
        Params.Telemetry.TopicId = IntfObj->TopicId;
        CFE_MissionLib_MapPublisherComponent(PubSub, &Params);
        break;
    }
    default:
    {
        memset(PubSub, 0, sizeof(*PubSub));
        break;
    }
    }
}

static void CFE_MissionLib_Lua_UnmapPubSubParams(CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj, const CFE_SB_SoftwareBus_PubSub_Interface_t *PubSub)
{
    switch(IntfObj->IntfId)
    {
    case CFE_SB_Telecommand_Interface_ID:
    {
        CFE_SB_Listener_Component_t Result;
        CFE_MissionLib_UnmapListenerComponent(&Result, PubSub);
        IntfObj->TopicId = Result.Telecommand.TopicId;
        IntfObj->InstanceNumber = Result.Telecommand.InstanceNumber;
        break;
    }
    case CFE_SB_Telemetry_Interface_ID:
    {
        CFE_SB_Publisher_Component_t Result;
        CFE_MissionLib_UnmapPublisherComponent(&Result, PubSub);
        IntfObj->TopicId = Result.Telemetry.TopicId;
        IntfObj->InstanceNumber = Result.Telemetry.InstanceNumber;
        break;
    }
    default:
    {
        IntfObj->TopicId = 0;
        IntfObj->InstanceNumber = 0;
        break;
    }
    }
}


CFE_MissionLib_Lua_Interface_Userdata_t *CFE_MissionLib_Lua_NewInterfaceObject(lua_State *lua, int dbobj_idx)
{
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj;

    IntfObj = lua_newuserdata(lua, sizeof(*IntfObj));
    memset(IntfObj,0,sizeof(*IntfObj));

    /*
     * Set metatable for the new object
     */
    luaL_getmetatable(lua, "CFE_MissionLib_Lua_Interface");
    lua_setmetatable(lua, -2);

    lua_getuservalue(lua, dbobj_idx);
    lua_rawgetp(lua, -1, &CFE_MISSIONLIB_INTFDB_KEY);
    IntfObj->IntfDB = lua_touserdata(lua, -1);
    lua_pop(lua, 2);

    return IntfObj;
}

static int CFE_MissionLib_Lua_GetInterface(lua_State *lua)
{
    const char *DestName = luaL_checkstring(lua, 1);
    const char *IndicationName = luaL_optstring(lua, 3, "indication");
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj;
    CFE_MissionLib_InterfaceInfo_t IntfInfo;
    int32_t Status;

    lua_settop(lua, 3);

    IntfObj = CFE_MissionLib_Lua_NewInterfaceObject(lua, lua_upvalueindex(1));

    if (lua_type(lua, 2) == LUA_TNUMBER)
    {
        IntfObj->InstanceNumber = lua_tointeger(lua, 2);
    }
    else if (lua_type(lua, 2) == LUA_TSTRING)
    {
        IntfObj->InstanceNumber = CFE_MissionLib_GetInstanceNumber(IntfObj->IntfDB, lua_tostring(lua, 2));
    }
    else
    {
        IntfObj->InstanceNumber = 1;
    }

    while (1)
    {
        ++IntfObj->IntfId;
        Status = CFE_MissionLib_GetInterfaceInfo(IntfObj->IntfDB, IntfObj->IntfId, &IntfInfo);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            break;
        }

        Status = CFE_MissionLib_FindCommandByName(IntfObj->IntfDB, IntfObj->IntfId, IndicationName, &IntfObj->IndicationId);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            continue;
        }

        Status = CFE_MissionLib_FindTopicByName(IntfObj->IntfDB, IntfObj->IntfId, DestName, &IntfObj->TopicId);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            continue;
        }

        /*
         * Currently all software bus interfaces have only one command with one argument,
         * and the argument is the actual message sent on the bus.
         *
         * Obtaining the EdsId for argument=1 should always be the correct type.
         *
         * If this fails it suggests a bug in the database object, as the topic ID was
         * just obtained from the same DB it should be valid.
         */
        Status = CFE_MissionLib_GetArgumentType(IntfObj->IntfDB, IntfObj->IntfId, IntfObj->TopicId,
                IntfObj->IndicationId, 1, &IntfObj->IndicationBaseArg);
        if (Status == CFE_MISSIONLIB_SUCCESS)
        {
            break;
        }
    }

    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return 0;
    }

    return 1;

}

static int CFE_MissionLib_Lua_InterfaceObjectGetProperty(lua_State *lua)
{
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj =
            luaL_checkudata(lua, 1, "CFE_MissionLib_Lua_Interface");
    const char *Str;
    int retval = 0;

    if (lua_type(lua, 2) == LUA_TSTRING)
    {
        const char *PropName = lua_tostring(lua, 2);

        if (strcmp(PropName, "IntfId") == 0)
        {
            lua_pushinteger(lua, IntfObj->IntfId);
            retval = 1;
        }
        else if (strcmp(PropName, "TopicId") == 0)
        {
            lua_pushinteger(lua, IntfObj->TopicId);
            retval = 1;
        }
        else if (strcmp(PropName, "IndicationId") == 0)
        {
            lua_pushinteger(lua, IntfObj->IndicationId);
            retval = 1;
        }
        else if (strcmp(PropName, "InstanceNumber") == 0)
        {
            lua_pushinteger(lua, IntfObj->InstanceNumber);
            retval = 1;
        }
        else if (strcmp(PropName, "IntfName") == 0)
        {
            Str = CFE_MissionLib_GetInterfaceName(IntfObj->IntfDB, IntfObj->IntfId);
            if (Str != NULL && Str[0] != 0)
            {
                lua_pushstring(lua, Str);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "TopicName") == 0)
        {
            Str = CFE_MissionLib_GetTopicName(IntfObj->IntfDB, IntfObj->IntfId, IntfObj->TopicId);
            if (Str != NULL && Str[0] != 0)
            {
                lua_pushstring(lua, Str);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "IndicationName") == 0)
        {
            Str = CFE_MissionLib_GetCommandName(IntfObj->IntfDB, IntfObj->IntfId, IntfObj->IndicationId);
            if (Str != NULL && Str[0] != 0)
            {
                lua_pushstring(lua, Str);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "InstanceName") == 0)
        {
            char StringBuffer[256];
            Str = CFE_MissionLib_GetInstanceName(IntfObj->IntfDB, IntfObj->InstanceNumber, StringBuffer, sizeof(StringBuffer));
            if (Str != NULL && Str[0] != 0)
            {
                lua_pushstring(lua, Str);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "MsgId") == 0)
        {
            CFE_SB_SoftwareBus_PubSub_Interface_t PubSub;
            CFE_MissionLib_Lua_MapPubSubParams(&PubSub, IntfObj);
            lua_pushinteger(lua, PubSub.MsgId.Value);
            retval = 1;
        }
    }

    return retval;
}

static int CFE_MissionLib_Lua_InterfaceObjectEqual(lua_State *lua)
{
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj1 =
            luaL_testudata(lua, 1, "CFE_MissionLib_Lua_Interface");
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj2 =
            luaL_testudata(lua, 2, "CFE_MissionLib_Lua_Interface");

    if (IntfObj1 == NULL || IntfObj2 == NULL)
    {
        lua_pushboolean(lua, 0);
    }
    else if (IntfObj1->IntfDB != IntfObj2->IntfDB ||
            IntfObj1->IntfId != IntfObj2->IntfId ||
            IntfObj1->TopicId != IntfObj2->TopicId ||
            IntfObj1->InstanceNumber != IntfObj2->InstanceNumber ||
            IntfObj1->IndicationId != IntfObj2->IndicationId)
    {
        lua_pushboolean(lua, 0);
    }
    else
    {
        lua_pushboolean(lua, 1);
    }

    return 1;
}

static int CFE_MissionLib_Lua_InterfaceObjectToString(lua_State *lua)
{
    char StringBuffer[128];
    const char *Str;
    int top_start = lua_gettop(lua);
    int top_end;

    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj =
            luaL_checkudata(lua, 1, "CFE_MissionLib_Lua_Interface");

    Str = CFE_MissionLib_GetInstanceName(IntfObj->IntfDB, IntfObj->InstanceNumber, StringBuffer, sizeof(StringBuffer));
    if (Str != NULL && Str[0] != 0)
    {
        lua_pushstring(lua, Str);
        lua_pushstring(lua, ":");
    }

    Str = CFE_MissionLib_GetInterfaceName(IntfObj->IntfDB, IntfObj->IntfId);
    if (Str != NULL && Str[0] != 0)
    {
        lua_pushstring(lua, Str);
        lua_pushstring(lua, ":");
    }

    Str = CFE_MissionLib_GetTopicName(IntfObj->IntfDB, IntfObj->IntfId, IntfObj->TopicId);
    if (Str != NULL && Str[0] != 0)
    {
        lua_pushstring(lua, Str);
        lua_pushstring(lua, ":");
    }

    Str = CFE_MissionLib_GetCommandName(IntfObj->IntfDB, IntfObj->IntfId, IntfObj->IndicationId);
    if (Str != NULL && Str[0] != 0)
    {
        lua_pushstring(lua, Str);
        lua_pushstring(lua, ":");
    }

    top_end = lua_gettop(lua);
    if (top_end <= top_start)
    {
        /* Nothing was identified, return nil */
        return 0;
    }

    /* Remove the trailing semicolon, concatenate everything else */
    lua_pop(lua, 1);
    --top_end;

    if ((top_end - top_start) > 1)
    {
        lua_concat(lua, top_end - top_start);
    }

    return 1;
}


static int CFE_MissionLib_Lua_NewMessage(lua_State *lua)
{
    const EdsLib_Lua_Database_Userdata_t *DbObj = lua_touserdata(lua, lua_upvalueindex(1));
    const CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj = luaL_checkudata(lua, 1, "CFE_MissionLib_Lua_Interface");
    const char *CommandName = luaL_optstring(lua, 2, NULL);
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    EdsLib_LuaBinding_DescriptorObject_t *ObjectUserData;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSub;
    EdsLib_Id_t PossibleId;
    uint16_t DerivIdx;
    int32_t Status;

    Status = EdsLib_DataTypeDB_GetDerivedInfo(DbObj->GD, IntfObj->IndicationBaseArg, &DerivInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        /* This would suggest that the base argument is invalid/incorrect */
        return 0;
    }

    ObjectUserData = EdsLib_LuaBinding_CreateEmptyObject(lua, DerivInfo.MaxSize.Bytes);
    ObjectUserData->GD = DbObj->GD;
    ObjectUserData->EdsId = IntfObj->IndicationBaseArg;

    /*
     * With commands this is only half the battle:
     * It may have a command code as well, which would be a similarly-named type that is
     * derived from the interface command argument type.
     */
    if (Status == EDSLIB_SUCCESS && CommandName != NULL)
    {
        DerivIdx = 0;
        while (1)
        {
            if (DerivIdx >= DerivInfo.NumDerivatives)
            {
                break;
            }

            if (EdsLib_DataTypeDB_GetDerivedTypeById(DbObj->GD, IntfObj->IndicationBaseArg, DerivIdx, &PossibleId) == EDSLIB_SUCCESS &&
                    strcmp(EdsLib_DisplayDB_GetBaseName(DbObj->GD, PossibleId), CommandName) == 0)
            {
                ObjectUserData->EdsId = PossibleId;
                break;
            }

            ++DerivIdx;
        }
    }

    EdsLib_DataTypeDB_GetTypeInfo(DbObj->GD, ObjectUserData->EdsId, &ObjectUserData->TypeInfo);

    CFE_MissionLib_Lua_MapPubSubParams(&PubSub, IntfObj);
    CFE_MissionLib_Set_PubSub_Parameters(EdsLib_Binding_GetNativeObject(ObjectUserData), &PubSub);
    EdsLib_Binding_InitStaticFields(ObjectUserData);

    return 1;
}

static int CFE_MissionLib_Lua_IdentifyMessage(lua_State *lua)
{
    const EdsLib_Lua_Database_Userdata_t *DbObj = lua_touserdata(lua, lua_upvalueindex(1));
    const uint8_t *SourceBuffer = (const uint8_t*)luaL_checkstring(lua, 1);
    uint32_t SourceBufferSize = lua_rawlen(lua, 1);
    const char *IndicationName = luaL_optstring(lua, 2, "indication");
    EdsLib_Binding_DescriptorObject_t *ObjectUserData;
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSub;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    CFE_MissionLib_InterfaceInfo_t IntfInfo;
    EdsLib_Id_t EdsId;
    int32_t Status;

    EdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), CFE_HDR_Message_DATADICTIONARY);
    Status = EdsLib_DataTypeDB_GetDerivedInfo(DbObj->GD, EdsId, &DerivInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    ObjectUserData = EdsLib_LuaBinding_CreateEmptyObject(lua, DerivInfo.MaxSize.Bytes);
    ObjectUserData->GD = DbObj->GD;
    ObjectUserData->EdsId = EdsId;

    Status = EdsLib_DataTypeDB_UnpackPartialObject(DbObj->GD, &EdsId, EdsLib_Binding_GetNativeObject(ObjectUserData),
            SourceBuffer, DerivInfo.MaxSize.Bytes, 8 * SourceBufferSize, 0);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    ObjectUserData->EdsId = EdsId;
    EdsLib_DataTypeDB_GetTypeInfo(DbObj->GD, EdsId, &ObjectUserData->TypeInfo);

    CFE_MissionLib_Get_PubSub_Parameters(&PubSub, EdsLib_Binding_GetNativeObject(ObjectUserData));

    IntfObj = CFE_MissionLib_Lua_NewInterfaceObject(lua, lua_upvalueindex(1));

    /*
     * JPHFIX:
     * This needs some sort of lookup function to positively identify
     * whether the message is CMD or TLM.  This needs to come from EDS without
     * relying on an assumption of CCSDS primary header bits.
     *
     * For now, just try both.  A mismatch should produce a topic ID of zero
     * which is invalid.  This is less efficient of course but it will work
     * for now and it has no reliance on CCSDS framing bits.
     */
    while (1)
    {
        ++IntfObj->IntfId;
        Status = CFE_MissionLib_GetInterfaceInfo(IntfObj->IntfDB, IntfObj->IntfId, &IntfInfo);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            break;
        }

        Status = CFE_MissionLib_FindCommandByName(IntfObj->IntfDB, IntfObj->IntfId, IndicationName, &IntfObj->IndicationId);
        if (Status != CFE_MISSIONLIB_SUCCESS)
        {
            continue;
        }

        CFE_MissionLib_Lua_UnmapPubSubParams(IntfObj, &PubSub);

        /*
         * Currently all software bus interfaces have only one command with one argument,
         * and the argument is the actual message sent on the bus.
         *
         * Obtaining the EdsId for argument=1 should always be the correct type.
         *
         * If this fails it suggests a bug in the database object, as the topic ID was
         * just obtained from the same DB it should be valid.
         */
        Status = CFE_MissionLib_GetArgumentType(IntfObj->IntfDB, IntfObj->IntfId, IntfObj->TopicId,
                IntfObj->IndicationId, 1, &IntfObj->IndicationBaseArg);
        if (Status == CFE_MISSIONLIB_SUCCESS)
        {
            break;
        }
    }

    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return 1;
    }


    EdsId = IntfObj->IndicationBaseArg;
    Status = EdsLib_DataTypeDB_UnpackPartialObject(DbObj->GD, &EdsId, EdsLib_Binding_GetNativeObject(ObjectUserData),
            SourceBuffer, DerivInfo.MaxSize.Bytes, 8 * SourceBufferSize, ObjectUserData->TypeInfo.Size.Bytes);
    if (Status != EDSLIB_SUCCESS)
    {
        return 1;
    }

    Status = EdsLib_DataTypeDB_VerifyUnpackedObject(DbObj->GD, EdsId, EdsLib_Binding_GetNativeObject(ObjectUserData),
            SourceBuffer, EDSLIB_DATATYPEDB_RECOMPUTE_NONE);
    if (Status != EDSLIB_SUCCESS)
    {
        return 1;
    }

    ObjectUserData->EdsId = EdsId;
    EdsLib_DataTypeDB_GetTypeInfo(DbObj->GD, EdsId, &ObjectUserData->TypeInfo);

    return 2;
}

void CFE_MissionLib_Lua_SoftwareBus_Attach(lua_State *lua, const CFE_MissionLib_SoftwareBus_Interface_t *IntfDB)
{
    int Obj = lua_gettop(lua);
    EdsLib_Lua_Database_Userdata_t *DbObj = luaL_checkudata(lua, Obj, "EdsDb");
    if (DbObj->GD == NULL)
    {
        /* This is a bug in the caller, better to complain loudly than silently returning */
        abort();
    }

    lua_getuservalue(lua, -1);

    lua_pushlightuserdata(lua, (void*)IntfDB);
    lua_rawsetp(lua, -2, &CFE_MISSIONLIB_INTFDB_KEY);

    lua_pushvalue(lua, Obj);
    lua_pushcclosure(lua, CFE_MissionLib_Lua_GetInterface, 1);
    lua_setfield(lua, -2, "GetInterface");

    lua_pushvalue(lua, Obj);
    lua_pushcclosure(lua, CFE_MissionLib_Lua_NewMessage, 1);
    lua_setfield(lua, -2, "NewMessage");

    lua_pushvalue(lua, Obj);
    lua_pushcclosure(lua, CFE_MissionLib_Lua_IdentifyMessage, 1);
    lua_setfield(lua, -2, "IdentifyMessage");


    /*
     * Create a metatable for EDS objects (userdata blobs)
     * This also has the hook to call our routine when old objects are collected
     */
    if (luaL_newmetatable(lua, "CFE_MissionLib_Lua_Interface"))
    {
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, CFE_MissionLib_Lua_InterfaceObjectToString);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, CFE_MissionLib_Lua_InterfaceObjectGetProperty);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__eq");
        lua_pushcfunction(lua, CFE_MissionLib_Lua_InterfaceObjectEqual);
        lua_rawset(lua, -3);
    }

    /* Reset the stack top to where it was initially */
    lua_settop(lua, Obj);

}
