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
 * \file     seds_memreq.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of the "memreq" memory requirement
 * calculation Lua object type.  This object facilitates
 * computing the actual size of EDS-described data types.
 *
 * It maintains a running total of the bit size as well as
 * expected byte requirements and alignment requirements based
 * on typical CPU requirements (i.e. aligned at corresponding
 * powers of 2 for integer types, padding as needed).
 *
 * These objects are instantiated as Lua userdata objects, and
 * metamethods are attached that allow for concatenation, unions,
 * and multiplication (i.e. arrays).  It also maintains a checksum
 * of the binary data signature, which can be used to detect changes
 * in subsequent versions of EDS databases.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "seds_memreq.h"


/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/


/**
 * Lua-callable Helper function to append an element to a container memory requirement
 *
 * Note that only the bit calculation here is truly reliable, as this is defined
 * by SEDS book 876.  The byte calculations would reflect the _typical_ or expected
 * size in a C environment, but a specific compiler implementation could be different.
 *
 * That being said - the C size estimation is only used for sizing buffers, and if
 * an actual implementation is different is more likely to be smaller than larger,
 * so it will still work OK.
 */
static int seds_memreq_add(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");
    seds_memreq_t *padd = luaL_checkudata(lua, 2, "seds_memreq");
    seds_integer_t start_offset_bytes;
    seds_integer_t start_offset_bits;

    luaL_argcheck(lua, psize != NULL, 1, "memreq argument expected");
    luaL_argcheck(lua, padd != NULL, 2, "memreq argument expected");

    /*
     * If the number of EDS-defined bits is exactly 8 times the number of
     * bytes allocated by the compiler for this object, then this means there
     * is no implicit padding and the structure can be considered ideally packed.
     * This means the runtime can make some optimizations, such as using
     * memcpy() between packed structs and native structs rather than twiddling
     * bits, which will be much faster.
     *
     * Otherwise it means there is padding and/or encoding diffs, and as such the
     * normal bit packing must be done.
     */
    if ((8 * padd->local_storage_bytes) != padd->raw_bit_size)
    {
        /* some type of padding is present (implicit or explicit)
         * so this must be flagged as such */
        psize->packing_status = SEDS_BYTEPACK_STATUS_OTHER;
    }
    else if (psize->packing_status == SEDS_BYTEPACK_STATUS_UNDEFINED)
    {
        /* inherit the status of the added element */
        psize->packing_status = padd->packing_status;
    }
    else if (padd->packing_status != SEDS_BYTEPACK_STATUS_UNDEFINED &&
            psize->packing_status != padd->packing_status)
    {
        psize->packing_status = SEDS_BYTEPACK_STATUS_OTHER;
    }

    start_offset_bits = psize->raw_bit_size;
    start_offset_bytes = (psize->endpoint_bytes + padd->local_align_mask) & ~padd->local_align_mask;
    psize->endpoint_bytes = start_offset_bytes + padd->local_storage_bytes;

    if (psize->local_align_mask < padd->local_align_mask)
    {
        psize->local_align_mask = padd->local_align_mask;
    }
    psize->raw_bit_size += padd->raw_bit_size;
    psize->local_storage_bytes = (psize->endpoint_bytes + psize->local_align_mask) & ~psize->local_align_mask;

    psize->checksum = seds_update_checksum_int(psize->checksum, psize->raw_bit_size);
    psize->checksum = seds_update_checksum_int(psize->checksum, padd->checksum);

    lua_pushinteger(lua, start_offset_bytes);
    lua_pushinteger(lua, start_offset_bits);
    return 2;
}

/**
 * Lua-callable Helper function to create a union of two structures
 *
 * This is basically just selecting the larger of the two, however it
 * has an effect on alignment as well.
 */
static int seds_memreq_union(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");
    seds_memreq_t *padd = luaL_checkudata(lua, 2, "seds_memreq");

    luaL_argcheck(lua, psize != NULL, 1, "memreq argument expected");
    luaL_argcheck(lua, padd != NULL, 2, "memreq argument expected");

    psize->local_align_mask |= padd->local_align_mask;

    if (padd->local_storage_bytes > psize->local_storage_bytes)
    {
        psize->local_storage_bytes = padd->local_storage_bytes;
    }

    if (padd->raw_bit_size > psize->raw_bit_size)
    {
        psize->raw_bit_size = padd->raw_bit_size;
    }

    psize->packing_status = SEDS_BYTEPACK_STATUS_OTHER;
    psize->checksum ^= padd->checksum;
    return 0;
}

/**
 * Lua-callable Helper function to multiply the size of a structure
 *
 * This is intended for array size calculation, where a given base
 * object is duplicated according to the dimension of the array.
 */
