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

#include "edslib_id.h"
#include "edslib_global.h"
#include "edslib_datatypedb.h"
#include "edslib_displaydb.h"
#include "edslib_intfdb.h"
#include "edslib_binding_objects.h"
#include "edslib_lua_objects.h"
#include "cfe_missionlib_lua_softwarebus.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

static const char CFE_MISSIONLIB_INTFDB_KEY;

static const EdsLib_Id_t CFE_SB_TELECOMMAND_INTF_ID =
    EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsInterface_CFE_SB_Telecommand_DECLARATION);
static const EdsLib_Id_t CFE_SB_TELEMETRY_INTF_ID =
    EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsInterface_CFE_SB_Telemetry_DECLARATION);

void CFE_MissionLib_Lua_MapPubSubParams(EdsInterface_CFE_SB_SoftwareBus_PubSub_t      *PubSub,
                                        const CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj)
{
    if (IntfObj->IsTelecommand)
    {
        EdsComponent_CFE_SB_Listener_t Params;
        Params.Telecommand.InstanceNumber = IntfObj->InstanceNumber;
        Params.Telecommand.TopicId        = IntfObj->TopicId;
        CFE_MissionLib_MapListenerComponent(PubSub, &Params);
    }
    else if (IntfObj->IsTelemetry)
    {
        EdsComponent_CFE_SB_Publisher_t Params;
        Params.Telemetry.InstanceNumber = IntfObj->InstanceNumber;
        Params.Telemetry.TopicId        = IntfObj->TopicId;
        CFE_MissionLib_MapPublisherComponent(PubSub, &Params);
    }
    else
    {
        memset(PubSub, 0, sizeof(*PubSub));
    }
}

void CFE_MissionLib_Lua_UnmapPubSubParams(CFE_MissionLib_Lua_Interface_Userdata_t        *IntfObj,
                                          const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *PubSub)
{
    if (IntfObj->IsTelecommand)
    {
        EdsComponent_CFE_SB_Listener_t Result;
        CFE_MissionLib_UnmapListenerComponent(&Result, PubSub);
        IntfObj->TopicId        = Result.Telecommand.TopicId;
        IntfObj->InstanceNumber = Result.Telecommand.InstanceNumber;
    }
    else if (IntfObj->IsTelemetry)
    {
        EdsComponent_CFE_SB_Publisher_t Result;
        CFE_MissionLib_UnmapPublisherComponent(&Result, PubSub);
        IntfObj->TopicId        = Result.Telemetry.TopicId;
        IntfObj->InstanceNumber = Result.Telemetry.InstanceNumber;
    }
    else
    {
        IntfObj->TopicId        = 0;
        IntfObj->InstanceNumber = 0;
    }
}

CFE_MissionLib_Lua_Interface_Userdata_t *CFE_MissionLib_Lua_NewInterfaceObject(lua_State *lua, int dbobj_idx)
{
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj;

    IntfObj = lua_newuserdata(lua, sizeof(*IntfObj));
    memset(IntfObj, 0, sizeof(*IntfObj));

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
    const char                              *DestName       = luaL_checkstring(lua, 1);
    const char                              *IndicationName = luaL_optstring(lua, 3, "indication");
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj;
    const EdsLib_DatabaseObject_t           *GD;
    EdsLib_IntfDB_InterfaceInfo_t            IntfInfo;
    int32_t                                  Status;

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

    GD     = CFE_MissionLib_GetParent(IntfObj->IntfDB);
    Status = EdsLib_IntfDB_FindComponentInterfaceByFullName(GD, DestName, &IntfObj->IntfEdsId);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    Status = CFE_MissionLib_FindTopicIdFromIntfId(IntfObj->IntfDB, IntfObj->IntfEdsId, &IntfObj->TopicId);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return 0;
    }

    /* Now figure out what the indication (aka command) ID is */
    Status = EdsLib_IntfDB_GetComponentInterfaceInfo(GD, IntfObj->IntfEdsId, &IntfInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    IntfObj->IsTelecommand = EdsLib_Is_Similar(IntfInfo.IntfTypeEdsId, CFE_SB_TELECOMMAND_INTF_ID);
    IntfObj->IsTelemetry   = EdsLib_Is_Similar(IntfInfo.IntfTypeEdsId, CFE_SB_TELEMETRY_INTF_ID);

    Status =
        EdsLib_IntfDB_FindCommandByLocalName(GD, IntfInfo.IntfTypeEdsId, IndicationName, &IntfObj->IndicationEdsId);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    /* Now we can finally determine the argument type */
    Status = EdsLib_IntfDB_FindAllArgumentTypes(GD,
                                                IntfObj->IndicationEdsId,
                                                IntfObj->IntfEdsId,
                                                &IntfObj->IndicationBaseArg,
                                                1);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    return 1;
}

