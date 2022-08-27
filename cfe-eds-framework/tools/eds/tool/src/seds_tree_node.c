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
 * \file     seds_tree_node.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implements the methods for SEDS Document Object Model (DOM) tree nodes.
 * This is the in-memory representation of the entire set of all EDS documents
 * in a given mission.
 *
 * Most of the tree node methods are actually implemented in an associated Lua
 * source file.  This C module facilitates loading of that Lua code and attaching
 * it into the global Lua state at initialization time, as well as providing
 * some additional helper methods that are written in C.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "seds_generic_props.h"
#include "seds_tree_node.h"
#include "seds_plugin.h"
#include "seds_user_message.h"

/*
 * Many of the SEDS tree methods are implemented in Lua,
 * which are defined in the external module file.
 * This module is loaded when the tree functions are registered.
 */
static const char SEDS_TREE_METHODS_SCRIPT_FILE[] = "seds_tree_methods.lua";
static const char SEDS_TREE_PROPERTIES_LUA_KEY = '\0';

/**
 * Local lookup table to get a printable name associated with a node type
 */
static const char *SEDS_NODETYPE_LOOKUP[SEDS_NODETYPE_MAX] =
{
        [SEDS_NODETYPE_UNKNOWN] = "UNKNOWN",
        [SEDS_NODETYPE_ROOT] = "ROOT",
        [SEDS_NODETYPE_CCSDS_STANDARD_FIRST] = "CCSDS_STANDARD_FIRST",
        [SEDS_NODETYPE_DATASHEET] = "DATASHEET",
        [SEDS_NODETYPE_PACKAGEFILE] = "PACKAGEFILE",
        [SEDS_NODETYPE_PACKAGE] = "PACKAGE",
        [SEDS_NODETYPE_LONG_DESCRIPTION] = "LONG_DESCRIPTION",
        [SEDS_NODETYPE_SEMANTICS] = "SEMANTICS",
        [SEDS_NODETYPE_SET_NODE_FIRST] = "SET_NODE_FIRST",
        [SEDS_NODETYPE_DATA_TYPE_SET] = "DATA_TYPE_SET",
        [SEDS_NODETYPE_BASE_INTERFACE_SET] = "BASE_INTERFACE_SET",
        [SEDS_NODETYPE_GENERIC_TYPE_SET] = "GENERIC_TYPE_SET",
        [SEDS_NODETYPE_PARAMETER_SET] = "PARAMETER_SET",
        [SEDS_NODETYPE_COMMAND_SET] = "COMMAND_SET",
        [SEDS_NODETYPE_GENERIC_TYPE_MAP_SET] = "GENERIC_TYPE_MAP_SET",
        [SEDS_NODETYPE_CONSTRAINT_SET] = "CONSTRAINT_SET",
        [SEDS_NODETYPE_DECLARED_INTERFACE_SET] = "DECLARED_INTERFACE_SET",
        [SEDS_NODETYPE_PROVIDED_INTERFACE_SET] = "PROVIDED_INTERFACE_SET",
        [SEDS_NODETYPE_REQUIRED_INTERFACE_SET] = "REQUIRED_INTERFACE_SET",
        [SEDS_NODETYPE_COMPONENT_SET] = "COMPONENT_SET",
        [SEDS_NODETYPE_ALTERNATE_SET] = "ALTERNATE_SET",
        [SEDS_NODETYPE_PARAMETER_MAP_SET] = "PARAMETER_MAP_SET",
        [SEDS_NODETYPE_VARIABLE_SET] = "VARIABLE_SET",
        [SEDS_NODETYPE_ACTIVITY_SET] = "ACTIVITY_SET",
        [SEDS_NODETYPE_METADATA_VALUE_SET] = "METADATA_VALUE_SET",
        [SEDS_NODETYPE_NOMINAL_RANGE_SET] = "NOMINAL_RANGE_SET",
        [SEDS_NODETYPE_PARAMETER_ACTIVITY_MAP_SET] = "PARAMETER_ACTIVITY_MAP_SET",
        [SEDS_NODETYPE_SAFE_RANGE_SET] = "SAFE_RANGE_SET",
        [SEDS_NODETYPE_STATE_MACHINE_SET] = "STATE_MACHINE_SET",
        [SEDS_NODETYPE_SET_NODE_LAST] = "SET_NODE_LAST",
        [SEDS_NODETYPE_COMPONENT] = "COMPONENT",
        [SEDS_NODETYPE_COMMAND] = "COMMAND",
        [SEDS_NODETYPE_IMPLEMENTATION] = "IMPLEMENTATION",
        [SEDS_NODETYPE_ALTERNATE] = "ALTERNATE",
        [SEDS_NODETYPE_PARAMETER] = "PARAMETER",
        [SEDS_NODETYPE_ARGUMENT] = "ARGUMENT",
        [SEDS_NODETYPE_GENERIC_TYPE_MAP] = "GENERIC_TYPE_MAP",
        [SEDS_NODETYPE_PARAMETER_MAP] = "PARAMETER_MAP",
        [SEDS_NODETYPE_VARIABLE] = "VARIABLE",
        [SEDS_NODETYPE_CONTAINER_ENTRY_LIST] = "CONTAINER_ENTRY_LIST",
        [SEDS_NODETYPE_CONTAINER_TRAILER_ENTRY_LIST] = "CONTAINER_TRAILER_ENTRY_LIST",
        [SEDS_NODETYPE_DIMENSION_LIST] = "DIMENSION_LIST",
        [SEDS_NODETYPE_ENUMERATION_LIST] = "ENUMERATION_LIST",
        [SEDS_NODETYPE_SCALAR_DATATYPE_FIRST] = "SCALAR_DATATYPE_FIRST",
        [SEDS_NODETYPE_INTEGER_DATATYPE] = "INTEGER_DATATYPE",
        [SEDS_NODETYPE_FLOAT_DATATYPE] = "FLOAT_DATATYPE",
        [SEDS_NODETYPE_ENUMERATION_DATATYPE] = "ENUMERATION_DATATYPE",
        [SEDS_NODETYPE_BINARY_DATATYPE] = "BINARY_DATATYPE",
        [SEDS_NODETYPE_STRING_DATATYPE] = "STRING_DATATYPE",
        [SEDS_NODETYPE_BOOLEAN_DATATYPE] = "BOOLEAN_DATATYPE",
        [SEDS_NODETYPE_SUBRANGE_DATATYPE] = "SUBRANGE_DATATYPE",
        [SEDS_NODETYPE_SCALAR_DATATYPE_LAST] = "SCALAR_DATATYPE_LAST",
        [SEDS_NODETYPE_COMPOUND_DATATYPE_FIRST] = "COMPOUND_DATATYPE_FIRST",
        [SEDS_NODETYPE_ARRAY_DATATYPE] = "ARRAY_DATATYPE",
        [SEDS_NODETYPE_CONTAINER_DATATYPE] = "CONTAINER_DATATYPE",
        [SEDS_NODETYPE_COMPOUND_DATATYPE_LAST] = "COMPOUND_DATATYPE_LAST",
        [SEDS_NODETYPE_DYNAMIC_DATATYPE_FIRST] = "DYNAMIC_DATATYPE_FIRST",
        [SEDS_NODETYPE_GENERIC_TYPE] = "GENERIC_TYPE",
        [SEDS_NODETYPE_DYNAMIC_DATATYPE_LAST] = "DYNAMIC_DATATYPE_LAST",
        [SEDS_NODETYPE_INTERFACE_FIRST] = "INTERFACE_FIRST",
        [SEDS_NODETYPE_INTERFACE] = "INTERFACE",
        [SEDS_NODETYPE_DECLARED_INTERFACE] = "DECLARED_INTERFACE",
        [SEDS_NODETYPE_PROVIDED_INTERFACE] = "PROVIDED_INTERFACE",
        [SEDS_NODETYPE_REQUIRED_INTERFACE] = "REQUIRED_INTERFACE",
        [SEDS_NODETYPE_BASE_INTERFACE] = "BASE_INTERFACE",
        [SEDS_NODETYPE_INTERFACE_LAST] = "INTERFACE_LAST",
        [SEDS_NODETYPE_ENUMERATION_ENTRY] = "ENUMERATION_ENTRY",
        [SEDS_NODETYPE_CONTAINER_ENTRY] = "CONTAINER_ENTRY",
        [SEDS_NODETYPE_CONTAINER_FIXED_VALUE_ENTRY] = "CONTAINER_FIXED_VALUE_ENTRY",
        [SEDS_NODETYPE_CONTAINER_PADDING_ENTRY] = "CONTAINER_PADDING_ENTRY",
        [SEDS_NODETYPE_CONTAINER_LIST_ENTRY] = "CONTAINER_LIST_ENTRY",
        [SEDS_NODETYPE_CONTAINER_LENGTH_ENTRY] = "CONTAINER_LENGTH_ENTRY",
        [SEDS_NODETYPE_CONTAINER_ERROR_CONTROL_ENTRY] = "CONTAINER_ERROR_CONTROL_ENTRY",
        [SEDS_NODETYPE_CONSTRAINT_FIRST] = "CONSTRAINT_FIRST",
        [SEDS_NODETYPE_CONSTRAINT] = "CONSTRAINT",
        [SEDS_NODETYPE_TYPE_CONSTRAINT] = "TYPE_CONSTRAINT",
        [SEDS_NODETYPE_RANGE_CONSTRAINT] = "RANGE_CONSTRAINT",
        [SEDS_NODETYPE_VALUE_CONSTRAINT] = "VALUE_CONSTRAINT",
        [SEDS_NODETYPE_CONSTRAINT_LAST] = "CONSTRAINT_LAST",
        [SEDS_NODETYPE_ENCODING_FIRST] = "ENCODING_FIRST",
        [SEDS_NODETYPE_INTEGER_DATA_ENCODING] = "INTEGER_DATA_ENCODING",
        [SEDS_NODETYPE_FLOAT_DATA_ENCODING] = "FLOAT_DATA_ENCODING",
        [SEDS_NODETYPE_STRING_DATA_ENCODING] = "STRING_DATA_ENCODING",
        [SEDS_NODETYPE_ENCODING_LAST] = "ENCODING_LAST",
        [SEDS_NODETYPE_RANGE_FIRST] = "RANGE_FIRST",
        [SEDS_NODETYPE_MINMAX_RANGE] = "MINMAX_RANGE",
        [SEDS_NODETYPE_PRECISION_RANGE] = "PRECISION_RANGE",
        [SEDS_NODETYPE_ENUMERATED_RANGE] = "ENUMERATED_RANGE",
        [SEDS_NODETYPE_RANGE_LAST] = "RANGE_LAST",
        [SEDS_NODETYPE_RANGE] = "RANGE",
        [SEDS_NODETYPE_VALID_RANGE] = "VALID_RANGE",
        [SEDS_NODETYPE_DIMENSION] = "DIMENSION",
        [SEDS_NODETYPE_ARRAY_DIMENSIONS] = "ARRAY_DIMENSIONS",
        [SEDS_NODETYPE_SPLINE_CALIBRATOR] = "SPLINE_CALIBRATOR",
        [SEDS_NODETYPE_POLYNOMIAL_CALIBRATOR] = "POLYNOMIAL_CALIBRATOR",
        [SEDS_NODETYPE_SPLINE_POINT] = "SPLINE_POINT",
        [SEDS_NODETYPE_POLYNOMIAL_TERM] = "POLYNOMIAL_TERM",
        [SEDS_NODETYPE_ACTIVITY] = "ACTIVITY",
        [SEDS_NODETYPE_ANDED_CONDITIONS] = "ANDED_CONDITIONS",
        [SEDS_NODETYPE_ARGUMENT_VALUE] = "ARGUMENT_VALUE",
        [SEDS_NODETYPE_ARRAY_DATA_TYPE] = "ARRAY_DATA_TYPE",
        [SEDS_NODETYPE_ASSIGNMENT] = "ASSIGNMENT",
        [SEDS_NODETYPE_BODY] = "BODY",
        [SEDS_NODETYPE_BOOLEAN_DATA_ENCODING] = "BOOLEAN_DATA_ENCODING",
        [SEDS_NODETYPE_CALIBRATION] = "CALIBRATION",
        [SEDS_NODETYPE_CALL] = "CALL",
        [SEDS_NODETYPE_CATEGORY] = "CATEGORY",
        [SEDS_NODETYPE_COMPARISON_OPERATOR] = "COMPARISON_OPERATOR",
        [SEDS_NODETYPE_CONDITIONAL] = "CONDITIONAL",
        [SEDS_NODETYPE_CONDITION] = "CONDITION",
        [SEDS_NODETYPE_DATE_VALUE] = "DATE_VALUE",
        [SEDS_NODETYPE_DEVICE] = "DEVICE",
        [SEDS_NODETYPE_DO] = "DO",
        [SEDS_NODETYPE_END_AT] = "END_AT",
        [SEDS_NODETYPE_ENTRY_STATE] = "ENTRY_STATE",
        [SEDS_NODETYPE_EXIT_STATE] = "EXIT_STATE",
        [SEDS_NODETYPE_FIRST_OPERAND] = "FIRST_OPERAND",
        [SEDS_NODETYPE_FLOAT_VALUE] = "FLOAT_VALUE",
        [SEDS_NODETYPE_GET_ACTIVITY] = "GET_ACTIVITY",
        [SEDS_NODETYPE_GUARD] = "GUARD",
        [SEDS_NODETYPE_INTEGER_VALUE] = "INTEGER_VALUE",
        [SEDS_NODETYPE_ITERATION] = "ITERATION",
        [SEDS_NODETYPE_LABEL] = "LABEL",
        [SEDS_NODETYPE_MATH_OPERATION] = "MATH_OPERATION",
        [SEDS_NODETYPE_METADATA] = "METADATA",
        [SEDS_NODETYPE_ON_COMMAND_PRIMITIVE] = "ON_COMMAND_PRIMITIVE",
        [SEDS_NODETYPE_ON_CONDITION_FALSE] = "ON_CONDITION_FALSE",
        [SEDS_NODETYPE_ON_CONDITION_TRUE] = "ON_CONDITION_TRUE",
        [SEDS_NODETYPE_ON_ENTRY] = "ON_ENTRY",
        [SEDS_NODETYPE_ON_EXIT] = "ON_EXIT",
        [SEDS_NODETYPE_ON_PARAMETER_PRIMITIVE] = "ON_PARAMETER_PRIMITIVE",
        [SEDS_NODETYPE_ON_TIMER] = "ON_TIMER",
        [SEDS_NODETYPE_OPERATOR] = "OPERATOR",
        [SEDS_NODETYPE_ORED_CONDITIONS] = "ORED_CONDITIONS",
        [SEDS_NODETYPE_OVER_ARRAY] = "OVER_ARRAY",
        [SEDS_NODETYPE_PARAMETER_ACTIVITY_MAP] = "PARAMETER_ACTIVITY_MAP",
        [SEDS_NODETYPE_PROVIDED] = "PROVIDED",
        [SEDS_NODETYPE_REQUIRED] = "REQUIRED",
        [SEDS_NODETYPE_SECOND_OPERAND] = "SECOND_OPERAND",
        [SEDS_NODETYPE_SEMANTICS_TERM] = "SEMANTICS_TERM",
        [SEDS_NODETYPE_SEND_COMMAND_PRIMITIVE] = "SEND_COMMAND_PRIMITIVE",
        [SEDS_NODETYPE_SEND_PARAMETER_PRIMITIVE] = "SEND_PARAMETER_PRIMITIVE",
        [SEDS_NODETYPE_SET_ACTIVITY_ONLY] = "SET_ACTIVITY_ONLY",
        [SEDS_NODETYPE_SET_ACTIVITY] = "SET_ACTIVITY",
        [SEDS_NODETYPE_START_AT] = "START_AT",
        [SEDS_NODETYPE_STATE_MACHINE] = "STATE_MACHINE",
        [SEDS_NODETYPE_STATE] = "STATE",
        [SEDS_NODETYPE_STEP] = "STEP",
        [SEDS_NODETYPE_STRING_VALUE] = "STRING_VALUE",
        [SEDS_NODETYPE_TRANSITION] = "TRANSITION",
        [SEDS_NODETYPE_TYPE_CONDITION] = "TYPE_CONDITION",
        [SEDS_NODETYPE_TYPE_OPERAND] = "TYPE_OPERAND",
        [SEDS_NODETYPE_VALUE] = "VALUE",
        [SEDS_NODETYPE_VARIABLE_REF] = "VARIABLE_REF",
        [SEDS_NODETYPE_CCSDS_STANDARD_LAST] = "CCSDS_STANDARD_LAST",
        [SEDS_NODETYPE_XINCLUDE_PASSTHRU] = "XINCLUDE_PASSTHRU",
        [SEDS_NODETYPE_DESCRIPTION_PASSTHRU] = "DESCRIPTION_PASSTHRU",
        [SEDS_NODETYPE_DESIGN_PARAMETERS] = "DESIGN_PARAMETERS",
        [SEDS_NODETYPE_DEFINE] = "DEFINE",
        [SEDS_NODETYPE_INSTANCE_RULE_SET] = "INSTANCE_RULE_SET",
        [SEDS_NODETYPE_INSTANCE_RULE] = "INSTANCE_RULE",
        [SEDS_NODETYPE_INTERFACE_MAP_SET] = "INTERFACE_MAP_SET",
        [SEDS_NODETYPE_INTERFACE_MAP] = "INTERFACE_MAP",
        [SEDS_NODETYPE_PARAMETER_VALUE] = "PARAMETER_VALUE",
};


