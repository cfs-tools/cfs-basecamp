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
 * \file     seds_generic_props.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Header file for the properties module.
 * See full module description on seds_generic_props.c
 */

#ifndef _SEDS_GENERIC_PROPS_H_
#define _SEDS_GENERIC_PROPS_H_


#include "seds_global.h"

/*******************************************************************************/
/*                  Function documentation and prototypes                      */
/*      (everything referenced outside this unit should be described here)     */
/*******************************************************************************/


/**
 * Lua callable function to get a property of a userdata object
 *
 * SEDS DOM nodes may contain any number of arbitrary properties.  These
 * are typically set by various plug-in scripts as the tool executes, and
 * they follow the Lua table paradigm being key/value pairs.
 *
 * This function provides the basic facility to read a key and get the value.
 *
 * Expected Lua input stack:
 *    1: userdata object
 *    2: property key
 *
 * returns value associated with the key (which may in be a Lua table containing
 * more key/value pairs).
 *
 * @sa seds_set_userdata_property
 */
int seds_generic_props_get_property(lua_State *lua);

/**
 * Lua callable function to set a property of a userdata object
 *
 * SEDS DOM nodes may contain any number of arbitrary properties.  These
 * are typically set by various plug-in scripts as the tool executes, and
 * they follow the Lua table paradigm being key/value pairs.
 *
 * This function provides the basic facility to write a key and set the value.
 *
 * Expected Lua input stack:
 *    1: userdata object
 *    2: property key
 *    3: property value
 *
 * @sa seds_get_userdata_property
 */
int seds_generic_props_set_property(lua_State *lua);

/**
 * Debugging helper function to enumerate the added keys in a DOM node
 *
 * Lua callable function, returns an array containing the keys
 * in the "uservalue" table of the userdata object
 */
int seds_generic_props_enumerate_properties(lua_State *lua);


/**
 * Register any globals associated with this module in the Lua state
 */
void seds_generic_props_register_globals(lua_State *lua);


#endif  /* _SEDS_GENERIC_PROPS_H_ */

