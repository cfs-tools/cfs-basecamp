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
 * \file     seds_instance_node.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implements methods for "instance nodes" within the SEDS DOM
 *
 * Instance nodes refer to specific instantiations of SEDS components.  This is a deployment
 * construct and is not directly specfied in the SEDS schema, but this information is required
 * to assemble an actual mission dictionary from EDS files.  These objects are created based
 * on the "InstanceRule" element types in the design parameter XML files.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "seds_global.h"
#include "seds_generic_props.h"
#include "seds_instance_node.h"
#include "seds_tree_node.h"
#include "seds_plugin.h"

/*
 * Many of the SEDS tree methods are implemented in Lua,
 * which are defined in the external module file.
 * This module is loaded when the tree functions are registered.
 */
static const char SEDS_INSTANCE_METHODS_SCRIPT_FILE[] = "seds_instance_methods.lua";
static const char SEDS_INSTANCE_PROPERTIES_LUA_KEY = '\0';



/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/



/* ------------------------------------------------------------------- */
/**
 * Helper function to compare two "seds_instance" objects
 *
 * In many cases the arrangement of the DOM in memory is dependent
 * upon the order with which files were originally processed.  Sorting
 * internal structures can provide consistent output in cases where
 * the input differed only in the order of objects, with identical
 * content.
 *
 * This provides a comparison function which operates primarily
 * on "name" and secondarily on "id" to sort nodes consistently
 */
static int seds_instance_node_compare(lua_State *lua)
{
    int op = lua_tointeger(lua, lua_upvalueindex(1));
    luaL_checkudata(lua, 1, "seds_instance");
    luaL_checkudata(lua, 2, "seds_instance");
    lua_getuservalue(lua, 1);
    lua_getuservalue(lua, 2);
    lua_getfield(lua, 3, "name");
    lua_getfield(lua, 4, "name");
    if (lua_isnil(lua, 5) ||
            lua_type(lua, 5) != lua_type(lua, 6) ||
            lua_compare(lua, 5, 6, LUA_OPEQ))
    {
        lua_pop(lua, 2);
        lua_getfield(lua, 3, "id");
        lua_getfield(lua, 4, "id");
    }

    lua_pushboolean(lua, lua_compare(lua, 5, 6, op));
    return 1;
}

/* ------------------------------------------------------------------- */
/**
 * Helper function to obtain a property of an instance object
 *
 * This first attempts to look up the property key in the static
 * table, which is used for methods that should exist on every
 * object.  If not present in the static method table, then
 * look up the key in the local value store.
 *
 * Since the static method table is read-only there is no
 * special set function; the default setter is used.
 *
 * Expected Stack args:
 *  1: tree node object
 *  2: property key
 */
static int seds_instance_node_get_property(lua_State *lua)
{
    /* exactly two arguments should have been passed */
    lua_settop(lua, 2);

    /*
     * First attempt to look up the value in the static tree method table
     * If this returns non-nil then use that value for the property
     * (This handles all tree methods/functions)
     */
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &SEDS_INSTANCE_PROPERTIES_LUA_KEY);
    lua_pushvalue(lua, 2);
    lua_rawget(lua, 3);

    /*
     * If the code above populated stack position 4,
     * then return that value to the caller
     */
    if (!lua_isnoneornil(lua, 4))
    {
        return 1;
    }

    /*
     * Otherwise try retrieving the key from the uservalue table
     */
    return seds_generic_props_get_property(lua);
}

/* ------------------------------------------------------------------- */
/**
 * Helper function to create an instance object
 *
 * This will push a new instance node onto the stack.
 *
 * Expected input stack args:
 *   1: Component node (seds_node, required) that defines the instance
 *   2: Rule node (seds_node, required) that the instance is satisfying
 *   3: Trigger node (seds_node, optional) if this is an "on-demand" instance, this
 *     indicates the node which needed this instance to be created.
 */
static int seds_new_component_instance(lua_State *lua)
{
    seds_node_t *compnode = luaL_checkudata(lua, 1, "seds_node");
    seds_node_t *rulenode = luaL_checkudata(lua, 2, "seds_node");
    seds_node_t *trigger;
    int instance_pos;

    if (lua_gettop(lua) >= 3)
    {
        trigger = luaL_checkudata(lua, 3, "seds_node");
    }
    else
    {
        trigger = NULL;
    }

    luaL_argcheck(lua, compnode->node_type == SEDS_NODETYPE_COMPONENT, 1, "component expected");
    luaL_argcheck(lua, rulenode->node_type == SEDS_NODETYPE_INSTANCE_RULE, 2, "rule expected");

    void **pinst = lua_newuserdata(lua, sizeof(void *));
    memset(pinst, 0, sizeof(*pinst));
    instance_pos = lua_gettop(lua);

    if (luaL_newmetatable(lua, "seds_instance"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_instance_node_get_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, seds_generic_props_set_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__lt");
        lua_pushinteger(lua, LUA_OPLT);
        lua_pushcclosure(lua, seds_instance_node_compare, 1);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__le");
        lua_pushinteger(lua, LUA_OPLE);
        lua_pushcclosure(lua, seds_instance_node_compare, 1);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    lua_newtable(lua);
    lua_pushvalue(lua, 1);
    lua_setfield(lua, -2, "component");
    lua_pushvalue(lua, 2);
    lua_setfield(lua, -2, "rule");
    if (trigger != NULL)
    {
        lua_pushvalue(lua, 3);
        lua_setfield(lua, -2, "trigger");
    }
    lua_newtable(lua);
    lua_setfield(lua, -2, "provided_links");
    lua_newtable(lua);
    lua_setfield(lua, -2, "required_links");
    lua_setuservalue(lua, -2);

    /*
     * Get the uservalue table from the rule, and append
     * this object to the "instance_list" subtable
     */
    lua_getuservalue(lua, 2);
    luaL_getsubtable(lua, -1, "instance_list");
    lua_pushvalue(lua, instance_pos);
    lua_rawseti(lua, -2, 1 + lua_rawlen(lua, -2));

    lua_settop(lua, instance_pos);

    return 1;
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
void seds_instance_node_register_globals(lua_State *lua)
{
    luaL_checktype(lua, -1, LUA_TTABLE);

    lua_pushcfunction(lua, seds_new_component_instance);
    lua_setfield(lua, -2, "new_component_instance");

    seds_plugin_load_lua(lua, SEDS_INSTANCE_METHODS_SCRIPT_FILE);
    lua_call(lua, 0, 1);
    luaL_checktype(lua, -1, LUA_TTABLE);

    lua_pushcfunction(lua, seds_generic_props_enumerate_properties);
    lua_setfield(lua, -2, "get_properties");

    lua_rawsetp(lua, LUA_REGISTRYINDEX, &SEDS_INSTANCE_PROPERTIES_LUA_KEY);
}

