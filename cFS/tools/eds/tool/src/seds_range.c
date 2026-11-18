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
 * \file     seds_range.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of the "range" construct from EDS,
 * allowing it to be used as if it was a value.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <strings.h>

#include "seds_range.h"

typedef enum
{
    seds_range_check_reject     = 0,
    seds_range_check_accept     = 1,
    seds_range_check_is_less    = 2,
    seds_range_check_is_greater = 3,
} seds_range_check_t;

/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/

static seds_rangesegment_t *seds_rangesegment_push(lua_State *lua, seds_rangesegment_t *pinit);
static void                 seds_range_push(lua_State *lua, seds_rangecontent_t initial_content);

/**
 * Helper function: call routine for every segment within range
 * Range must be in stack position indicated by range_idx
 * The routine to call should be at the top of the stack (-1)
 *
 * NOTE: This does NOT pop the routine, it leaves it on the stack (different from lua_call here)
 */
static int seds_range_foreach_segment_impl(lua_State *lua, int range_idx);

/**
 * Helper function: initalize a limit (min or max) from a lua value
 * at the given index, which should be absolute
 *
 * If the index is invalid or none, this will initialize the limit
 * as infinite
 *
 * table data should have the form:
 *   { value = X, inclusive = Y }
 */
int seds_rangesegment_set_limit_from_idx(lua_State *lua, int lim_idx, seds_rangelimit_t *rlim)
{
    if (lua_istable(lua, lim_idx))
    {
        lua_pushstring(lua, "value");
        lua_gettable(lua, lim_idx);
    }
    else if (lua_isnumber(lua, lim_idx))
    {
        lua_pushvalue(lua, lim_idx);
    }
    else
    {
        /* initialize to no limit */
        lua_pushnil(lua);
    }

    rlim->is_defined = !lua_isnil(lua, -1);
    if (rlim->is_defined)
    {
        rlim->valx = lua_tonumber(lua, -1);
    }
    lua_pop(lua, 1);

    /* determine if it is inclusive or not */
    /* assume it is inclusive unless specifically indicated as false */
    if (lua_istable(lua, lim_idx))
    {
        lua_pushstring(lua, "inclusive");
        lua_gettable(lua, lim_idx);
    }
    else
    {
        /* initialize to default */
        lua_pushnil(lua);
    }

    /* nil here means it was not specified, so interpret it as true */
    rlim->is_inclusive = lua_isnil(lua, -1) || lua_toboolean(lua, -1);
    lua_pop(lua, 1);

    return 0;
}

/**
 * Helper function: Push a table onto the stack that represents
 * the limit in table form
 *
 * table data will have the form:
 *   { value = X, inclusive = Y }
 */
int seds_rangesegment_push_limit_value(lua_State *lua, seds_rangelimit_t *rlim)
{
    double whole;

    if (!rlim->is_defined)
    {
        lua_pushnil(lua);
    }
    else if (modf(rlim->valx, &whole) == 0.0)
    {
        /* The limit is a whole number, so push it as an integer */
        lua_pushinteger(lua, (lua_Integer)whole);
    }
    else
    {
        lua_pushnumber(lua, rlim->valx);
    }

    return 1;
}

/**
 * Helper function: Push a table onto the stack that represents
 * the limit in table form
 *
 * table data will have the form:
 *   { value = X, inclusive = Y }
 */
int seds_rangesegment_get_limit_as_table(lua_State *lua, seds_rangelimit_t *rlim)
{
    /* check if it is a normal limit */
    if (rlim->is_defined)
    {
        lua_newtable(lua);

        lua_pushstring(lua, "value");
        seds_rangesegment_push_limit_value(lua, rlim);
        lua_settable(lua, -3);

        lua_pushstring(lua, "inclusive");
        lua_pushboolean(lua, rlim->is_inclusive);
        lua_settable(lua, -3);
    }
    else
    {
        /* it has no limit */
        lua_pushnil(lua);
    }

    return 1;
}

/**
 * Helper function: gets the range type as a string
 */
int seds_rangesegment_get_type_as_string(lua_State *lua, seds_rangesegment_t *pseg)
{
    const char *inclusion;

    inclusion = NULL;

    if (pseg->max.is_defined && pseg->min.is_defined)
    {
        if (pseg->min.is_inclusive)
        {
            if (pseg->max.is_inclusive)
            {
                inclusion = "inclusiveMinInclusiveMax";
            }
            else
            {
                inclusion = "inclusiveMinExclusiveMax";
            }
        }
        else
        {
            if (pseg->max.is_inclusive)
            {
                inclusion = "exclusiveMinInclusiveMax";
            }
            else
            {
                inclusion = "exclusiveMinExclusiveMax";
            }
        }
    }
    else if (pseg->max.is_defined)
    {
        if (pseg->max.is_inclusive)
        {
            inclusion = "atMost";
        }
        else
        {
            inclusion = "lessThan";
        }
    }
    else if (pseg->min.is_defined)
    {
        if (pseg->min.is_inclusive)
        {
            inclusion = "atLeast";
        }
        else
        {
            inclusion = "greaterThan";
        }
    }
    else
    {
        inclusion = "infinite";
    }

    lua_pushstring(lua, inclusion);
    return 1;
}

/**
 * Helper function: tests if lim1 may include lim2, considering exclusivity
 */
static seds_range_check_t seds_rangesegment_compare(const seds_rangelimit_t *plim1, const seds_rangelimit_t *plim2)
{
    seds_range_check_t retval;

    if (!plim1->is_defined)
    {
        /* if lim1 is not defined then it will match anything EXCEPT
         * if lim2 is also not defined. */
        if (plim2->is_defined || plim1->is_inclusive)
        {
            retval = seds_range_check_accept;
        }
        else
        {
            retval = seds_range_check_reject;
        }
    }
    else if (!plim2->is_defined)
    {
        /* lim1 is defined so lim2 must also be defined */
        retval = seds_range_check_reject;
    }
    else if (plim1->valx < plim2->valx)
    {
        /* both are defined and lim1 < lim2 */
        retval = seds_range_check_is_less;
    }
    else if (plim1->valx > plim2->valx)
    {
        /* both are defined and lim1 > lim2 */
        retval = seds_range_check_is_greater;
    }
    else if (!plim2->is_inclusive || plim1->is_inclusive)
    {
        /* values are equal: if lim2 is exclusive, then it
         * matches regardless of lim1 */
        retval = seds_range_check_accept;
    }
    else
    {
        retval = seds_range_check_reject;
    }

    return retval;
}

/**
 * Helper function: tests if lim1 >= lim2, considering exclusivity
 */
static bool seds_rangesegment_compare_max(const seds_rangelimit_t *plim1, const seds_rangelimit_t *plim2)
{
    seds_range_check_t cmp = seds_rangesegment_compare(plim1, plim2);

    return (cmp == seds_range_check_accept || cmp == seds_range_check_is_greater);
}

/**
 * Helper function: tests if lim1 <= lim2, considering exclusivity
 */
static bool seds_rangesegment_compare_min(const seds_rangelimit_t *plim1, const seds_rangelimit_t *plim2)
{
    seds_range_check_t cmp = seds_rangesegment_compare(plim1, plim2);

    return (cmp == seds_range_check_accept || cmp == seds_range_check_is_less);
}

/**
 * Helper function: tests if segment has ANY values in it
 */
static bool seds_rangesegment_is_empty(const seds_rangesegment_t *pseg)
{
    /* if either side is undefined then the segment is not empty (it goes to infinity) */
    if (!pseg->max.is_defined || !pseg->min.is_defined)
    {
        return false;
    }

    /* if min <= max then it is not empty */
    return !seds_rangesegment_compare_min(&pseg->min, &pseg->max);
}

/**
 * Helper function: check if the rangesegment is a singular value
 */
static bool seds_rangesegment_is_single_value(const seds_rangesegment_t *pseg)
{
    /* if either side is undefined then the segment is not a single value (it goes to infinity) */
    /* similarly if either side is not inclusive, it cannot be a single value either */
    if (!pseg->max.is_defined || !pseg->min.is_defined || !pseg->max.is_inclusive || !pseg->min.is_inclusive)
    {
        return false;
    }

    /* it is a single value if min and max are the same */
    return (pseg->max.valx == pseg->min.valx);
}

/**
 * Helper function: push a single value onto the stack corresponding to the segment
 *
 * If the segment is a single value, this pushes that value in simple/primitive form.
 * For example a single number is pushed as a number.  If the value is not actually
 * a single, then this depends on the "force" option.
 *
 * If force is false - then push nil
 * If force is true - then push a number representing the midpoint of the segment
 */