/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/


/**
 * Lua callable function to output a message regarding a SEDS DOM node object
 *
 * Contextual information will be added to the message.
 *
 * Expected Lua input stack:
 *   upvalue 1: The type of message to generate (error or warning, typically)
 *   1: seds_node (DOM) object which the message is based on
 *   2: message text (string)
 *
 * In the case of an error message, this in turn throws the exception out to
 * the Lua engine, which can be caught by the calling code if running in
 * protected mode.
 */
static int seds_tree_mark_error(lua_State *lua)
{
    const char *extramsg;
    seds_user_message_t msgtype = lua_tointeger(lua, lua_upvalueindex(1));

    luaL_checkudata(lua, 1, "seds_node");
    luaL_argcheck(lua, lua_isstring(lua, 2), 2, "string argument required");

    /* 3rd parameter is optional */
    if (!lua_isnoneornil(lua, 3))
    {
        extramsg = luaL_tolstring(lua, 3, NULL);
    }
    else
    {
        extramsg = NULL;
    }

    lua_getfield(lua, 1, "xml_filename");
    lua_getfield(lua, 1, "xml_linenum");

    seds_user_message_preformat(msgtype,
            lua_tostring(lua, -2),
            lua_tointeger(lua, -1),
            lua_tostring(lua, 2), extramsg);

    if (msgtype == SEDS_USER_MESSAGE_ERROR)
    {
        lua_pushstring(lua, "Processing Error");
        lua_error(lua);
    }

    return 0;
}

