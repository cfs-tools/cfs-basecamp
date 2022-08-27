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
 * \file     cfe_missionlib_lua_softwarebus.h
 * \ingroup  lua
 * \author   joseph.p.hickey@nasa.gov
 *
 */

#ifndef _CFE_MISSIONLIB_LUA_SOFTWAREBUS_H_
#define _CFE_MISSIONLIB_LUA_SOFTWAREBUS_H_


#include <lua.h>
#include "cfe_missionlib_api.h"


typedef struct
{
    const CFE_MissionLib_SoftwareBus_Interface_t *IntfDB;
    uint16_t InstanceNumber;
    uint16_t IntfId;
    uint16_t TopicId;
    uint16_t IndicationId;
    EdsLib_Id_t IndicationBaseArg;
} CFE_MissionLib_Lua_Interface_Userdata_t;


void CFE_MissionLib_Lua_SoftwareBus_Attach(lua_State *lua, const CFE_MissionLib_SoftwareBus_Interface_t *IntfDB);

#endif  /* _CFE_MISSIONLIB_LUA_SOFTWAREBUS_H_ */

