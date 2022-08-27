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
 * \file     test_interface.c
 * \ingroup  testexecutive
 * \author   joseph.p.hickey@nasa.gov
 *
 * Interface between test executive and test targets
 */

#include <stdlib.h>
#include <string.h> /* memset() */
#include <time.h>
#include <errno.h>
#include <sys/types.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "osapi.h"

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_displaydb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"
#include "edslib_lua_objects.h"
#include "cfe_missionlib_lua_softwarebus.h"

#include "testexec.h"

typedef struct
{
    uint16 TargetNum;
} TestIntf_t;

typedef struct
{
    const char *LuaName;
    int (*InitFunc)(lua_State *lua);
} TestExec_Intf_ModuleEntry_t;

#define TESTEXEC_MODULE(x)  extern int TestIntf_ ## x ## _Create(lua_State *lua);
#include "testexec_compiledin_modules.h"
#undef TESTEXEC_MODULE

#define TESTEXEC_MODULE(x) { #x, TestIntf_ ## x ## _Create },
static const TestExec_Intf_ModuleEntry_t TESTEXEC_INTF_TABLE[] =
{
#include "testexec_compiledin_modules.h"
        { NULL, NULL }
};
#undef TESTEXEC_MODULE


static int TestIntf_Preprocess_Message(lua_State *lua)
{
    printf("%s(): Start\n", __func__);

    /*
     * Expected arguments:
     *  idx@1 - received raw/packed message object (string)
     */
    lua_settop(lua, 1);
    lua_newtable(lua);

    lua_getglobal(lua, "EdsDB");
    lua_getfield(lua, -1, "IdentifyMessage");
    lua_pushvalue(lua, 1);
    lua_call(lua, 1, 2);

    /*
     * Expected after call:
     *  idx@-1 - MissionLib interface object
     *  idx@-2 - EdsLib data object
     */
    printf("%s(): Intf=%s\n", __func__,
            luaL_tolstring(lua, -1, NULL));
    lua_pop(lua, 1);

    printf("%s(): EdsObj=%s\n", __func__,
            luaL_tolstring(lua, -2, NULL));
    lua_pop(lua, 1);

    lua_setfield(lua, 2, "Interface");
    lua_setfield(lua, 2, "Object");
    lua_settop(lua, 2);

    printf("%s(): End\n", __func__);

    return 1;
}

static int TestIntf_Filter_Message(lua_State *lua)
{
    int is_match;

    /*
     * Expected arguments:
     *  idx@1 - filter criteria (table; may contain field "Interface")
     *  idx@2 - preprocessed event data (table; must contain field "Interface" and "Object")
     *
     * If interfaces match, then return the object.
     * If no match, then return nothing.
     */
    luaL_checktype(lua, 1, LUA_TTABLE);
    luaL_checktype(lua, 2, LUA_TTABLE);
    lua_settop(lua, 2);
    lua_getfield(lua, 1, "Interface");
    if (lua_isnil(lua, -1))
    {
        /*
         * if user did not specify an interface, then do not match anything.
         * (user could be looking for events other than messages)
         */
        is_match = 0;
    }
    else
    {
        lua_getfield(lua, 2, "Interface");
        is_match = lua_compare(lua, 3, 4, LUA_OPEQ);
    }

    if (is_match)
    {
        /* return the object matched */
        lua_getfield(lua, 2, "Object");
        return 1;
    }

    /* no match */
    return 0;
}

