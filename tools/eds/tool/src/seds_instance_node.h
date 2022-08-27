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
 * \file     seds_instance_node.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implements methods for "instance nodes" within the seds DOM
 * See full description in seds_instance_node.c
 */

#ifndef _SEDS_INSTANCE_NODE_H_
#define _SEDS_INSTANCE_NODE_H_

#include "seds_global.h"

/*******************************************************************************/
/*                  Function documentation and prototypes                      */
/*      (everything referenced outside this unit should be described here)     */
/*******************************************************************************/


/**
 * Register the tree functions which are called from Lua
 * These are added to a table which is at the top of the stack
 */
void seds_instance_node_register_globals(lua_State *lua);


#endif  /* _SEDS_INSTANCE_NODE_H_ */