/**
 * Lua callable function to get the property of a DOM node
 *
 * This is a wrapper around the generic seds_get_userdata_property
 * for tree node (DOM) objects.  This first checks a static table
 * table for methods that should be present on all DOM nodes.
 *
 * If the requested key is not present in the static table, then
 * check the user properties in the userdata object.
 *
 * Expected Stack args:
 *  1: tree node object
 *  2: property key
 *
 * @note The static properties are all read only so there is no
 * correspnding "set" operation; all sets go directly into the
 * userdata object.
 */
static int seds_tree_node_get_property(lua_State *lua)
{
    seds_node_t *pnode = luaL_checkudata(lua, 1, "seds_node");

    /* Exactly two arguments should have been passed in */
    lua_settop(lua, 2);

    /*
     * First attempt to look up the value in the static tree method table
     * If this returns non-nil then use that value for the property
     * (This handles all tree methods/functions)
     */
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &SEDS_TREE_PROPERTIES_LUA_KEY);
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

    lua_settop(lua, 2);

    if (lua_type(lua, 2) == LUA_TSTRING &&
            strcmp(lua_tostring(lua, 2), "entity_type") == 0)
    {
        if (pnode->node_type < SEDS_NODETYPE_MAX &&
                SEDS_NODETYPE_LOOKUP[pnode->node_type] != NULL)
        {
            lua_pushstring(lua, SEDS_NODETYPE_LOOKUP[pnode->node_type]);
        }
        else
        {
            lua_pushnil(lua);
        }

        return 1;
    }

    /*
     * Otherwise try retrieving the key from the uservalue table
     */
    return seds_generic_props_get_property(lua);
}