static void seds_rangesegment_push_as_single_value(lua_State *lua, int arg_idx, bool force)
{
    const seds_rangesegment_t *pseg = luaL_testudata(lua, arg_idx, "seds_rangesegment");

    if (pseg == NULL)
    {
        /* if its not a rangesegment, then the value is the arg itself */
        lua_pushvalue(lua, arg_idx);
    }
    else if (seds_rangesegment_is_single_value(pseg))
    {
        /* if it is a single number, then push that number (min or max is the same) */
        lua_pushnumber(lua, pseg->max.valx);
    }
    else if (force)
    {
        /* push the midpoint as a number, this is reasonable for sorting purposes */
        lua_pushnumber(lua, (pseg->min.valx + pseg->max.valx) / 2);
    }
    else
    {
        /* not able to push as single value, push as nil */
        lua_pushnil(lua);
    }
}

/**
 * Helper function: Sets the rangelimit to given value
 */
static void seds_rangelimit_set_value(seds_rangelimit_t *plim, seds_number_t value, bool inclusive)
{
    plim->is_defined   = true;
    plim->is_inclusive = inclusive;
    plim->valx         = value;
}

/**
 * Helper function: Sets the segment to a single value
 * This is a range that only matches one specific value
 */
static void seds_rangesegment_set_single_value(seds_rangesegment_t *pseg, seds_number_t value)
{
    seds_rangelimit_set_value(&pseg->min, value, true);
    pseg->max = pseg->min;
}

/**
 * Helper function: Sets the segment to an empty set
 * This is a range that contains no valid values
 */
static void seds_rangesegment_set_empty(seds_rangesegment_t *pseg)
{
    seds_rangelimit_set_value(&pseg->min, 0, false);
    pseg->max = pseg->min;
}

/**
 * Helper function: tests if seg2 is totally within seg1
 */
static bool seds_rangesegment_includes_rangesegment(seds_rangesegment_t *pseg1, seds_rangesegment_t *pseg2)
{
    return seds_rangesegment_compare_min(&pseg1->min, &pseg2->min)
           && seds_rangesegment_compare_max(&pseg1->max, &pseg2->max);
}

/**
 * Helper function to check if an argument permits range segment processing
 * This applies to anything numeric
 */
static bool seds_rangesegment_acceptable_value(lua_State *lua, int arg_idx)
{
    return (lua_isnumber(lua, arg_idx) || luaL_testudata(lua, arg_idx, "seds_rangesegment"));
}

/**
 * Helper function: get argument as a segment
 *
 * If the given lua stack argument is a rangesegment, then return a pointer to it directly
 * If it is a single value, then initialize the temp to refer to that single value and return
 *
 * Either way the result is a rangesegment that can be passed to other routines.
 */
static seds_rangesegment_t *seds_rangesegment_get_safe(lua_State *lua, int arg_pos, seds_rangesegment_t *tempbuf)
{
    seds_rangesegment_t *pseg = luaL_testudata(lua, arg_pos, "seds_rangesegment");

    if (pseg == NULL)
    {
        if (lua_isnumber(lua, arg_pos))
        {
            seds_rangesegment_set_single_value(tempbuf, lua_tonumber(lua, arg_pos));
        }
        else if (lua_isnoneornil(lua, arg_pos))
        {
            /* this handles e.g. nil, consider it an empty set */
            seds_rangesegment_set_empty(tempbuf);
        }
        else
        {
            luaL_error(lua, "Cannot get numeric rangesegment from type=%s", luaL_typename(lua, arg_pos));
        }

        pseg = tempbuf;
    }

    return pseg;
}

/**
 * Helper function to check if a rangesegment matches a given value
 *
 * Expected Args:
 *  1: Object to check
 *
 * Upvalues:
 *  1: pointer to boolean with result
 *  2: Reference Segment to check against
 *
 * If the argument (1) is NOT covered by the reference segment, sets the
 * boolean to false.
 */
static int seds_rangesegment_match_helper(lua_State *lua)
{
    seds_rangesegment_t *pseg = luaL_checkudata(lua, lua_upvalueindex(2), "seds_rangesegment");
    seds_rangesegment_t *pcheck;
    seds_rangesegment_t  temp;
    bool                *presult = lua_touserdata(lua, lua_upvalueindex(1));

    /* this should only set result to false, never set it to true */
    if (seds_rangesegment_acceptable_value(lua, 1))
    {
        pcheck = seds_rangesegment_get_safe(lua, 1, &temp);
        if (!seds_rangesegment_includes_rangesegment(pseg, pcheck))
        {
            *presult = false;
        }
    }
    else
    {
        *presult = false;
    }

    return 0;
}

/**
 * Lua callable function to check if the argument falls within the range
 *
 * Expected Stack args:
 *  1: rangesegment object
 *  2: value to check (number) or another rangesegment
 */
static int seds_rangesegment_matches(lua_State *lua)
{
    bool result;

    result = true;

    lua_pushlightuserdata(lua, &result);
    lua_pushvalue(lua, 1);
    lua_pushcclosure(lua, seds_rangesegment_match_helper, 2);

    if (luaL_testudata(lua, 2, "seds_range"))
    {
        /* call the helper with each segment */
        seds_range_foreach_segment_impl(lua, 2);
        lua_pop(lua, 1);
    }
    else
    {
        /* just call the helper directly with the arg */
        lua_pushvalue(lua, 2);
        lua_call(lua, 1, 0);
    }

    lua_pushboolean(lua, result);
    return 1;
}

/**
 * Helper function to compute a intersection of 2 segments
 *
 * NOTE: this will produce an invalid result if the segments
 * do not overlap at all (this can be checked for via is_empty)
 */
static void seds_rangesegment_compute_intersection(seds_rangesegment_t       *pcombined,
                                                   const seds_rangesegment_t *pseg1,
                                                   const seds_rangesegment_t *pseg2)
{
    memset(pcombined, 0, sizeof(*pcombined));

    /* get the greater of the two minimums */
    if (seds_rangesegment_compare_min(&pseg1->min, &pseg2->min))
    {
        /* this means r1 min <= r2 min */
        pcombined->min = pseg2->min;
    }
    else
    {
        pcombined->min = pseg1->min;
    }

    /* get the lesser of the two maximums */
    if (seds_rangesegment_compare_max(&pseg1->max, &pseg2->max))
    {
        /* this means r1 max >= r2 max */
        pcombined->max = pseg2->max;
    }
    else
    {
        pcombined->max = pseg1->max;
    }
}

/**
 * Helper function to bridge 2 segments together
 *
 * NOTE: this covers any gap between them, the caller should confirm the segments
 * are overlapping in some way before bridging if the intent is to create a union
 */
static void seds_rangesegment_compute_bridge(seds_rangesegment_t       *pcombined,
                                             const seds_rangesegment_t *pseg1,
                                             const seds_rangesegment_t *pseg2)
{
    /* get the lesser of the two minimums */
    if (seds_rangesegment_compare_min(&pseg1->min, &pseg2->min))
    {
        /* this means r1 min <= r2 min */
        pcombined->min = pseg1->min;
    }
    else
    {
        pcombined->min = pseg2->min;
    }

    /* get the greater of the two maximums */
    if (seds_rangesegment_compare_max(&pseg1->max, &pseg2->max))
    {
        /* this means r1 max >= r2 max */
        pcombined->max = pseg1->max;
    }
    else
    {
        pcombined->max = pseg2->max;
    }
}

/**
 * Lua-callable function to create a new range userdata object
 *
 * Expected input Lua stack, direct initializer:
 *      1: value
 */
int seds_rangesegment_new_single_value(lua_State *lua)
{
    seds_rangesegment_t *pseg;

    pseg = seds_rangesegment_push(lua, NULL);

    if (lua_isnumber(lua, 1))
    {
        seds_rangesegment_set_single_value(pseg, lua_tonumber(lua, 1));
    }
    else
    {
        /* consider it an empty set */
        seds_rangesegment_set_empty(pseg);
    }

    return 1;
}

/**
 * Lua callable function to calculate the intersection of 2 range segments
 *
 * Expected Stack args:
 *  1: object 1, may be a rangesegment OR a single value
 *  2: object 2, may be another rangesegment OR a single value
 */