static int TestIntf_WaitAction(lua_State *lua)
{
    int32_t Timeout = luaL_optinteger(lua, 2, -1);
    struct timespec expire;
    struct timespec current;
    int nret;
    uint32_t conditions_req;
    uint32_t conditions_met;
    enum
    {
        WAIT_STATE_NORMAL = 0,
        WAIT_STATE_SLEEP
    } waitstate;

    printf("%s(): Start\n", __func__);
    nret = 0;
    luaL_checktype(lua, 1, LUA_TTABLE);
    lua_settop(lua, 2);
    lua_getfield(lua, lua_upvalueindex(1), "API");   /* @3 (userdata object = low level intf) */
    waitstate = WAIT_STATE_NORMAL;

    clock_gettime(CLOCK_MONOTONIC, &expire);
    if (Timeout >= 0)
    {
        expire.tv_sec += Timeout / 1000;
        expire.tv_nsec += (Timeout % 1000) * 1000000;
    }
    if (expire.tv_nsec > 1000000000)
    {
        ++expire.tv_sec;
        expire.tv_nsec -= 1000000000;
    }

    while(1)
    {
        lua_settop(lua, 3);

        printf("%s(): Poll\n", __func__);
        lua_getfield(lua, 3, "Poll"); /* idx 4 = poll function */
        lua_pushvalue(lua, 3);
        lua_call(lua, 1, 3);

        /*
         * After call:
         *  idx@4 = type of event ("Message", etc)
         *  idx@5 = event data object
         *  idx@6 = additional event attributes
         */
        printf("%s(): Stack= %s %s %s\n", __func__,
                lua_typename(lua, lua_type(lua, 4)),
                lua_typename(lua, lua_type(lua, 5)),
                lua_typename(lua, lua_type(lua, 6)));
        if (!lua_isnil(lua, 4))
        {
            /*
             * Preprocess returned data
             */
            if (!lua_isnil(lua, 5))
            {
                lua_getfield(lua, lua_upvalueindex(1), "RecvProcessor");/* idx@7 - preprocessor table */
                lua_pushvalue(lua, 4);                                  /* idx@8 - event type */
                lua_gettable(lua, -2);                                  /* idx@8 - preprocessor func */
                if (lua_isfunction(lua, -1))
                {
                    lua_pushvalue(lua, 5);                              /* idx@9 - event data object */
                    lua_call(lua, 1, 1);                                /* idx@8 - preprocessed event data obj */
                    lua_replace(lua, 5);                                /* replace original event data obj */
                }
                lua_settop(lua, 6);
            }

            /*
             * check if the matches have been satisfied
             */
            conditions_req = 0;
            conditions_met = 0;
            lua_pushnil(lua);                           /* idx @7 - expected event table index */
            while(lua_next(lua, 1))                     /* idx @8 - expected event table value */
            {
                ++conditions_req;
                if (lua_type(lua, 8) == LUA_TTABLE)
                {
                    lua_getfield(lua, 8, "Matched");    /* idx @9 - already matched? */
                    if (lua_toboolean(lua, -1))
                    {
                        ++conditions_met;
                    }
                    else
                    {
                        lua_getfield(lua, lua_upvalueindex(1), "RecvFilter");   /* idx@10 - filter table */
                        lua_pushvalue(lua, 4);                                  /* idx@11 - event type */
                        lua_gettable(lua, -2);                                  /* idx@11 - filter func */
                        if (lua_isfunction(lua, -1))
                        {
                            lua_pushvalue(lua, 8);                              /* idx@12 - filter criteria table */
                            lua_pushvalue(lua, 5);                              /* idx@13 - preprocessed event data */
                            lua_call(lua, 2, 1);                                /* idx@11 - object (match) or nil (no match) */
                            if (lua_toboolean(lua, -1))
                            {
                                /* if the user specified a callback,
                                 * call it now and use its return value.
                                 * It may return nil or false to drop/reject the event.
                                 */
                                lua_getfield(lua, 8, "Callback");
                                if (lua_isfunction(lua, -1))
                                {
                                    lua_pushvalue(lua, -2);
                                    lua_call(lua, 1, 1);
                                }
                                else
                                {
                                    lua_pop(lua, 1);
                                }

                                if (lua_toboolean(lua, -1))
                                {
                                    lua_setfield(lua, 8, "Matched");
                                    lua_pushvalue(lua, 6);
                                    lua_setfield(lua, 8, "Attribs");
                                    ++conditions_met;
                                }
                            }
                        }
                    }
                }
                lua_settop(lua, 7); /* remove everything above event table index */
            }

            if (conditions_met >= conditions_req)
            {
                lua_pushboolean(lua, 1);
                nret = 1;
                break;
            }

            /* not all conditions met so poll again for more data */
        }
        else
        {
            /* Poll function returned nothing, so need to wait */

            /*
             * If a timeout was specified, check if we have exceeded the timeout yet.
             * If timeout was not passed yet then call a low level wait routine
             */
            if (Timeout >= 0)
            {
                clock_gettime(CLOCK_MONOTONIC, &current);
                current.tv_sec = expire.tv_sec - current.tv_sec;
                current.tv_nsec = expire.tv_nsec - current.tv_nsec;
                if (current.tv_nsec < 0)
                {
                    --current.tv_sec;
                    current.tv_nsec += 1000000000;
                }

                if (current.tv_sec < 0)
                {
                    /* timeout occurred */
                    break;
                }

                if (current.tv_sec == 0)
                {
                    if (current.tv_nsec == 0)
                    {
                        /* timeout occurred */
                        break;
                    }

                    if (current.tv_nsec < 1000000)
                    {
                        /* wait a minimum of 1ms -
                         * this may exceed original timeout,
                         * but that may be less bad than returning prior to the requested timeout.
                         */
                        current.tv_nsec = 1000000;
                    }
                }
            }
            else
            {
                current.tv_sec = 1;
                current.tv_nsec = 0;
            }

            /*
             * If the low level interface has a wait routine, then use it.
             * The wait routine should exit and return true if there is any activity
             * that makes it worthwhile to re-poll.
             */
            lua_getfield(lua, 3, "Wait");
            if (!lua_isfunction(lua, -1))
            {
                waitstate = WAIT_STATE_SLEEP;
                lua_pop(lua, 1);
            }
            else
            {
                lua_pushvalue(lua, 3);

                /*
                 * The wait task should return if any relevant activity occurs, so
                 * it is OK to pass in a bigger timeout here.  However to be friendly
                 * to implementations that may have limitations on their timeouts, we
                 * will not ask for anything more than 30 seconds in one shot.
                 */
                if (current.tv_sec < 30)
                {
                    lua_pushinteger(lua, (current.tv_sec * 1000) + (current.tv_nsec / 1000000));
                }
                else
                {
                    lua_pushinteger(lua, 30000);
                }

                lua_call(lua, 2, 1);
                if (!lua_toboolean(lua, -1))
                {
                    waitstate = WAIT_STATE_SLEEP;
                }
                lua_pop(lua, 1);
            }

            if (waitstate == WAIT_STATE_SLEEP)
            {
                /*
                 * Fall back to using sleep-based polling
                 * since this will not automatically wake up early, limit the
                 * sleep time to ~250ms and re-poll.
                 */
                if (current.tv_sec > 0 || current.tv_nsec > 250000000)
                {
                    current.tv_sec = 0;
                    current.tv_nsec = 250000000;
                }
                clock_nanosleep(CLOCK_MONOTONIC, 0, &current, NULL);
                waitstate = WAIT_STATE_NORMAL;
            }
        }
    }

    return nret;
}

