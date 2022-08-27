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
 * \file     seds_preprocess.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Preprocessor module prototypes.
 * For full module description, see seds_preprocess.c
 */

#ifndef _SEDS_PREPROCESS_H_
#define _SEDS_PREPROCESS_H_


/**
 * Register the preprocessor functions in the global Lua state
 */
void seds_preprocess_register_globals(lua_State *lua);


#endif  /* _SEDS_PREPROCESS_H_ */

