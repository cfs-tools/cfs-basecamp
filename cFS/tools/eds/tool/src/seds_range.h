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
 * \file     seds_range.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * For full module description, see seds_range.c
 */

#ifndef SEDS_RANGE_H
#define SEDS_RANGE_H

#include "seds_global.h"

/* Note with respect to the values:
 * Not relying on IEEE-754 special values like NaN and infinity here, because
 * it is not guaranteed/required that the platform supports these, and things
 * get unclear with respect to comparisons when they are possibly involved.
 *
 * The value in this structure should always be a normal number, but the
 * number is ignored if "is_defined" flag is false.
 */
typedef struct
{
    bool          is_defined;   /**< If false then anything matches (infinite) */
    bool          is_inclusive; /**< Whether value itself it is included in the range */
    seds_number_t valx;         /**< Value */
} seds_rangelimit_t;

/**
 * Structure that tracks the range of EDS-defined objects
 *
 * Instances of these objects (in the form of Lua userdata objects) may be
 * attached to any EDS DOM node which has a range associated with it.
 */
typedef struct
{
    seds_rangelimit_t min;
    seds_rangelimit_t max;
} seds_rangesegment_t;

typedef enum
{
    SEDS_RANGECONTENT_EMPTY,
    SEDS_RANGECONTENT_NORMAL,
    SEDS_RANGECONTENT_INFINITE
} seds_rangecontent_t;

/**
 * Structure that has no real members of its own
 *
 * The real data is a Lua value stored as the "uservalue"
 * within this userdata object, along with a metatable for
 * standard access functions which can be implemented in C.
 *
 * Instances of these objects (in the form of Lua userdata objects) may be
 * attached to any EDS DOM node which has a range associated with it.
 *
 * seds_range:
 * This combines one or more segments to allow for a disjointed range.
 *
 * seds_rangemap:
 * Basically a Lua table that may use seds_range objects as keys.  The
 * lookup function knows how to compare ranges.
 */
typedef struct
{
    seds_rangecontent_t content;
} seds_range_ud_t;

/*******************************************************************************/
/*                  Function documentation and prototypes                      */
/*      (everything referenced outside this unit should be described here)     */
/*******************************************************************************/

/**
 * Registers the range functions into the SEDS library object
 * These are added to a table which is at the top of the stack
 */
void seds_range_register_globals(lua_State *lua);

#endif /* SEDS_RANGE_H */