static int TestIntf_DoAction(lua_State *lua)
{
    /*
     * Expected inputs:
     *  1 => Generic User argument (object or interface)
     *  2 => Generic User attribute(s)
     *
     *  Upvalue 1 => Low Level intf object (uservalue table from TestIntf object)
     *  Upvalue 2 => Action name (string)
     */
    lua_settop(lua, 2);

    lua_getfield(lua, lua_upvalueindex(1), "API");  /* idx @3 - low level userdata object */
    lua_pushvalue(lua, lua_upvalueindex(2));
    lua_gettable(lua, -2);                          /* idx @4 - action function */
    if (!lua_isfunction(lua, -1))
    {
        return luaL_error(lua, "Action %s undefined", luaL_tolstring(lua, lua_upvalueindex(2), NULL));
    }
    lua_insert(lua, -2);

    /*
     * Stack at this point:
     *  @3 => Action function
     *  @4 => Low level intf object (API) - must be 1st parameter to action function
     */

    if (luaL_testudata(lua, 1, "EdsLib_Object") != NULL)
    {
        /* Desired action is to send the message to the target */
        lua_getglobal(lua, "EdsDB");
        lua_getfield(lua, -1, "Encode");
        lua_replace(lua, -2);
        lua_pushvalue(lua, 1);
        lua_call(lua, 1, 1);

        if (!lua_isstring(lua, -1))
        {
            return luaL_error(lua, "EdsDb Encode method did not return a string");
        }
    }
    else if (luaL_testudata(lua, 1, "CFE_MissionLib_Lua_Interface") != NULL)
    {
        lua_getfield(lua, 1, "MsgId");
    }
    else if (!lua_isnil(lua, 1))
    {
        lua_pushvalue(lua, 1);
    }

    /*
     * Argument 2 is an optional attribute argument
     * it is passed through unmodified if present.
     * this could be a table to specify several attributes if required
     */
    if (!lua_isnil(lua, 2))
    {
        lua_pushvalue(lua, 2);
    }

    /* Call the low level function */
    lua_call(lua, lua_gettop(lua) - 3, 2);  /* idx @3,4 = return values */

    if (lua_type(lua, 3) == LUA_TTABLE)
    {
        lua_pushnil(lua);
        while(lua_next(lua, 3))
        {
            printf("%s(): %s = %s\n", __func__,
                    luaL_tolstring(lua, 5, NULL),
                    luaL_tolstring(lua, 6, NULL));
            lua_settop(lua, 5);
        }
        lua_pushvalue(lua, lua_upvalueindex(1));
        lua_pushcclosure(lua, TestIntf_WaitAction, 1);
        lua_insert(lua, 3);
        lua_call(lua, 2, 0);
    }

    return 0;
}