static int seds_rangesegment_intersection(lua_State *lua)
{
    seds_rangesegment_t *pseg1;
    seds_rangesegment_t *pseg2;
    seds_rangesegment_t  temp1;
    seds_rangesegment_t  temp2;
    seds_rangesegment_t  combined;

    pseg1 = seds_rangesegment_get_safe(lua, 1, &temp1);
    pseg2 = seds_rangesegment_get_safe(lua, 2, &temp2);

    seds_rangesegment_compute_intersection(&combined, pseg1, pseg2);

    if (seds_rangesegment_is_empty(&combined))
    {
        /* the result is empty, return nothing */
        return 0;
    }

    if (seds_rangesegment_is_single_value(&combined))
    {
        /* the result is a single number, return it */
        lua_pushnumber(lua, combined.min.valx);
    }
    else
    {
        /* the result is a valid segment, return it */
        seds_rangesegment_push(lua, &combined);
    }

    return 1;
}

/**
 * Helper function: tests if there is a gap between two limit values
 */
static bool seds_rangelimit_is_contiguous(seds_rangelimit_t *plim1, seds_rangelimit_t *plim2)
{
    bool result;

    if (plim1->is_defined && plim2->is_defined && plim1->valx == plim2->valx)
    {
        /* as long as one or othe other is inclusive, it is contiguous */
        result = (plim1->is_inclusive || plim2->is_inclusive);
    }
    else
    {
        result = false;
    }

    return result;
}

/**
 * Lua callable function to check for continuity between two segments
 *
 * Expected Stack args:
 *  1: object 1, may be a rangesegment OR a single value
 *  2: object 2, may be another rangesegment OR a single value
 */
static int seds_rangesegment_contiguous_with(lua_State *lua)
{
    seds_rangesegment_t *pseg1;
    seds_rangesegment_t *pseg2;
    seds_rangesegment_t  temp1;
    seds_rangesegment_t  temp2;
    bool                 result;

    pseg1 = seds_rangesegment_get_safe(lua, 1, &temp1);
    pseg2 = seds_rangesegment_get_safe(lua, 2, &temp2);

    result = (seds_rangelimit_is_contiguous(&pseg1->max, &pseg2->min)
              || seds_rangelimit_is_contiguous(&pseg1->min, &pseg2->max));

    lua_pushboolean(lua, result);

    return 1;
}

