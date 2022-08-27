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
 * \file     seds_generic_props.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of a generic "properties" concept for DOM nodes.  Functions
 * in here utilize the "uservalue" table embedded within Lua userdata objects
 * to provide an arbitrary key/value store for DOM objects.
 *
 * It is implemented separately, as the same concept is utilized across several
 * different DOM node types and sharing the implementation provides consistent
 * behavior across all of them.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "seds_generic_props.h"

/**
 * Local registry key for tracking the history of generic properties on objects
 * This can be helpful for debugging, to locate where a property was set or modified.
 */
static const char PROPERTY_HISTORY_KEY;

/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/



/* ------------------------------------------------------------------- */
/**
 * Lua callable helper function to get the tracking data of a given property name
 *
 * Expected input stack:
 *  1: Property name
 *
 * The returned object should be a table, containing actions as keys to
 * sub-tables that have script name as key, and a count value of actions by that script
 */
static int seds_generic_props_get_history(lua_State *lua)
{
    lua_settop(lua, 1);
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &PROPERTY_HISTORY_KEY);
    lua_pushvalue(lua, 1);
    lua_rawget(lua, 2);
    return 1;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable helper function to implement tracking of property names
 *
 * Expected input stack:
 *  1: <not used>
 *  2: Property name
 *
 * The stack may have more entries, which are also not referenced.  This function
 * uses relative stack addressing except for index 2.
 */
static void seds_generic_props_do_tracking(lua_State *lua, const char *action_name)
{
    int start_top = lua_gettop(lua);

    lua_rawgetp(lua, LUA_REGISTRYINDEX, &PROPERTY_HISTORY_KEY);
    if (lua_istable(lua, start_top + 1))
    {
        lua_pushvalue(lua, 2);
        lua_rawget(lua, start_top + 1);
        if (!lua_istable(lua, start_top + 2))
        {
            lua_pop(lua, 1);
            lua_newtable(lua);
            lua_pushvalue(lua, 2);
            lua_pushvalue(lua, start_top + 2);
            lua_rawset(lua, start_top + 1);
        }

        /* start_top + 2 should be the history record (table) for this property */
        luaL_getsubtable(lua, start_top + 2, action_name);
        lua_rawgetp(lua, LUA_REGISTRYINDEX, &sedstool.CURRENT_SCRIPT_KEY);
        if (!lua_isnil(lua, -1))
        {
            lua_pushvalue(lua, -1);
            lua_rawget(lua, start_top + 3);
            if (lua_type(lua, -1) == LUA_TNUMBER)
            {
                lua_pushinteger(lua, 1);
                lua_arith(lua, LUA_OPADD);
            }
            else
            {
                lua_pop(lua, 1);
                lua_pushinteger(lua, 1);
            }
            lua_rawset(lua, start_top + 3);
        }
    }

    lua_settop(lua, start_top);
}

/*******************************************************************************/
/*                      Externally-Called Functions                            */
/*      (referenced outside this unit and prototyped in a separate header)     */
/*******************************************************************************/


/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
int seds_generic_props_get_property(lua_State *lua)
{
    /* exactly two arguments should have been passed */
    lua_settop(lua, 2);

    /*
     * The "uservalue" is an embedded Lua table that contains all
     * the key/value pair mappings.
     */
    lua_getuservalue(lua, 1);
    if (lua_type(lua, -1) != LUA_TTABLE)
    {
        lua_pop(lua, 1);
        lua_pushnil(lua);
    }
    else
    {
        /*
         * gets are more frequent and therefore this is
         * more of a performance hit than set tracking,
         * so only do it in debug mode
         */
        if (sedstool.verbosity >= 2)
        {
            seds_generic_props_do_tracking(lua, "access");
        }

        lua_pushvalue(lua, 2);
        lua_rawget(lua, -2);
    }

    return 1;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
int seds_generic_props_set_property(lua_State *lua)
{
    const char *action_name;

    /* exactly two arguments should have been passed */
    lua_settop(lua, 3);

    lua_getuservalue(lua, 1);
    if (lua_type(lua, 4) == LUA_TTABLE)
    {
        /*
         * Property value accounting and history tracking...
         * For debugging purposes it is helpful to identify where
         * properties are created and modified.
         *
         * this is a bit of a performance hit,
         * so only do it in verbose mode
         */
        if (sedstool.verbosity >= 1)
        {
            lua_pushvalue(lua, 2);
            lua_rawget(lua, 4);
            if (lua_isnil(lua, 5))
            {
                if (lua_isnil(lua, 3))
                {
                    action_name = "none";
                }
                else
                {
                    action_name = "create";
                }
            }
            else
            {
                if (lua_isnil(lua, 3))
                {
                    action_name = "delete";
                }
                else
                {
                    action_name = "modify";
                }
            }
            lua_pop(lua, 1);

            seds_generic_props_do_tracking(lua, action_name);
        }


        /*
         * Do the actual property value set
         */
        lua_pushvalue(lua, 2);
        lua_pushvalue(lua, 3);
        lua_rawset(lua, 4);
    }
    lua_pop(lua, 1);

    return 0;
}


/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
int seds_generic_props_enumerate_properties(lua_State *lua)
{
    luaL_checktype(lua, 1, LUA_TUSERDATA);
    lua_newtable(lua);
    lua_getuservalue(lua, 1);
    luaL_checktype(lua, 3, LUA_TTABLE);
    lua_pushnil(lua);
    while(lua_next(lua, 3) != 0)
    {
        lua_pushvalue(lua, 4);
        lua_rawseti(lua, 2, 1 + lua_rawlen(lua, 2));
        lua_pop(lua, 1);
    }
    lua_pop(lua, 1);
    return 1;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_generic_props_register_globals(lua_State *lua)
{
    /*
     * Create an entry in the register for tracking the history
     * of SEDS node property values.  This can be helpful for tracking
     * down the originator of a property during testing and debug.
     */
    lua_newtable(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &PROPERTY_HISTORY_KEY);

    lua_pushcfunction(lua, seds_generic_props_get_history);
    lua_setfield(lua, -2, "get_property_history");
}