static int seds_memreq_multiply(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");
    seds_integer_t multiplier = luaL_checkinteger(lua, 2);
    seds_boolean_t byte_pack = lua_toboolean(lua, 3);

    if (psize->raw_bit_size == 0)
    {
        return 0;
    }

    psize->raw_bit_size *= multiplier;

    if (byte_pack)
    {
        psize->endpoint_bytes = (psize->raw_bit_size + 7) / 8;
    }
    else
    {
        psize->endpoint_bytes = psize->local_storage_bytes * multiplier;
    }
    psize->local_storage_bytes = (psize->endpoint_bytes + psize->local_align_mask) & ~psize->local_align_mask;

    psize->checksum = seds_update_checksum_int(psize->checksum, psize->raw_bit_size);
    psize->checksum = seds_update_checksum_int(psize->checksum, multiplier);

    return 0;
}

/**
 * Lua-callable function to "salt" a checksum hash in different ways
 *
 * The checksum is modified based on the passed-in value, such that the
 * same inputs will always produce the same result, but a different
 * input produces a very different result.
 *
 * This is used to incorporate additional metadata into a datatype
 * checksum calculation, which wouldn't otherwise affect the checksum.
 */
static int seds_memreq_flavor(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");

    switch (lua_type(lua, 2))
    {
    case LUA_TSTRING:
        psize->checksum += 2;
        psize->checksum = seds_update_checksum_string(psize->checksum, lua_tostring(lua, 2));
        break;
    case LUA_TNUMBER:
        psize->checksum += 3;
        psize->checksum = seds_update_checksum_int(psize->checksum, lua_tointeger(lua, 2));
        break;
    case LUA_TBOOLEAN:
        psize->checksum += 4;
        psize->checksum = seds_update_checksum_int(psize->checksum, lua_toboolean(lua, 2));
        break;
    default:
        /*
         * Causes every pass through this routine to modify the checksum in some way
         */
        ++psize->checksum;
        break;
    }

    return 0;
}

/**
 * Lua-callable function to set the packing style of the object
 *
 * This is used to incorporate additional metadata into a datatype
 * checksum calculation, which wouldn't otherwise affect the checksum.
 */
static int seds_memreq_setpack(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");
    seds_bytepack_status_t packstat;

    if (lua_isnoneornil(lua, 2))
    {
        packstat = SEDS_BYTEPACK_STATUS_UNDEFINED;
    }
    else if (lua_type(lua, 2) != LUA_TSTRING)
    {
        packstat = SEDS_BYTEPACK_STATUS_OTHER;
    }
    else if (strcmp(lua_tostring(lua, 2), "LE") == 0)
    {
        packstat = SEDS_BYTEPACK_STATUS_LITTLEENDIAN_STYLE;
    }
    else if (strcmp(lua_tostring(lua, 2), "BE") == 0)
    {
        packstat = SEDS_BYTEPACK_STATUS_BIGENDIAN_STYLE;
    }
    else
    {
        packstat = SEDS_BYTEPACK_STATUS_OTHER;
    }

    /* if packing status was already set _differently_ then
     * set it to OTHER (for mixed values) */
    if (psize->packing_status != SEDS_BYTEPACK_STATUS_UNDEFINED &&
            packstat != SEDS_BYTEPACK_STATUS_UNDEFINED &&
            psize->packing_status != packstat)
    {
        packstat = SEDS_BYTEPACK_STATUS_OTHER;
    }

    if (packstat != SEDS_BYTEPACK_STATUS_UNDEFINED &&
            psize->packing_status != packstat)
    {
        psize->packing_status = packstat;
        psize->checksum = seds_update_checksum_int(psize->checksum, 100 + packstat);
    }

    return 0;
}

/**
 * Lua callable function to add a "pad" to an object size
 *
 * The object is padded by the specified number of bits.  This does not affect
 * the byte size of an object, since EDS-specified padding is not represented
 * in C data structures at all.  However, it will change the checksum.
 */
static int seds_memreq_pad(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");
    seds_integer_t pad_bits = luaL_checkinteger(lua, 2);

    psize->checksum += 5;
    psize->checksum = seds_update_checksum_int(psize->checksum, pad_bits);
    psize->raw_bit_size += pad_bits;

    /*
     * Padded structures are probably not optimally packed.
     * TBD - this may not be true, since the padding could be accounting
     * for padding that the compiler would have to add.
     */
    psize->packing_status = SEDS_BYTEPACK_STATUS_OTHER;
    return 0;
}

/**
 * Lua callable function to get a memreq object property
 *
 * Expected Stack args:
 *  1: tree node object
 *  2: property key
 *
 * Unlike other types of objects these do not support directly setting
 * any property.  The modifications occur through method calls only.
 */