static int CFE_MissionLib_Lua_InterfaceObjectGetProperty(lua_State *lua)
{
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj = luaL_checkudata(lua, 1, "CFE_MissionLib_Lua_Interface");
    EdsLib_IntfDB_InterfaceInfo_t            IntfInfo;
    int32_t                                  Status;
    const char                              *Str;
    char                                     StringBuffer[256];
    int                                      retval = 0;

    if (lua_type(lua, 2) == LUA_TSTRING)
    {
        const char *PropName = lua_tostring(lua, 2);

        if (strcmp(PropName, "IntfEdsId") == 0)
        {
            lua_pushinteger(lua, IntfObj->IntfEdsId);
            retval = 1;
        }
        else if (strcmp(PropName, "TopicId") == 0)
        {
            lua_pushinteger(lua, IntfObj->TopicId);
            retval = 1;
        }
        else if (strcmp(PropName, "IndicationEdsId") == 0)
        {
            lua_pushinteger(lua, IntfObj->IndicationEdsId);
            retval = 1;
        }
        else if (strcmp(PropName, "InstanceNumber") == 0)
        {
            lua_pushinteger(lua, IntfObj->InstanceNumber);
            retval = 1;
        }
        else if (strcmp(PropName, "IntfName") == 0)
        {
            /* This is really the interface type name */
            Status = EdsLib_IntfDB_GetComponentInterfaceInfo(CFE_MissionLib_GetParent(IntfObj->IntfDB),
                                                             IntfObj->IntfEdsId,
                                                             &IntfInfo);
            if (Status == EDSLIB_SUCCESS)
            {
                Status = EdsLib_IntfDB_GetFullName(CFE_MissionLib_GetParent(IntfObj->IntfDB),
                                                   IntfInfo.IntfTypeEdsId,
                                                   StringBuffer,
                                                   sizeof(StringBuffer));
            }

            if (Status == EDSLIB_SUCCESS)
            {
                lua_pushstring(lua, StringBuffer);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "TopicName") == 0)
        {
            Status = EdsLib_IntfDB_GetFullName(CFE_MissionLib_GetParent(IntfObj->IntfDB),
                                               IntfObj->IntfEdsId,
                                               StringBuffer,
                                               sizeof(StringBuffer));
            if (Status == EDSLIB_SUCCESS)
            {
                lua_pushstring(lua, StringBuffer);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "IndicationName") == 0)
        {
            Status = EdsLib_IntfDB_GetFullName(CFE_MissionLib_GetParent(IntfObj->IntfDB),
                                               IntfObj->IndicationEdsId,
                                               StringBuffer,
                                               sizeof(StringBuffer));
            if (Status == EDSLIB_SUCCESS)
            {
                lua_pushstring(lua, StringBuffer);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "InstanceName") == 0)
        {
            Str = CFE_MissionLib_GetInstanceNameOrNull(IntfObj->IntfDB, IntfObj->InstanceNumber);
            if (Str != NULL && Str[0] != 0)
            {
                lua_pushstring(lua, Str);
                retval = 1;
            }
        }
        else if (strcmp(PropName, "MsgId") == 0)
        {
            EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSub;
            CFE_MissionLib_Lua_MapPubSubParams(&PubSub, IntfObj);
            lua_pushinteger(lua, PubSub.MsgId.Value);
            retval = 1;
        }
    }

    return retval;
}