static int TestIntf_GetAction(lua_State *lua)
{
    luaL_checkudata(lua, 1, "TestIntf");
    lua_settop(lua, 2);

    lua_getuservalue(lua, 1);       /* @3 */

    if (lua_type(lua, 2) == LUA_TSTRING)
    {
        if (strcmp(lua_tostring(lua, 2), "Wait") == 0)
        {
            lua_pushcclosure(lua, TestIntf_WaitAction, 1);
            return 1;
        }
    }

    /* if above did not push a function then try to get it from the low level */
    lua_pushvalue(lua, 2);
    lua_pushcclosure(lua, TestIntf_DoAction, 2);
    return 1;
}


static int TestIntf_Destroy(lua_State *lua)
{
    luaL_checkudata(lua, 1, "TestIntf");
    return 0;
}

static int TestIntf_Create(lua_State *lua)
{
    TestIntf_t *Conn;

    lua_settop(lua, 3);

    Conn = lua_newuserdata(lua, sizeof(TestIntf_t)); /* Index 4 */
    if (luaL_newmetatable(lua, "TestIntf"))
    {
        lua_pushcfunction(lua, TestIntf_GetAction);
        lua_setfield(lua, -2, "__index");

        lua_pushcfunction(lua, TestIntf_Destroy);
        lua_setfield(lua, -2, "__gc");
    }
    lua_setmetatable(lua, -2);

    if (lua_isstring(lua, 2))
    {
        Conn->TargetNum = CFE_MissionLib_GetInstanceNumber(&CFE_SOFTWAREBUS_INTERFACE, lua_tostring(lua, 2));
    }
    else if (lua_isnumber(lua, 2))
    {
        Conn->TargetNum = lua_tointeger(lua, 2);
    }
    else
    {
        Conn->TargetNum = 1;
    }

    luaL_argcheck(lua, Conn->TargetNum != 0, 2, "Invalid target specified");

    /* create a table for use as the userdata */
    lua_newtable(lua);  /* index 5 */

    lua_pushvalue(lua, 1);
    lua_gettable(lua, lua_upvalueindex(1));
    if (!lua_isfunction(lua, -1))
    {
        return luaL_argerror(lua, 1, "Unknown connection type");
    }

    lua_pushinteger(lua, Conn->TargetNum);
    lua_pushvalue(lua, 3);
    lua_call(lua, 2, 1);
    lua_setfield(lua, 5, "API");
    lua_newtable(lua);
    lua_setfield(lua, 5, "RecvEvents");
    lua_newtable(lua);
    lua_pushcfunction(lua, TestIntf_Preprocess_Message);
    lua_setfield(lua, -2, "Message");
    lua_setfield(lua, 5, "RecvProcessor");
    lua_newtable(lua);
    lua_pushcfunction(lua, TestIntf_Filter_Message);
    lua_setfield(lua, -2, "Message");
    lua_setfield(lua, 5, "RecvFilter");
    lua_setuservalue(lua, 4);

    return 1;
} /* end TestIntf_Udp_Create */

int TestIntf_GetFactory(lua_State *lua)
{
    const TestExec_Intf_ModuleEntry_t *ModEnt = TESTEXEC_INTF_TABLE;

    lua_newtable(lua);

    while (ModEnt != NULL &&
            ModEnt->InitFunc != NULL &&
            ModEnt->LuaName != NULL)
    {
        lua_pushcfunction(lua, ModEnt->InitFunc);
        lua_setfield(lua, -2, ModEnt->LuaName);
        ++ModEnt;
    }

    lua_pushcclosure(lua, TestIntf_Create, 1);
    return 1;
}
