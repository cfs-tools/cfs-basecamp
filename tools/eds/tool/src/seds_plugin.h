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
 * \file     seds_plugin.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Plug-in loader module declarations.
 * For full module description, see seds_plugin.c
 */

#ifndef _SEDS_PLUGIN_H_
#define _SEDS_PLUGIN_H_

#include "seds_global.h"


/*******************************************************************************/
/*                  Function documentation and prototypes                      */
/*      (everything referenced outside this unit should be described here)     */
/*******************************************************************************/


/**
 * Load a Lua runtime script plugin from the C source directory.
 *
 * A wrapper around the underlying luaL_loadfile() implementation that uses
 * the path of the C source code as the source for the specified Lua file.
 *
 * A considerable amount of tool functionality is actually implemented in
 * Lua.  Only the base engine is implemented in C.  The Lua source files for the
 * main tool operation are loaded at runtime via this function.
 *
 * @note The binary executable and all Lua source files must be moved together; the
 * tool will not execute if the corresponding Lua source is not present.
 *
 * If the source files are moved from their original location at the time of compilation,
 * the "-s" command option may be used to indicate the correct Lua source location.
 *
 * @param lua Lua state object
 * @param runtime_file file to load from C source directory
 */
void seds_plugin_load_lua(lua_State *lua, const char *runtime_file);

/**
 * Load a dynamic shared object (.so) plugin from the binary build directory.
 *
 * A wrapper around the underlying luaL_loadfile() implementation that uses
 * the path of the C source code as the source for the specified Lua file.
 *
 * @param lua Lua state object
 * @param runtime_file file to load from C source directory
 */
void seds_plugin_load_so(lua_State *lua, const char *filename);

/**
 * Register functions related to Plugin loading with the Lua interpreter
 */
void seds_plugin_register_globals(lua_State *lua);



#endif  /* _SEDS_PLUGIN_H_ */