static int CFE_MissionLib_Lua_InterfaceObjectEqual(lua_State *lua)
{
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj1 = luaL_testudata(lua, 1, "CFE_MissionLib_Lua_Interface");
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj2 = luaL_testudata(lua, 2, "CFE_MissionLib_Lua_Interface");

    if (IntfObj1 == NULL || IntfObj2 == NULL)
    {
        lua_pushboolean(lua, 0);
    }
    else if (IntfObj1->IntfDB != IntfObj2->IntfDB || EdsLib_Is_Match(IntfObj1->IntfEdsId, IntfObj2->IntfEdsId)
             || EdsLib_Is_Match(IntfObj1->IndicationEdsId, IntfObj2->IndicationEdsId)
             || IntfObj1->TopicId != IntfObj2->TopicId || IntfObj1->InstanceNumber != IntfObj2->InstanceNumber)
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
    char                                     StringBuffer[128];
    const char                              *Str;
    int32_t                                  Status;
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj = luaL_checkudata(lua, 1, "CFE_MissionLib_Lua_Interface");

    int top_start = lua_gettop(lua);
    int top_end;

    Str = CFE_MissionLib_GetInstanceNameOrNull(IntfObj->IntfDB, IntfObj->InstanceNumber);
    if (Str != NULL && Str[0] != 0)
    {
        lua_pushstring(lua, Str);
        lua_pushstring(lua, ":");
    }
    else
    {
        lua_pushnumber(lua, IntfObj->InstanceNumber);
        lua_tostring(lua, -1);
        lua_pushstring(lua, ":");
    }

    Status = EdsLib_IntfDB_GetFullName(CFE_MissionLib_GetParent(IntfObj->IntfDB),
                                       IntfObj->IntfEdsId,
                                       StringBuffer,
                                       sizeof(StringBuffer));
    if (Status == EDSLIB_SUCCESS)
    {
        lua_pushstring(lua, StringBuffer);
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
    const EdsLib_Lua_Database_Userdata_t          *DbObj   = lua_touserdata(lua, lua_upvalueindex(1));
    const CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj = luaL_checkudata(lua, 1, "CFE_MissionLib_Lua_Interface");
    const char                                    *CommandName = luaL_optstring(lua, 2, NULL);
    EdsLib_DataTypeDB_DerivedTypeInfo_t            DerivInfo;
    EdsLib_LuaBinding_DescriptorObject_t          *ObjectUserData;
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t       PubSub;
    EdsLib_Id_t                                    PossibleId;
    uint16_t                                       DerivIdx;
    int32_t                                        Status;

    Status = EdsLib_DataTypeDB_GetDerivedInfo(DbObj->GD, IntfObj->IndicationBaseArg, &DerivInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        /* This would suggest that the base argument is invalid/incorrect */
        return 0;
    }

    ObjectUserData        = EdsLib_LuaBinding_CreateEmptyObject(lua, DerivInfo.MaxSize.Bytes);
    ObjectUserData->GD    = DbObj->GD;
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

            if (EdsLib_DataTypeDB_GetDerivedTypeById(DbObj->GD, IntfObj->IndicationBaseArg, DerivIdx, &PossibleId)
                    == EDSLIB_SUCCESS
                && strcmp(EdsLib_DisplayDB_GetBaseName(DbObj->GD, PossibleId), CommandName) == 0)
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
    const EdsLib_Lua_Database_Userdata_t    *DbObj            = lua_touserdata(lua, lua_upvalueindex(1));
    const uint8_t                           *SourceBuffer     = (const uint8_t *)luaL_checkstring(lua, 1);
    uint32_t                                 SourceBufferSize = lua_rawlen(lua, 1);
    const char                              *IndicationName   = luaL_optstring(lua, 2, "indication");
    EdsLib_Binding_DescriptorObject_t       *ObjectUserData;
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj;
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSub;
    EdsLib_DataTypeDB_DerivedTypeInfo_t      DerivInfo;
    EdsLib_IntfDB_InterfaceInfo_t            IntfInfo;
    EdsLib_SizeInfo_t                        MaxSize;
    EdsLib_SizeInfo_t                        DecodeSize;
    CFE_MissionLib_TopicInfo_t               TopicInfo;
    const EdsLib_DatabaseObject_t           *GD;
    EdsLib_Id_t                              EdsId;
    int32_t                                  Status;

    EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), EdsContainer_CFE_HDR_Message_DATADICTIONARY);
    Status = EdsLib_DataTypeDB_GetDerivedInfo(DbObj->GD, EdsId, &DerivInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    memset(&DecodeSize, 0, sizeof(DecodeSize));

    ObjectUserData        = EdsLib_LuaBinding_CreateEmptyObject(lua, DerivInfo.MaxSize.Bytes);
    ObjectUserData->GD    = DbObj->GD;
    ObjectUserData->EdsId = EdsId;

    MaxSize.Bytes = DerivInfo.MaxSize.Bytes;
    MaxSize.Bits  = EdsLib_OCTETS_TO_BITS(SourceBufferSize);

    Status = EdsLib_DataTypeDB_UnpackPartialObjectVarSize(DbObj->GD,
                                                          &EdsId,
                                                          EdsLib_Binding_GetNativeObject(ObjectUserData),
                                                          SourceBuffer,
                                                          &MaxSize,
                                                          &DecodeSize);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    ObjectUserData->EdsId = EdsId;
    CFE_MissionLib_Get_PubSub_Parameters(&PubSub, EdsLib_Binding_GetNativeObject(ObjectUserData));

    IntfObj = CFE_MissionLib_Lua_NewInterfaceObject(lua, lua_upvalueindex(1));
    GD      = CFE_MissionLib_GetParent(IntfObj->IntfDB);

    /* It is not clear if this is a TLM or CMD, so need to try both, only one will work */
    if (IntfObj->TopicId == 0)
    {
        EdsComponent_CFE_SB_Listener_t Result;
        CFE_MissionLib_UnmapListenerComponent(&Result, &PubSub);
        IntfObj->TopicId        = Result.Telecommand.TopicId;
        IntfObj->InstanceNumber = Result.Telecommand.InstanceNumber;
        IntfObj->IsTelecommand  = true;
    }
    if (IntfObj->TopicId == 0)
    {
        EdsComponent_CFE_SB_Publisher_t Result;
        CFE_MissionLib_UnmapPublisherComponent(&Result, &PubSub);
        IntfObj->TopicId        = Result.Telemetry.TopicId;
        IntfObj->InstanceNumber = Result.Telemetry.InstanceNumber;
        IntfObj->IsTelemetry    = true;
    }

    Status = CFE_MissionLib_GetTopicInfo(IntfObj->IntfDB, IntfObj->TopicId, &TopicInfo);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return 0;
    }

    IntfObj->IntfEdsId = TopicInfo.ParentIntfId;

    /* Now figure out what the indication (aka command) ID is */
    Status = EdsLib_IntfDB_GetComponentInterfaceInfo(GD, IntfObj->IntfEdsId, &IntfInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    Status =
        EdsLib_IntfDB_FindCommandByLocalName(GD, IntfInfo.IntfTypeEdsId, IndicationName, &IntfObj->IndicationEdsId);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    /* Now we can finally determine the argument type */
    Status = EdsLib_IntfDB_FindAllArgumentTypes(GD,
                                                IntfObj->IndicationEdsId,
                                                IntfObj->IntfEdsId,
                                                &IntfObj->IndicationBaseArg,
                                                1);
    if (Status != EDSLIB_SUCCESS)
    {
        return 0;
    }

    /* Beyond this, the IntfObj is considered OK so return 1 */
    EdsId  = IntfObj->IndicationBaseArg;
    Status = EdsLib_DataTypeDB_UnpackPartialObjectVarSize(DbObj->GD,
                                                          &EdsId,
                                                          EdsLib_Binding_GetNativeObject(ObjectUserData),
                                                          SourceBuffer,
                                                          &MaxSize,
                                                          &DecodeSize);
    if (Status != EDSLIB_SUCCESS)
    {
        return 1;
    }

    Status = EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(DbObj->GD,
                                                           EdsId,
                                                           EdsLib_Binding_GetNativeObject(ObjectUserData),
                                                           SourceBuffer,
                                                           EDSLIB_DATATYPEDB_RECOMPUTE_NONE,
                                                           &DecodeSize);
    if (Status == EDSLIB_SUCCESS)
    {
        return 1;
    }

    ObjectUserData->EdsId = EdsId;
    EdsLib_DataTypeDB_GetTypeInfo(DbObj->GD, ObjectUserData->EdsId, &ObjectUserData->TypeInfo);

    return 2;
}

void CFE_MissionLib_Lua_SoftwareBus_Attach(lua_State *lua, const CFE_MissionLib_SoftwareBus_Interface_t *IntfDB)
{
    int                             Obj   = lua_gettop(lua);
    EdsLib_Lua_Database_Userdata_t *DbObj = luaL_checkudata(lua, Obj, "EdsDb");
    if (DbObj->GD == NULL)
    {
        /* This is a bug in the caller, better to complain loudly than silently returning */
        abort();
    }

    lua_getuservalue(lua, -1);

    lua_pushlightuserdata(lua, (void *)IntfDB);
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