static int seds_memreq_get_property(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");

    /*
     * Handle some fixed key names, but only if the key is a string
     */
    if (lua_isstring(lua, 2))
    {
        if (strcmp(lua_tostring(lua, 2), "bits") == 0)
        {
            lua_pushinteger(lua, psize->raw_bit_size);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "bytes") == 0)
        {
            lua_pushinteger(lua, psize->local_storage_bytes);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "alignment") == 0)
        {
            lua_pushinteger(lua, 8 * (1 + psize->local_align_mask));
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "is_packed") == 0)
        {
            if ((8 * psize->local_storage_bytes) != psize->raw_bit_size)
            {
                lua_pushnil(lua);
            }
            else if (psize->packing_status == SEDS_BYTEPACK_STATUS_LITTLEENDIAN_STYLE)
            {
                lua_pushstring(lua, "LE");
            }
            else if (psize->packing_status == SEDS_BYTEPACK_STATUS_BIGENDIAN_STYLE)
            {
                lua_pushstring(lua, "BE");
            }
            else
            {
                lua_pushnil(lua);
            }

            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "checksum") == 0)
        {
            /*
             * Because the checksums are 64 bit they might fit in a lua number type (which is double, even for ints)
             * Therefore the checksums are returned as a hex string
             */
            char stringbuf[20];
            snprintf(stringbuf,sizeof(stringbuf),"%016lx", (unsigned long)psize->checksum);
            lua_pushstring(lua, stringbuf);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "add") == 0)
        {
            lua_pushcfunction(lua, seds_memreq_add);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "union") == 0)
        {
            lua_pushcfunction(lua, seds_memreq_union);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "multiply") == 0)
        {
            lua_pushcfunction(lua, seds_memreq_multiply);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "flavor") == 0)
        {
            lua_pushcfunction(lua, seds_memreq_flavor);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "setpack") == 0)
        {
            lua_pushcfunction(lua, seds_memreq_setpack);
            return 1;
        }

        if (strcmp(lua_tostring(lua, 2), "pad") == 0)
        {
            lua_pushcfunction(lua, seds_memreq_pad);
            return 1;
        }
    }

    return 0;
}

/**
 * Lua callable function to convert a memreq object to a string
 *
 * This implements the "tostring" method for memreq objects, and it
 * returns a summary string containing the bits, bytes, alignment,
 * and checksum of the object binary format.
 */
static int seds_memreq_to_string(lua_State *lua)
{
    seds_memreq_t *psize = luaL_checkudata(lua, 1, "seds_memreq");
    char stringbuf[128];

    snprintf(stringbuf, sizeof(stringbuf), "bits=%4ld  bytes=%4ld/%-4ld  align=0x%lx  checksum=%016llx",
            (long)psize->raw_bit_size,
            (long)psize->endpoint_bytes,
            (long)psize->local_storage_bytes,
            (unsigned long)psize->local_align_mask,
            (unsigned long long)psize->checksum);

    lua_pushstring(lua, stringbuf);
    return 1;
}

/**
 * Lua-callable function to create a new memreq userdata object
 *
 * In default mode (no parameters) this pushes an empty memreq object with the
 * raw bit size set to zero.
 *
 * Optional input Lua stack:
 *      1: Initial bit size of the object
 *      2:
 */
int seds_memreq_new_object(lua_State *lua)
{
    seds_memreq_t *psize = lua_newuserdata(lua, sizeof(seds_memreq_t));
    memset(psize, 0, sizeof(*psize));
    if (luaL_newmetatable(lua, "seds_memreq"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_memreq_get_property);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, seds_memreq_to_string);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    psize->checksum = SEDS_CHECKSUM_INITIAL;

    if (lua_isnumber(lua, 1))
    {
        psize->raw_bit_size = lua_tointeger(lua, 1);
        psize->endpoint_bytes = (psize->raw_bit_size + 7) / 8;

        if (lua_isnumber(lua, 2))
        {
            psize->local_align_mask = lua_tointeger(lua, 2);
            if (psize->local_align_mask <= 0)
            {
                psize->local_align_mask = 0;
            }
            else if (psize->local_align_mask >= 15)
            {
                psize->local_align_mask = 15;
            }
            else
            {
                --psize->local_align_mask;
                psize->local_align_mask |= psize->local_align_mask >> 1;
                psize->local_align_mask |= psize->local_align_mask >> 2;
            }
        }
        else
        {
            /*
             * Calculate the initial alignment mask and storage based on the supplied size
             */
            psize->local_align_mask = 1;
            while (psize->local_align_mask < 16 &&
                    psize->local_align_mask < psize->endpoint_bytes)
            {
                psize->local_align_mask <<= 1;
            }
            --psize->local_align_mask;
        }

        psize->local_storage_bytes = (psize->endpoint_bytes + psize->local_align_mask) & ~psize->local_align_mask;
        psize->checksum = seds_update_checksum_int(psize->checksum, psize->raw_bit_size);
    }
    else if (lua_isuserdata(lua, 1))
    {
        if (luaL_testudata(lua, 1, "seds_node") != NULL)
        {
            lua_getfield(lua, 1, "resolved_size");
            lua_replace(lua, 1);
        }
        seds_memreq_t *pbasemem = luaL_testudata(lua, 1, "seds_memreq");
        if (pbasemem != NULL)
        {
            *psize = *pbasemem;
        }
    }

    if (psize->endpoint_bytes == 0)
    {
        /*
         * Note all entities must occupy at least 1 byte of storage in the C/C++ world, such that
         * an array of such entities will have a different/unique memory address for each index.
         */
        psize->local_storage_bytes = 1 + psize->local_align_mask;
    }

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
void seds_memreq_register_globals(lua_State *lua)
{
    luaL_checktype(lua, -1, LUA_TTABLE);
    lua_pushcfunction(lua, seds_memreq_new_object);
    lua_setfield(lua, -2, "new_size_object");

}