/**
 * Lua callable helper function to convert a tree node object into a string
 *
 * This is helpful for debugging, so informational messages can include
 * the critical bits of information about a tree node.  It is useful as
 * a "tostring" metamethod for tree nodes.
 *
 * Expected Lua input stack:
 *   1: Tree node object
 *
 * Returns a Lua string representation of the object.
 *
 */
static int seds_tree_node_to_string(lua_State *lua)
{
    seds_node_t *pnode = lua_touserdata(lua, 1);
    luaL_Buffer buf;

    luaL_buffinit(lua, &buf);

    lua_getuservalue(lua, 1);           /* stack pos = 2 */
    if (pnode->node_type < SEDS_NODETYPE_MAX)
    {
        luaL_addstring(&buf, SEDS_NODETYPE_LOOKUP[pnode->node_type]);
    }
    else
    {
        luaL_addstring(&buf, SEDS_NODETYPE_LOOKUP[SEDS_NODETYPE_UNKNOWN]);
    }
    lua_getfield(lua, 2, "name");
    if (lua_isstring(lua, -1))
    {
        luaL_addstring(&buf, " name=");
        luaL_addstring(&buf, lua_tostring(lua, -1));
    }
    lua_settop(lua, 1);
    luaL_pushresult(&buf);
    return 1;
}

