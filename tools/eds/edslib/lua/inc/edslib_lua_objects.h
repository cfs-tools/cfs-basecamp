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
 * \file     edslib_lua_objects.h
 * \ingroup  lua
 * \author   joseph.p.hickey@nasa.gov
 *
 * API for EdsLib-Lua binding library
 */

#ifndef _EDSLIB_LUA_OBJECTS_H_
#define _EDSLIB_LUA_OBJECTS_H_


#include <edslib_datatypedb.h>
#include <edslib_displaydb.h>

/*
 * Abstract types for the Lua binding
 * These are not distinct types, but intended to be aliases of types defined elsewhere.
 * They are aliases to avoid creating a dependency on the header file which actually defines them.
 */
typedef struct lua_State                        EdsLib_LuaBinding_State_t;
typedef struct EdsLib_Binding_DescriptorObject  EdsLib_LuaBinding_DescriptorObject_t;
typedef struct EdsLib_DatabaseObject            EdsLib_LuaBinding_DatabaseObject_t;

typedef struct
{
    const EdsLib_LuaBinding_DatabaseObject_t *GD;
} EdsLib_Lua_Database_Userdata_t;

void EdsLib_LuaBinding_GetNativeObject(EdsLib_LuaBinding_State_t *lua, int narg, void **OutPtr, size_t *SizeBuf);
EdsLib_LuaBinding_DescriptorObject_t *EdsLib_LuaBinding_CreateEmptyObject(EdsLib_LuaBinding_State_t *lua, size_t MaxSize);
void EdsLib_Lua_Attach(EdsLib_LuaBinding_State_t *lua, const EdsLib_LuaBinding_DatabaseObject_t *MissionObj);


#endif  /* _EDSLIB_LUA_OBJECTS_H_ */