static bool
seds_rangesegment_compute_union(seds_rangesegment_t *pcombined, seds_rangesegment_t *pseg1, seds_rangesegment_t *pseg2)
{
    seds_rangesegment_t temp;
    bool                result;

    /* first compute the intersection, if this is not empty this
     * means the union will be a single segment */
    seds_rangesegment_compute_intersection(&temp, pseg1, pseg2);

    /* if the intersection was empty, there is still a possibility that
     * the segment(s) are immediately next to eachother even if they do not
     * overlap.  In this case the union is still representable as a single
     * segment. */
    if (!seds_rangesegment_is_empty(&temp) || seds_rangelimit_is_contiguous(&pseg1->max, &pseg2->min)
        || seds_rangelimit_is_contiguous(&pseg1->min, &pseg2->max))
    {
        seds_rangesegment_compute_bridge(pcombined, pseg1, pseg2);
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/**
 * Lua callable function to compute the union of two ranges
 *
 * Expected Stack args:
 *  1: object 1, may be a rangesegment OR a single value
 *  2: object 2, may be another rangesegment OR a single value
 *
 * If the two segments are contiguous, this returns a segment
 * that reflects the union of the two.  If they are not contiguous
 * this returns nil - this means the union cannot be expressed
 * as a single segment, it is disjointed.
 */
static int seds_rangesegment_union(lua_State *lua)
{
    seds_rangesegment_t *pseg1;
    seds_rangesegment_t *pseg2;
    seds_rangesegment_t  temp1;
    seds_rangesegment_t  temp2;
    seds_rangesegment_t  combined;

    pseg1 = seds_rangesegment_get_safe(lua, 1, &temp1);
    pseg2 = seds_rangesegment_get_safe(lua, 2, &temp2);

    if (!seds_rangesegment_compute_union(&combined, pseg1, pseg2))
    {
        /* not representable as a single segment */
        return 0;
    }

    if (seds_rangesegment_is_single_value(&combined))
    {
        /* the result is a single number, return it */
        lua_pushnumber(lua, combined.min.valx);
    }
    else
    {
        /* the result is a valid segment, return it */
        seds_rangesegment_push(lua, &combined);
    }

    return 1;
}

/**
 * Lua callable function to get a range object property
 *
 * Expected Stack args:
 *  1: range object
 *  2: property key
 */
static int seds_rangesegment_get_property(lua_State *lua)
{
    seds_rangesegment_t *pseg = luaL_checkudata(lua, 1, "seds_rangesegment");
    const char          *key;

    /*
     * Handle some fixed key names, but only if the key is a string
     */
    if (lua_isstring(lua, 2))
    {
        key = lua_tostring(lua, 2);

        if (strcmp(key, "is_single_value") == 0)
        {
            lua_pushboolean(lua, seds_rangesegment_is_single_value(pseg));
            return 1;
        }

        if (strcmp(key, "as_single_value") == 0)
        {
            seds_rangesegment_push_as_single_value(lua, 1, true);
            return 1;
        }

        if (strcmp(key, "matches") == 0)
        {
            lua_pushcfunction(lua, seds_rangesegment_matches);
            return 1;
        }

        if (strcmp(key, "intersection") == 0)
        {
            lua_pushcfunction(lua, seds_rangesegment_intersection);
            return 1;
        }

        if (strcmp(key, "union") == 0)
        {
            lua_pushcfunction(lua, seds_rangesegment_union);
            return 1;
        }

        if (strcmp(key, "empty") == 0)
        {
            lua_pushboolean(lua, seds_rangesegment_is_empty(pseg));
            return 1;
        }

        if (strcmp(key, "contiguous_with") == 0)
        {
            lua_pushcfunction(lua, seds_rangesegment_contiguous_with);
            return 1;
        }

        if (strcmp(key, "type") == 0)
        {
            /* return a string with the range type */
            return seds_rangesegment_get_type_as_string(lua, pseg);
        }

        if (strcmp(key, "min") == 0)
        {
            /* return as a 2-tuple with value and inclusion */
            return seds_rangesegment_get_limit_as_table(lua, &pseg->min);
        }

        if (strcmp(key, "max") == 0)
        {
            /* return as a 2-tuple with value and inclusion */
            return seds_rangesegment_get_limit_as_table(lua, &pseg->max);
        }

        if (strcmp(key, "min_value") == 0)
        {
            return seds_rangesegment_push_limit_value(lua, &pseg->min);
        }

        if (strcmp(key, "max_value") == 0)
        {
            return seds_rangesegment_push_limit_value(lua, &pseg->max);
        }

        if (strcmp(key, "min_inclusive") == 0)
        {
            lua_pushboolean(lua, pseg->min.is_inclusive);
            return 1;
        }

        if (strcmp(key, "max_inclusive") == 0)
        {
            lua_pushboolean(lua, pseg->max.is_inclusive);
            return 1;
        }
    }

    return 0;
}

/**
 * Lua callable function to get a range object property
 *
 * Expected Stack args:
 *  1: range object
 *  2: property key
 *  3: property value
 */
static int seds_rangesegment_set_property(lua_State *lua)
{
    seds_rangesegment_t *pseg = luaL_checkudata(lua, 1, "seds_rangesegment");
    const char          *key;

    /*
     * Handle some fixed key names, but only if the key is a string
     */
    if (lua_isstring(lua, 2))
    {
        key = lua_tostring(lua, 2);

        if (strcmp(key, "min") == 0)
        {
            return seds_rangesegment_set_limit_from_idx(lua, 3, &pseg->min);
        }

        if (strcmp(key, "max") == 0)
        {
            return seds_rangesegment_set_limit_from_idx(lua, 3, &pseg->max);
        }

        if (strcmp(key, "min_value") == 0)
        {
            pseg->min.is_defined = !lua_isnil(lua, 3);
            if (pseg->min.is_defined)
            {
                pseg->min.valx = lua_tonumber(lua, 3);
            }
            return 0;
        }

        if (strcmp(key, "max_value") == 0)
        {
            pseg->max.is_defined = !lua_isnil(lua, 3);
            if (pseg->max.is_defined)
            {
                pseg->max.valx = lua_tonumber(lua, 3);
            }
            return 0;
        }

        if (strcmp(key, "min_inclusive") == 0)
        {
            pseg->min.is_inclusive = lua_toboolean(lua, 3);
            return 0;
        }

        if (strcmp(key, "max_inclusive") == 0)
        {
            pseg->max.is_inclusive = lua_toboolean(lua, 3);
            return 0;
        }
    }

    return 0;
}

/**
 * Lua callable function to convert a range object to a string
 *
 * This implements the "tostring" method for range objects, and it
 * returns a summary string containing the bits, bytes, alignment,
 * and checksum of the object binary format.
 */
static int seds_rangesegment_to_string(lua_State *lua)
{
    seds_rangesegment_t *pseg = luaL_checkudata(lua, 1, "seds_rangesegment");
    char                 minbuf[28];
    char                 maxbuf[28];
    char                 stringbuf[64];

    if (pseg->min.is_defined)
    {
        snprintf(minbuf, sizeof(minbuf), "%.1lf", (double)pseg->min.valx);
    }
    else
    {
        snprintf(minbuf, sizeof(minbuf), "-inf");
    }
    if (pseg->max.is_defined)
    {
        snprintf(maxbuf, sizeof(maxbuf), "%.1lf", (double)pseg->max.valx);
    }
    else
    {
        snprintf(maxbuf, sizeof(maxbuf), "+inf");
    }

    snprintf(stringbuf,
             sizeof(stringbuf),
             "%c%s,%s%c",
             pseg->min.is_inclusive ? '[' : '(',
             minbuf,
             maxbuf,
             pseg->max.is_inclusive ? ']' : ')');

    lua_pushstring(lua, stringbuf);
    return 1;
}

/**
 * Lua callable function to check for range equality
 *
 * Expected Stack args:
 *  1: range object
 *  2: range object
 *
 */
static int seds_rangesegment_is_equal(lua_State *lua)
{
    seds_rangesegment_t *pseg1;
    seds_rangesegment_t *pseg2;
    seds_rangesegment_t  temp2;

    pseg1 = luaL_checkudata(lua, 1, "seds_rangesegment");
    pseg2 = seds_rangesegment_get_safe(lua, 2, &temp2);

    bool result;

    if (pseg1 == pseg2)
    {
        /* they are the same object */
        result = true;
    }
    else
    {
        result = (memcmp(pseg1, pseg2, sizeof(seds_rangesegment_t)) == 0);
    }

    lua_pushboolean(lua, result);
    return 1;
}

/**
 * Compare two rangesegments for sorting purposes
 *
 * This facilitates organizing segments by relative values, but it only makes sense
 * if the segments do not overlap at all.
 *
 * Return value is like strcmp()
 */
static int seds_rangesegment_sort_compare(const seds_rangesegment_t *pseg1, const seds_rangesegment_t *pseg2)
{
    int                 result;
    seds_rangesegment_t isect_temp;

    /* compute the intersection - this must be empty */
    seds_rangesegment_compute_intersection(&isect_temp, pseg1, pseg2);
    if (!seds_rangesegment_is_empty(&isect_temp))
    {
        /* they intersect, so consider it equal for value-checking purposes */
        result = 0;
    }
    else if (seds_rangesegment_compare_min(&pseg1->min, &pseg2->min))
    {
        /* pseg1 < pseg2 */
        result = -1;
    }
    else
    {
        result = 1;
    }

    return result;
}

/**
 * Lua callable function to compare rangesegments
 *
 * "Less than" does not apply directly to ranges, the main need here
 * is to be consistent in application of whatever comparison is done,
 * such that when e.g. sorting the resulting order will be consistent
 * from run to run.
 *
 * Expected Stack args:
 *  1: rangesegment object
 *  2: any numeric value
 *
 */
static int seds_rangesegment_is_lessthan(lua_State *lua)
{
    seds_rangesegment_t *pseg1;
    seds_rangesegment_t *pseg2;
    seds_rangesegment_t  temp2;

    pseg1 = luaL_checkudata(lua, 1, "seds_rangesegment");
    pseg2 = seds_rangesegment_get_safe(lua, 2, &temp2);

    lua_pushboolean(lua, seds_rangesegment_sort_compare(pseg1, pseg2) < 0);
    return 1;
}

/**
 * Lua callable function to compare rangesegments
 *
 * "Less than" does not apply directly to ranges, the main need here
 * is to be consistent in application of whatever comparison is done,
 * such that when e.g. sorting the resulting order will be consistent
 * from run to run.
 *
 * Expected Stack args:
 *  1: range object
 *  2: range object
 *
 */
static int seds_rangesegment_is_lessequal(lua_State *lua)
{
    seds_rangesegment_t *pseg1;
    seds_rangesegment_t *pseg2;
    seds_rangesegment_t  temp2;

    pseg1 = luaL_checkudata(lua, 1, "seds_rangesegment");
    pseg2 = seds_rangesegment_get_safe(lua, 2, &temp2);

    lua_pushboolean(lua, seds_rangesegment_sort_compare(pseg1, pseg2) <= 0);
    return 1;
}

/**
 * Lua-callable function to create a new range userdata object
 *
 * Expected input Lua stack, table initializer:
 *      1: table with data in the form:
 *          { min = { value = X, inclusive = Y }, max = { value = X, inclusive = Y } }
 *
 * Optionally min and max can be direct values, inclusivity will be assumed true
 */
int seds_rangesegment_new_from_table(lua_State *lua)
{
    seds_rangesegment_t *pseg = seds_rangesegment_push(lua, NULL);

    lua_pushstring(lua, "min");
    lua_gettable(lua, 1);
    seds_rangesegment_set_limit_from_idx(lua, lua_gettop(lua), &pseg->min);
    lua_pop(lua, 1);

    lua_pushstring(lua, "max");
    lua_gettable(lua, 1);
    seds_rangesegment_set_limit_from_idx(lua, lua_gettop(lua), &pseg->max);
    lua_pop(lua, 1);

    return 1;
}

/**
 * Lua-callable function to create a copy of a range userdata object
 *
 * Argument 1 must be another instance of seds_rangesegment
 */
int seds_rangesegment_new_copy(lua_State *lua)
{
    seds_rangesegment_push(lua, luaL_checkudata(lua, 1, "seds_rangesegment"));
    return 1;
}

/**
 * Lua-callable function to create a new range userdata object
 *
 * Expected input Lua stack, direct initializer:
 *      1: minimum value (number or nil)
 *      2: maximum value (number or nil)
 *      3: inclusion mode (string per EDS spec, or nil)
 */
int seds_rangesegment_new_from_direct(lua_State *lua)
{
    seds_rangesegment_t *pseg;
    const char          *inclusion;

    lua_settop(lua, 3); /* in case args were ommitted, assume nil */

    pseg = seds_rangesegment_push(lua, NULL);

    /* this will only get the values */
    seds_rangesegment_set_limit_from_idx(lua, 1, &pseg->min);
    seds_rangesegment_set_limit_from_idx(lua, 2, &pseg->max);

    inclusion = luaL_optstring(lua, 3, NULL);
    if (inclusion != NULL)
    {
        if (strcasecmp(inclusion, "inclusiveMinInclusiveMax") == 0)
        {
            pseg->min.is_inclusive = true;
            pseg->max.is_inclusive = true;
        }
        else if (strcasecmp(inclusion, "inclusiveMinExclusiveMax") == 0)
        {
            pseg->min.is_inclusive = true;
            pseg->max.is_inclusive = false;
        }
        else if (strcasecmp(inclusion, "exclusiveMinInclusiveMax") == 0)
        {
            pseg->min.is_inclusive = false;
            pseg->max.is_inclusive = true;
        }
        else if (strcasecmp(inclusion, "exclusiveMinExclusiveMax") == 0)
        {
            pseg->min.is_inclusive = false;
            pseg->max.is_inclusive = false;
        }
        else if (strcasecmp(inclusion, "greaterThan") == 0)
        {
            pseg->min.is_inclusive = false;
            pseg->max.is_defined   = false;
        }
        else if (strcasecmp(inclusion, "atLeast") == 0)
        {
            pseg->min.is_inclusive = true;
            pseg->max.is_defined   = false;
        }
        else if (strcasecmp(inclusion, "lessThan") == 0)
        {
            pseg->min.is_defined   = false;
            pseg->max.is_inclusive = false;
        }
        else if (strcasecmp(inclusion, "atMost") == 0)
        {
            pseg->min.is_defined   = false;
            pseg->max.is_inclusive = true;
        }
        else
        {
            return luaL_error(lua, "Invalid rangeType: %s", inclusion);
        }
    }

    return 1;
}

/**
 * Lua-callable function to create a new range userdata object
 *
 * Depending on the argument this will initialize the range object in one of 3 ways:
 * - table init
 * - copy init
 * - direct init
 *
 * The decision is made based on the type of the first arg
 */
int seds_rangesegment_new(lua_State *lua)
{
    int nret;

    if (lua_gettop(lua) != 1)
    {
        /* multi-arg construction: assume it is a direct initializer with a min and max */
        nret = seds_rangesegment_new_from_direct(lua);
    }
    else if (lua_type(lua, 1) == LUA_TTABLE)
    {
        /* table constructor form */
        nret = seds_rangesegment_new_from_table(lua);
    }
    else if (lua_type(lua, 1) == LUA_TUSERDATA)
    {
        /* copy constructor form */
        nret = seds_rangesegment_new_copy(lua);
    }
    else
    {
        /* single value constructor form */
        nret = seds_rangesegment_new_single_value(lua);
    }

    return nret;
}

/**
 * Helper function: Pushes a userdata object of seds_rangesegment_t onto the stack
 */
seds_rangesegment_t *seds_rangesegment_push(lua_State *lua, seds_rangesegment_t *pinit)
{
    seds_rangesegment_t *pseg = lua_newuserdata(lua, sizeof(seds_rangesegment_t));

    memset(pseg, 0, sizeof(*pseg));
    if (luaL_newmetatable(lua, "seds_rangesegment"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_rangesegment_get_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, seds_rangesegment_set_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, seds_rangesegment_to_string);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__eq");
        lua_pushcfunction(lua, seds_rangesegment_is_equal);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__lt");
        lua_pushcfunction(lua, seds_rangesegment_is_lessthan);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__le");
        lua_pushcfunction(lua, seds_rangesegment_is_lessequal);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    if (pinit != NULL)
    {
        /* copy constructor form */
        memcpy(pseg, pinit, sizeof(*pseg));
    }
    else
    {
        memset(pseg, 0, sizeof(*pseg));
    }

    return pseg;
}

/**
 * Helper function: Check for an empty range
 *
 * The range must be on the stack
 */
static bool seds_range_is_empty(lua_State *lua, int range_idx)
{
    seds_range_ud_t *prange = luaL_testudata(lua, range_idx, "seds_range");
    return (prange != NULL && prange->content == SEDS_RANGECONTENT_EMPTY);
}

/**
 * Helper function: Check for an infinite range
 *
 * The range must be on the stack
 */
static bool seds_range_is_infinite(lua_State *lua, int range_idx)
{
    seds_range_ud_t *prange = luaL_testudata(lua, range_idx, "seds_range");
    return (prange != NULL && prange->content == SEDS_RANGECONTENT_INFINITE);
}

/**
 * Helper function: Check for a normal range
 *
 * The range must be on the stack.  A normal range is one that has at least
 * one real segment; it is not empty nor infinite.
 */
static bool seds_range_is_normal(lua_State *lua, int range_idx)
{
    seds_range_ud_t *prange = luaL_testudata(lua, range_idx, "seds_range");
    return (prange != NULL && prange->content == SEDS_RANGECONTENT_NORMAL);
}

static void seds_range_set_normal(lua_State *lua, int range_idx, int content_idx)
{
    seds_range_ud_t *prange = luaL_checkudata(lua, range_idx, "seds_range");
    prange->content         = SEDS_RANGECONTENT_NORMAL;
    lua_pushvalue(lua, content_idx);
    lua_setuservalue(lua, range_idx);
}

static void seds_range_set_infinite(lua_State *lua, int range_idx)
{
    seds_range_ud_t *prange = luaL_checkudata(lua, range_idx, "seds_range");
    prange->content         = SEDS_RANGECONTENT_INFINITE;
    lua_pushnil(lua);
    lua_setuservalue(lua, range_idx);
}

static void seds_range_set_empty(lua_State *lua, int range_idx)
{
    seds_range_ud_t *prange = luaL_checkudata(lua, range_idx, "seds_range");
    prange->content         = SEDS_RANGECONTENT_EMPTY;
    lua_pushnil(lua);
    lua_setuservalue(lua, range_idx);
}

/**
 * Helper function: Call a routine for every segment within the range
 *
 * The range and the function must be on the stack
 */
static int seds_range_foreach_segment_impl(lua_State *lua, int range_idx)
{
    int func_idx;
    int nsegs;

    nsegs = 0;

    if (seds_range_is_normal(lua, range_idx))
    {
        func_idx = lua_gettop(lua); /* function to call should be at top */

        lua_getuservalue(lua, range_idx);
        if (lua_istable(lua, -1))
        {
            /* multi-segment range; loop through all segments */
            lua_pushnil(lua);
            while (lua_next(lua, -2) != 0)
            {
                /* do not care about the value, the key is the segment */
                lua_pop(lua, 1);

                lua_pushvalue(lua, func_idx);
                lua_pushvalue(lua, -2);
                lua_call(lua, 1, 0);

                ++nsegs;
            }
        }
        else
        {
            /* single-segment range; just call the function */
            lua_pushvalue(lua, func_idx);
            lua_pushvalue(lua, -2);
            lua_call(lua, 1, 0);

            ++nsegs;
        }

        /* reset stack to original position, removes anything pushed here */
        lua_settop(lua, func_idx);
    }

    return nsegs;
}

/**
 * Lua-callable Helper function to attempt to merge/unionize segments
 * within a range
 *
 * Expected input stack:
 *  1: segment to unionize (seds_rangesegment userdata or a single number)
 *
 * Upvalues:
 *  1: subject rangesegment
 */
static int seds_range_try_merge_segments(lua_State *lua)
{
    seds_rangesegment_t *pseg1;
    seds_rangesegment_t *pseg2;
    seds_rangesegment_t  temp2;
    seds_rangesegment_t  combined;

    lua_settop(lua, 1);
    pseg1 = luaL_checkudata(lua, lua_upvalueindex(1), "seds_rangesegment");
    pseg2 = seds_rangesegment_get_safe(lua, 1, &temp2);

    if (seds_rangesegment_compute_union(&combined, pseg1, pseg2))
    {
        /* replace the original segment with the combined one */
        *pseg1 = combined;
    }

    return 0;
}

/**
 * Lua-callable Helper function to filter overlapping segments
 *
 * Expected input stack:
 *  1: value to test (seds_rangesegment userdata or a single number)
 *
 * Upvalues:
 *  1: Table to add non-overlapping results into
 *  2: value to compare against
 */
static int seds_range_check_overlap(lua_State *lua)
{
    seds_rangesegment_t *prefseg;
    seds_rangesegment_t *pcheckseg;
    seds_rangesegment_t  temp1;
    seds_rangesegment_t  temp2;
    int                  ref_idx = lua_upvalueindex(2);
    bool                 is_overlap;

    /* simplest case, if the values are exactly equal, they overlap */
    is_overlap = lua_compare(lua, 1, ref_idx, LUA_OPEQ);
    if (!is_overlap && seds_rangesegment_acceptable_value(lua, 1) && seds_rangesegment_acceptable_value(lua, ref_idx))
    {
        prefseg   = seds_rangesegment_get_safe(lua, ref_idx, &temp1);
        pcheckseg = seds_rangesegment_get_safe(lua, 1, &temp2);

        is_overlap = seds_rangesegment_includes_rangesegment(prefseg, pcheckseg);
    }

    /* if it completely overlaps reference, do nothing.  otherwise this is a different value
     * and needs to be added to the result set */
    if (!is_overlap)
    {
        /* disjointed from reference, add it to table */
        lua_pushboolean(lua, true);
        lua_rawset(lua, lua_upvalueindex(1));
    }

    return 0;
}

/**
 * Lua-callable Helper function to append a segment into another range
 *
 * Expected input stack:
 *  1: segment to add (seds_rangesegment userdata or a single number)
 *
 * Upvalues:
 *  1: Dest range (seds_range userdata)
 *
 * NOTE: this modifies the destination (upvalue 1) in-place.
 */

static int seds_range_append_segment(lua_State *lua)
{
    seds_rangesegment_t *pmerged;
    seds_rangesegment_t  temp_segment;
    int                  range_idx = lua_upvalueindex(1);
    int                  result_idx;

    lua_settop(lua, 1);

    /* check for the simple cases */
    if (seds_range_is_infinite(lua, range_idx))
    {
        /* nothing to do, the dest is already infinite */
        return 0;
    }

    /* range segments need to be merged */
    /* special handling for numeric segments, they can be combined */
    if (seds_rangesegment_acceptable_value(lua, 1))
    {
        /* first merge with anything it is contiguous with */
        /* index 2 is a NEW segment based on arg 1 (always a segment, not a number) */
        pmerged = seds_rangesegment_push(lua, seds_rangesegment_get_safe(lua, 1, &temp_segment));
        lua_pushvalue(lua, 2);
        lua_pushcclosure(lua, seds_range_try_merge_segments, 1);
        seds_range_foreach_segment_impl(lua, range_idx);
        lua_pop(lua, 1); /* remove seds_range_try_merge_segments */

        /* push the new segment onto the stack - if the new segment is a singular value, push that */
        if (seds_rangesegment_is_single_value(pmerged))
        {
            lua_pushnumber(lua, pmerged->min.valx);
        }
        else
        {
            lua_pushvalue(lua, 2); /* keep it as a segment */
        }
    }
    else
    {
        /* the value we are adding is itself */
        lua_pushvalue(lua, 1);
    }

    lua_newtable(lua);
    result_idx = lua_gettop(lua); /* this is our temporary result table */

    lua_pushvalue(lua, result_idx);
    lua_pushvalue(lua, 2);
    lua_pushcclosure(lua, seds_range_check_overlap, 2);
    seds_range_foreach_segment_impl(lua, range_idx);
    lua_pop(lua, 1); /* remove seds_range_check_overlap */

    /* now check if the result table is empty */
    lua_pushvalue(lua, 2);
    lua_pushnil(lua);
    if (lua_next(lua, result_idx))
    {
        /* drop key and value, does not matter, this only means its not empty */
        lua_pop(lua, 2);

        /* add the segment to the table (key is on top already) */
        lua_pushboolean(lua, true);
        lua_rawset(lua, result_idx);
    }
    else
    {
        /* replace the original result_idx (table) with this single value */
        lua_replace(lua, result_idx);
    }

    /* this new item becomes the userdata of the range */
    seds_range_set_normal(lua, range_idx, result_idx);

    return 0;
}

/**
 * Lua callable function to append all segments from one range into another range,
 * creating the union of the two ranges
 *
 * Expected Stack args:
 *  1: range object
 *  2: range to add in to (1)
 *
 * A new range object is returned containing a new set of segments, this
 * does not modify the input args
 */
static int seds_range_union(lua_State *lua)
{
    int arg_idx;
    int result_idx;

    seds_range_push(lua, SEDS_RANGECONTENT_EMPTY); /* make a NEW range object to return, do not modify the original */
    result_idx = lua_gettop(lua);

    lua_pushvalue(lua, result_idx);
    lua_pushcclosure(lua, seds_range_append_segment, 1);

    for (arg_idx = 1; arg_idx < result_idx && !seds_range_is_infinite(lua, result_idx); ++arg_idx)
    {
        if (luaL_testudata(lua, arg_idx, "seds_range"))
        {
            if (seds_range_is_infinite(lua, arg_idx))
            {
                seds_range_set_infinite(lua, result_idx);
            }
            else if (!seds_range_is_empty(lua, arg_idx))
            {
                seds_range_foreach_segment_impl(lua, arg_idx);
            }
        }
        else if (!lua_isnil(lua, arg_idx))
        {
            /* just call the append function with the original arg */
            /* need to push the append function again because lua_call removes it,
             * and we might need it for the next iteration of the loop */
            lua_pushvalue(lua, result_idx + 1);
            lua_pushvalue(lua, arg_idx);
            lua_call(lua, 1, 0);
        }
    }

    /* return the destination range, allows for chaining */
    lua_settop(lua, result_idx);
    return 1;
}

/**
 * Lua callable function to check if a range matches a value
 *
 * Expected Stack args:
 *  1: segment to check (can also be number)
 *
 * Upvalues:
 *  1: light user data pointing to boolean result
 *  2: reference value
 */
static int seds_range_check_segmentmatch(lua_State *lua)
{
    int *pmatchcount;
    int  refval_idx;
    bool localresult;

    pmatchcount = lua_touserdata(lua, lua_upvalueindex(1));
    refval_idx  = lua_upvalueindex(2);

    /* if the reference and the test value are equal, move on */
    localresult = lua_compare(lua, 1, refval_idx, LUA_OPEQ);
    if (!localresult && luaL_testudata(lua, 1, "seds_rangesegment") != NULL)
    {
        lua_pushcfunction(lua, seds_rangesegment_matches);
        lua_pushvalue(lua, 1);
        lua_pushvalue(lua, refval_idx);
        lua_call(lua, 2, 1);

        localresult = lua_toboolean(lua, -1);
        lua_pop(lua, 1);
    }

    if (localresult)
    {
        /* consider it a match */
        ++(*pmatchcount);
    }

    return 0;
}

/**
 * Lua callable function to check if a range matches a segment or value
 *
 * Expected Stack args:
 *  1: range object
 *  2: value to check (may be a value or a segment)
 */
static int seds_range_matches_segment(lua_State *lua)
{
    int required_segments;
    int matching_segments;

    matching_segments = 0;

    if (seds_range_is_empty(lua, 1))
    {
        /* will always be false */
        required_segments = 1;
    }
    else if (seds_range_is_infinite(lua, 1))
    {
        /* will always be true */
        required_segments = 0;
    }
    else
    {
        /* need at least one positive match */
        required_segments = 1;

        lua_pushlightuserdata(lua, &matching_segments);
        lua_pushvalue(lua, 2);
        lua_pushcclosure(lua, seds_range_check_segmentmatch, 2);

        seds_range_foreach_segment_impl(lua, 1);
    }

    lua_pushboolean(lua, matching_segments >= required_segments);
    return 1;
}

/**
 * Lua callable function to check if a range matches a value
 *
 * Expected Stack args:
 *  1: value to check (may be a value or a segment)
 *
 * Upvalues:
 *  1: counter object (pointer to int as lightuserdata)
 *  2: comparison range object
 */
static int seds_range_subsegment_match_helper(lua_State *lua)
{
    int *pmatchcount = lua_touserdata(lua, lua_upvalueindex(1));

    lua_pushcfunction(lua, seds_range_matches_segment);
    lua_pushvalue(lua, lua_upvalueindex(2));
    lua_pushvalue(lua, 1);
    lua_call(lua, 2, 1);

    if (lua_toboolean(lua, -1))
    {
        ++(*pmatchcount);
    }

    return 0;
}

/**
 * Lua callable function to check if a range matches a value
 *
 * Expected Stack args:
 *  1: range object
 *  2: value to check (may be a value, a segment, or another range)
 */
static int seds_range_matches(lua_State *lua)
{
    int needed_segments;
    int matching_segments;

    luaL_checkudata(lua, 1, "seds_range");
    lua_settop(lua, 2); /* ignore extra args */

    matching_segments = 0;

    if (seds_range_is_empty(lua, 1))
    {
        /* will always result as false */
        needed_segments = 1;
    }
    else if (seds_range_is_infinite(lua, 1))
    {
        /* will always result as true */
        needed_segments = 0;
    }
    else
    {
        lua_pushlightuserdata(lua, &matching_segments);
        lua_pushvalue(lua, 1);
        lua_pushcclosure(lua, seds_range_subsegment_match_helper, 2);

        if (luaL_testudata(lua, 2, "seds_range"))
        {
            /* check all the segments from the second range */
            needed_segments = seds_range_foreach_segment_impl(lua, 2);
        }
        else
        {
            /* pass through the value to the helper */
            needed_segments = 1;
            lua_pushvalue(lua, 2);
            lua_call(lua, 1, 0);
        }

        lua_settop(lua, 2); /* remove anything pushed above */
    }

    lua_pushboolean(lua, (matching_segments == needed_segments));
    return 1;
}

/**
 * Lua callable function to calculate a intersection
 *
 * Expected Stack args:
 *  1: value to intersect (may be a value or a single segment, NOT another range)
 *
 * Upvalues:
 *  1: function to call on resulting segment(s)
 *  2: value to intersect (may be a value or a single segment, OR another range)
 *
 * NOTE: the subject value must not be an infinite range.  The caller already
 * checked for this possibility and handled that.  This works with either an
 * empty range or a normally-defined range.
 */
static int seds_range_subsegment_intersection_helper(lua_State *lua)
{
    int result_idx  = lua_upvalueindex(1);
    int subject_idx = lua_upvalueindex(2);

    if (luaL_testudata(lua, subject_idx, "seds_range"))
    {
        /* If intersecting with another range, need to do each segment of that range */
        /* we can recursively call this function to do this */
        lua_pushvalue(lua, result_idx);
        lua_pushvalue(lua, 1);
        lua_pushcclosure(lua, seds_range_subsegment_intersection_helper, 2);
        seds_range_foreach_segment_impl(lua, subject_idx);
    }
    else
    {
        if (seds_rangesegment_acceptable_value(lua, 1) && seds_rangesegment_acceptable_value(lua, subject_idx))
        {
            /* compute the intersection of the two numeric segments (either of which could be a single value) */
            lua_pushcfunction(lua, seds_rangesegment_intersection);
            lua_pushvalue(lua, 1);
            lua_pushvalue(lua, subject_idx);
            lua_call(lua, 2, 1);
        }
        else if (seds_range_is_infinite(lua, subject_idx) || lua_compare(lua, subject_idx, 1, LUA_OPEQ))
        {
            lua_pushvalue(lua, 1);
        }
        else
        {
            /* there is no intersection */
            lua_pushnil(lua);
        }

        if (!lua_isnil(lua, -1))
        {
            /* result is valid - call the function */
            lua_pushvalue(lua, result_idx);
            lua_insert(lua, -2);
            lua_call(lua, 1, 0);
        }
    }
    return 0;
}

/**
 * Lua callable function to calculate the intersection of two ranges
 *
 * Expected Stack args:
 *  1: range object
 *  2: value to intersect with (may be a value, a segment, or another range)
 */
static int seds_range_intersection(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_range");

    /* if no arg was given, this becomes a noop */
    if (lua_gettop(lua) == 1)
    {
        seds_range_push(lua, SEDS_RANGECONTENT_INFINITE);
    }

    /* make sure the arg is another seds_range object */
    /* this makes the following logic more consistent as it always intersects two seds_range objects */
    lua_settop(lua, 2);
    if (!luaL_testudata(lua, 2, "seds_range"))
    {
        /* arg is not natively a range - make a seds_range to wrap it */
        lua_pushcfunction(lua, seds_range_union);
        lua_insert(lua, 2);
        lua_call(lua, 1, 1);
    }

    /* simple case if either range is infinite - then the result is equal to the other arg */
    if (seds_range_is_infinite(lua, 1))
    {
        /* result is equivalent to arg2 */
        lua_pushvalue(lua, 2);
    }
    else if (seds_range_is_infinite(lua, 2))
    {
        /* result is equivalent to arg1 */
        lua_pushvalue(lua, 1);
    }
    else
    {
        /* result will be a new range, make it initially empty */
        seds_range_push(lua, SEDS_RANGECONTENT_EMPTY);

        /* this computes the intersection of each segment
         * and appends all the non-empty resulting segments to the result range */
        lua_pushvalue(lua, 3);
        lua_pushcclosure(lua, seds_range_append_segment, 1);
        lua_pushvalue(lua, 2);
        lua_pushcclosure(lua, seds_range_subsegment_intersection_helper, 2);

        /* call the intersection helper for everything in THIS range */
        /* this in turn will invoke seds_range_append_segment for all overlapping segments */
        seds_range_foreach_segment_impl(lua, 1);

        lua_pop(lua, 1); /* remove the helper function */
    }

    /* return the result which was left at the top of the stack */
    return 1;
}

/**
 * Lua callable function to invoke a callback on all range segments
 *
 * Expected Stack args:
 *  1: range object
 *  2: function to invoke
 */
static int seds_range_foreach_segment(lua_State *lua)
{
    lua_settop(lua, 2);
    seds_range_foreach_segment_impl(lua, 1);
    return 0;
}

/**
 * Lua callable function to get a range object property
 *
 * Expected Stack args:
 *  1: range object
 *  2: property key
 */
static int seds_range_get_property(lua_State *lua)
{
    const char *key;

    luaL_checkudata(lua, 1, "seds_range");
    lua_settop(lua, 2);

    /*
     * Handle some fixed key names, but only if the key is a string
     */
    if (lua_isstring(lua, 2))
    {
        key = lua_tostring(lua, 2);

        if (strcmp(key, "union") == 0)
        {
            lua_pushcfunction(lua, seds_range_union);
            return 1;
        }

        if (strcmp(key, "foreach") == 0)
        {
            lua_pushcfunction(lua, seds_range_foreach_segment);
            return 1;
        }

        if (strcmp(key, "matches") == 0)
        {
            lua_pushcfunction(lua, seds_range_matches);
            return 1;
        }

        if (strcmp(key, "intersection") == 0)
        {
            lua_pushcfunction(lua, seds_range_intersection);
            return 1;
        }

        if (strcmp(key, "empty") == 0)
        {
            lua_pushboolean(lua, seds_range_is_empty(lua, 1));
            return 1;
        }

        if (strcmp(key, "infinite") == 0)
        {
            lua_pushboolean(lua, seds_range_is_infinite(lua, 1));
            return 1;
        }

        /* if this is a singleton segment, pass it through */
        lua_getuservalue(lua, 1);
        if (luaL_testudata(lua, -1, "seds_rangesegment"))
        {
            lua_replace(lua, 1);
            return seds_rangesegment_get_property(lua);
        }

        if (strcmp(key, "is_single_value") == 0)
        {
            lua_pushboolean(lua, lua_isnumber(lua, -1) || lua_isstring(lua, -1) || lua_isboolean(lua, -1));
            return 1;
        }

        if (strcmp(key, "as_single_value") == 0)
        {
            if (lua_isnumber(lua, -1) || lua_isstring(lua, -1) || lua_isboolean(lua, -1))
            {
                /* just return the value */
            }
            else
            {
                lua_pushnil(lua);
            }
            return 1;
        }
    }
    return 0;
}

/**
 * Lua callable function to set a range object property
 *
 * Expected Stack args:
 *  1: range object
 *  2: property key
 *  3: property value
 */
static int seds_range_set_property(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_range");
    lua_settop(lua, 3);

    /* Currently the range object has no (writable) properties of its own */
    return 0;
}

static int seds_range_tostring_helper(lua_State *lua)
{
    luaL_Buffer *pbuf = lua_touserdata(lua, lua_upvalueindex(1));

    /* convert the value to a string and concat */
    luaL_addstring(pbuf, ",");
    luaL_addstring(pbuf, luaL_tolstring(lua, 1, NULL));

    return 0;
}

/**
 * Lua callable function to convert a range object to a string
 *
 * This implements the "tostring" method for range objects, and it
 * returns a summary string containing the bits, bytes, alignment,
 * and checksum of the object binary format.
 */
static int seds_range_to_string(lua_State *lua)
{
    luaL_Buffer lbuf;

    luaL_checkudata(lua, 1, "seds_range");

    if (seds_range_is_empty(lua, 1))
    {
        lua_pushstring(lua, "(empty)");
    }
    else if (seds_range_is_infinite(lua, 1))
    {
        lua_pushstring(lua, "[infinite]");
    }
    else
    {
        luaL_buffinit(lua, &lbuf);
        lua_pushlightuserdata(lua, &lbuf);
        lua_pushcclosure(lua, seds_range_tostring_helper, 1);

        seds_range_foreach_segment_impl(lua, 1);
        luaL_pushresult(&lbuf);

        /* chop off the leading comma */
        if (lua_rawlen(lua, -1) > 1)
        {
            lua_pushstring(lua, lua_tostring(lua, -1) + 1);
        }
    }

    return 1;
}

/**
 * Lua callable function to check for range equality
 *
 * Expected Stack args:
 *  1: range object
 *  2: range object
 *
 */
static int seds_range_is_equal(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_range");
    luaL_checkudata(lua, 2, "seds_range");

    /* cheat here, this is not terribly efficient
     * call "matches" in both directions, if r1 matches r2
     * and r2 matches r1, then they must be equivalent.
     *
     * (if one was a subset of the other, one test would fail)
     */

    lua_pushcfunction(lua, seds_range_matches);
    lua_pushvalue(lua, 1);
    lua_pushvalue(lua, 2);
    lua_call(lua, 2, 1);
    if (lua_toboolean(lua, -1))
    {
        lua_pop(lua, 1);
        lua_pushcfunction(lua, seds_range_matches);
        lua_pushvalue(lua, 2);
        lua_pushvalue(lua, 1);
        lua_call(lua, 2, 1);
    }

    return 1;
}

/**
 * Lua callable function to compare ranges
 *
 * "Less than" does not apply directly to ranges, the main need here
 * is to be consistent in application of whatever comparison is done,
 * such that when e.g. sorting the resulting order will be consistent
 * from run to run.
 *
 * Expected Stack args:
 *  1: range object
 *  2: range object
 *
 */
static int seds_range_is_lessthan(lua_State *lua)
{
    /* simplest method is to convert both to strings, then return the result
     * of that comparison */

    luaL_tolstring(lua, 1, NULL);
    luaL_tolstring(lua, 2, NULL);
    lua_pushboolean(lua, lua_compare(lua, -1, -2, LUA_OPLT));
    return 1;
}

/**
 * Lua callable function to compare ranges
 *
 * "Less than" does not apply directly to ranges, the main need here
 * is to be consistent in application of whatever comparison is done,
 * such that when e.g. sorting the resulting order will be consistent
 * from run to run.
 *
 * Expected Stack args:
 *  1: range object
 *  2: range object
 *
 */
static int seds_range_is_lessequal(lua_State *lua)
{
    /* simplest method is to convert both to strings, then return the result
     * of that comparison */

    luaL_tolstring(lua, 1, NULL);
    luaL_tolstring(lua, 2, NULL);
    lua_pushboolean(lua, lua_compare(lua, -1, -2, LUA_OPLE));
    return 1;
}

/**
 * Helper function: Pushes a userdata object of seds_range_t onto the stack
 * The new range will be empty
 */
static void seds_range_push(lua_State *lua, seds_rangecontent_t initial_content)
{
    seds_range_ud_t *prange = lua_newuserdata(lua, sizeof(seds_range_ud_t));

    memset(prange, 0, sizeof(*prange));
    prange->content = initial_content;
    if (luaL_newmetatable(lua, "seds_range"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_range_get_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, seds_range_set_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, seds_range_to_string);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__eq");
        lua_pushcfunction(lua, seds_range_is_equal);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__lt");
        lua_pushcfunction(lua, seds_range_is_lessthan);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__le");
        lua_pushcfunction(lua, seds_range_is_lessequal);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);
}

/**
 * Helper function: Pushes a userdata object of seds_range_t onto the stack,
 * and initializes it to the given arguments
 *
 * Upvalue 1 indicates whether the new range should be initially empty or infinite
 */
static int seds_range_new(lua_State *lua)
{
    if (lua_isnoneornil(lua, 1))
    {
        /* by default if no args, create an infinite range */
        seds_range_push(lua, SEDS_RANGECONTENT_INFINITE);

        if (!lua_toboolean(lua, lua_upvalueindex(1)))
        {
            /* create an empty range if using the empty constructor */
            seds_range_set_empty(lua, -1);
        }
    }
    else
    {
        /* if the argument is another range, this acts as a copy constructor */
        if (luaL_testudata(lua, 1, "seds_range"))
        {
            /* just in case - prune any other args */
            lua_settop(lua, 1);
        }
        else
        {
            /* for everything else make a segment object based on the arg(s) */
            /* this must be done first to pass-through the args correctly. */
            seds_rangesegment_new(lua);
        }

        /* Call the union routine which returns a new range matching the new segment */
        lua_pushcfunction(lua, seds_range_union);
        lua_insert(lua, -2);

        lua_call(lua, 1, 1);
    }

    return 1;
}

/**
 * Helper function: Checks for a matching key within the rangemap
 * If found: pushes two items on the stack - key + value - and returns true
 * If not found: leaves the stack at its original position, and returns false
 */
static bool seds_rangemap_find_key(lua_State *lua, int map_idx, int arg_idx)
{
    bool found;
    int  key_idx;

    found = false;

    /* this requires walking the list, checking each range */
    lua_pushnil(lua);
    key_idx = lua_gettop(lua);
    while (lua_next(lua, map_idx))
    {
        if (lua_isuserdata(lua, key_idx))
        {
            /* invoke the "matches" metamethod */
            lua_getfield(lua, key_idx, "matches");
            if (lua_isfunction(lua, -1))
            {
                lua_pushvalue(lua, key_idx); /* the range object */
                lua_pushvalue(lua, arg_idx); /* the argument */
                lua_call(lua, 2, 1);

                if (lua_toboolean(lua, -1))
                {
                    /* its a match - leave key + value on stack */
                    lua_pop(lua, 1);
                    found = true;
                    break;
                }
            }
        }
        lua_settop(lua, key_idx);
    }

    return found;
}

/**
 * Lua callable meta function: Gets a value from a rangemap
 * This uses range-aware key matching
 *
 * Expected Stack args:
 *  1: rangemap object
 *  2: key value
 */
static int seds_rangemap_get_value(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_rangemap");
    lua_settop(lua, 2);

    if (!lua_isnil(lua, 2))
    {
        /* first try a simple lookup from the table, for an exact match */
        lua_getuservalue(lua, 1); /* index 3 */
        lua_pushvalue(lua, 2);
        lua_rawget(lua, 3);

        if (lua_isnil(lua, -1))
        {
            /*
             * need to try range checking:
             * if this finds a matching key it pushes both key+value to the stack
             * if it does not, it leaves the stack alone (so top will still be nil)
             */
            seds_rangemap_find_key(lua, 3, 2);
        }
    }

    return 1;
}

/**
 * Lua callable meta function: Sets a value in a rangemap
 * This uses range-aware key matching. If a matching key
 * previously existed the value will be replaced with the new value
 *
 * Expected Stack args:
 *  1: rangemap object
 *  2: key value
 *  3: value to set (may be nil to delete a key)
 */
static int seds_rangemap_set_value(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_rangemap");
    lua_settop(lua, 3);
    lua_getuservalue(lua, 1); /* index 4 */

    /* need to first check if the key already exists */
    /* check for an exact match */
    lua_pushvalue(lua, 2);
    lua_rawget(lua, 4);
    if (lua_isnil(lua, -1))
    {
        /* need to try range checking to see if this key already exists */
        if (seds_rangemap_find_key(lua, 4, 2))
        {
            /* duplicate key - should this throw an exception? */
            /* replace old value with new value, rather than adding a new key */
            lua_pop(lua, 1);
            lua_replace(lua, 2);
        }
    }

    /* set the key in the table */
    lua_pushvalue(lua, 2);
    lua_pushvalue(lua, 3);
    lua_rawset(lua, 4);

    return 0;
}

/**
 * Lua callable meta function: Convert rangemap to a string
 *
 * Expected Stack args:
 *  1: rangemap object
 */
static int seds_rangemap_tostring(lua_State *lua)
{
    luaL_Buffer buf;
    bool        is_empty;

    is_empty = true;
    luaL_buffinit(lua, &buf);
    luaL_addstring(&buf, "seds_rangemap:");
    lua_getuservalue(lua, 1);
    lua_pushnil(lua);
    while (lua_next(lua, -2))
    {
        if (is_empty)
        {
            is_empty = false;
        }
        else
        {
            luaL_addstring(&buf, "+");
        }
        luaL_addstring(&buf, luaL_tolstring(lua, -2, NULL));
        lua_pop(lua, 1);
        luaL_addstring(&buf, " => ");
        luaL_addstring(&buf, luaL_tolstring(lua, -1, NULL));
        lua_pop(lua, 2);
    }
    luaL_pushresult(&buf);
    return 1;
}

/**
 * Lua callable meta function: iterate over the map
 *
 * Expected Stack args:
 *  1: rangemap object
 */
static int seds_rangemap_get_pairs(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_rangemap");
    lua_settop(lua, 1);
    lua_getglobal(lua, "pairs");
    lua_getuservalue(lua, 1);
    lua_call(lua, 1, 3);
    return 3;
}

/**
 * Helper function: Pushes a userdata object of seds_rangemap
 * The new map will be empty
 */
static int seds_rangemap_new(lua_State *lua)
{
    seds_range_ud_t *pmap = lua_newuserdata(lua, sizeof(seds_range_ud_t));

    memset(pmap, 0, sizeof(*pmap));
    if (luaL_newmetatable(lua, "seds_rangemap"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_rangemap_get_value);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, seds_rangemap_set_value);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, seds_rangemap_tostring);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__pairs");
        lua_pushcfunction(lua, seds_rangemap_get_pairs);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    /* start with an empty table */
    lua_newtable(lua);
    lua_setuservalue(lua, -2);

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
void seds_range_register_globals(lua_State *lua)
{
    luaL_checktype(lua, -1, LUA_TTABLE);
    lua_pushcfunction(lua, seds_rangesegment_new);
    lua_setfield(lua, -2, "new_rangesegment");
    lua_pushboolean(lua, true);
    lua_pushcclosure(lua, seds_range_new, 1);
    lua_setfield(lua, -2, "new_range_infinite");
    lua_pushboolean(lua, false);
    lua_pushcclosure(lua, seds_range_new, 1);
    lua_setfield(lua, -2, "new_range_empty");
    lua_pushcfunction(lua, seds_rangemap_new);
    lua_setfield(lua, -2, "new_rangemap");
}