/**
 * Lua callable function to instantiate a new a tree node
 *
 * Expected Lua input stack:
 *   1: Tree node type (as string)
 *
 * Returns a new node of the correct type, or nil if the type
 * was not valid
 */
static int seds_tree_node_new_object(lua_State *lua)
{
    int i;
    int nret;
    const char *nodetype_str = luaL_checkstring(lua, 1);

    nret = 0;
    if (nodetype_str != NULL)
    {
        for (i=0; i < (sizeof(SEDS_NODETYPE_LOOKUP) / sizeof(SEDS_NODETYPE_LOOKUP[0])); ++i)
        {
            if (SEDS_NODETYPE_LOOKUP[i] != NULL &&
                    strcmp(SEDS_NODETYPE_LOOKUP[i], nodetype_str) == 0)
            {
                seds_tree_node_push_new_object(lua, i);
                nret = 1;
            }
        }
    }

    return nret;
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
void seds_tree_node_push_new_object(lua_State *lua, seds_nodetype_t node_type)
{
    seds_node_t *pnode = lua_newuserdata(lua, sizeof(seds_node_t));
    memset(pnode, 0, sizeof(*pnode));
    pnode->node_type = node_type;
    if (luaL_newmetatable(lua, "seds_node"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_tree_node_get_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, seds_generic_props_set_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, seds_tree_node_to_string);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    lua_newtable(lua);
    lua_setuservalue(lua, -2);
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
seds_nodetype_t seds_tree_node_get_type(lua_State *lua, int pos)
{
    seds_node_t *pnode;

    pnode = luaL_checkudata(lua, pos, "seds_node");
    luaL_argcheck(lua, pnode != NULL, pos, "seds_node expected");

    return pnode->node_type;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_tree_node_register_globals(lua_State *lua)
{
    luaL_checktype(lua, -1, LUA_TTABLE);

    lua_pushcfunction(lua, seds_tree_node_new_object);
    lua_setfield(lua, -2, "new_tree_object");

    /*
     * Load the basic SEDS tree methods which are implemented in Lua
     */
    seds_plugin_load_lua(lua, SEDS_TREE_METHODS_SCRIPT_FILE);
    lua_call(lua, 0, 1);
    luaL_checktype(lua, -1, LUA_TTABLE);

    /* additional tree methods that are implemented in C,
     * supplements the set which is implemented in Lua */
    lua_pushinteger(lua, SEDS_USER_MESSAGE_ERROR);
    lua_pushcclosure(lua, seds_tree_mark_error, 1);
    lua_setfield(lua, -2, "error");
    lua_pushinteger(lua, SEDS_USER_MESSAGE_WARNING);
    lua_pushcclosure(lua, seds_tree_mark_error, 1);
    lua_setfield(lua, -2, "warning");
    lua_pushcfunction(lua, seds_generic_props_enumerate_properties);
    lua_setfield(lua, -2, "get_properties");

    lua_rawsetp(lua, LUA_REGISTRYINDEX, &SEDS_TREE_PROPERTIES_LUA_KEY);

}

