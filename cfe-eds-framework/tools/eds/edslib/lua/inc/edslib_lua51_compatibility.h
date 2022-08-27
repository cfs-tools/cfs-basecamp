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
 * \file     edslib_lua51_compatibility.h
 * \ingroup  lua
 * \author   joseph.p.hickey@nasa.gov
 * 
 * Compatibility shim to support Lua 5.1, which despite being EOL
 * for some time, is still distributed in some Enterprise Linux
 * distributions.
 */

#ifndef _EDSLIB_LUA51_COMPATIBILITY_H_
#define _EDSLIB_LUA51_COMPATIBILITY_H_


#include <stdlib.h>
#include <string.h>

#ifndef LUA_VERSION_NUM
#error "This file must be included after lua.h"
#endif

#if (LUA_VERSION_NUM <= 501)

enum
{
    LUA_OK = 0,
    LUA_OPADD,      /**< performs addition (+) */
    LUA_OPSUB,      /**< performs subtraction (-) */
    LUA_OPMUL,      /**< performs multiplication (*) */
    LUA_OPDIV,      /**< performs division (/) */
    LUA_OPMOD,      /**< performs modulo (%) */
    LUA_OPPOW,      /**< performs exponentiation (^) */
    LUA_OPUNM,      /**< performs mathematical negation (unary -) */
    LUA_OPEQ,       /**< compares for equality (==) */
    LUA_OPLT,       /**< compares for less than (<) */
    LUA_OPLE       /**< compares for less or equal (<=) */
};


/* use lua_objlen in place of lua_rawlen() function.
 * this differs in metamethod invocation, so its similar enough
 * for the use case here */
static inline int lua_rawlen(lua_State *lua, int n)
{
    return lua_objlen(lua, n);
}

/* provide an approximation for the absindex routine.
 * this will convert a relative index between -100 and -1
 * to its positive/absolute equivalent */
static inline int lua_absindex(lua_State *lua, int n)
{
    if (n > -100 && n < 0)
    {
        n = 1 + lua_gettop(lua) + n;
    }
    return n;
}

/* lua5.1 does not have an API to check the type of a udata object.
 * using checkudata will generally work, but it will throw an exception
 * if the type does not match, so it means some code will behave differently*/
static inline void *luaL_testudata (lua_State *lua, int n, const char *tname)
{
    void *r;
    n = lua_absindex(lua, n);
    lua_getmetatable(lua, n);
    luaL_getmetatable(lua, tname);
    if (lua_equal(lua, -2, -1))
    {
        r = luaL_checkudata(lua, n, tname);
    }
    else
    {
        r = NULL;
    }
    lua_pop(lua, 2);
    return r;
}


static inline void lua_rawsetp (lua_State *lua, int n, const void *p)
{
    n = lua_absindex(lua, n);
    lua_pushlightuserdata(lua, (void*)p);
    lua_insert(lua, -2);
    lua_rawset(lua, n);
}

static inline void lua_rawgetp (lua_State *lua, int n, const void *p)
{
    lua_pushlightuserdata(lua, (void*)p);
    lua_rawget(lua, n);
}

/* the "uservalue" feature in lua5.2 and up allows one to associate an
 * arbitrary table with a userdata object.  This mimics that feature
 * using the Lua registry.  It might be enough to get by but it
 * will not be correctly garbage collected as expected. */
static inline void lua_getuservalue(lua_State *lua, int n)
{
    /* get value associated with userdata at n */
    lua_getfenv(lua, n);
}

static inline void lua_setuservalue(lua_State *lua, int n)
{
    /* pop top entry off and associate it with userdata at n */
    lua_setfenv(lua, n);
}

static inline const char *luaL_tolstring(lua_State *lua, int n, size_t *sz)
{
    /* this uses the basic tolstring function, which does not
     * invoke the __tostring metamethod.  So the output might be
     * incorrect for userdata objects.  */
    lua_pushvalue(lua, n);
    return lua_tolstring(lua, -1, sz);
}

static inline int luaL_getsubtable (lua_State *lua, int n, const char *fname)
{
    int rval;
    n = lua_absindex(lua, n);
    lua_pushstring(lua, fname);
    lua_rawget(lua, n);
    rval = lua_istable(lua, -1);
    if (!rval)
    {
        lua_pop(lua, 1);
        lua_newtable(lua);
        lua_pushstring(lua, fname);
        lua_pushvalue(lua, -2);
        lua_rawset(lua, n);
    }
    return rval;
}

static inline void lua_arith (lua_State *lua, int op)
{
    lua_Integer n1;
    lua_Integer n2;

    n1 = lua_tointeger(lua, -1);

    /* everything except negation requires 2 operands */
    if (op != LUA_OPUNM)
    {
        n2 = lua_tointeger(lua, -2);
    }
    else
    {
        n2 = 0;
    }

    /* note lua5.2+ defines the second operand as the one
     * on top of the stack (n1 here) */
    switch(op)
    {
    case LUA_OPADD:
        n2 += n1;
        break;
    case LUA_OPSUB:
        n2 -= n1;
        break;
    case LUA_OPMUL:
        n2 *= n1;
        break;
    case LUA_OPDIV:
        n2 /= n1;
        break;
    case LUA_OPMOD:
        n2 %= n1;
        break;
    case LUA_OPUNM:
        n2 = -n1;
        break;

    default:
        abort(); /* not implemented */
        break;
    }

    lua_pushinteger(lua, n2);
}

static inline int lua_compare (lua_State *lua, int index1, int index2, int op)
{
    int b;

    switch(op)
    {
    case LUA_OPEQ:
        b = lua_equal(lua, index1, index2);
        break;
    case LUA_OPLT:
        b = lua_lessthan(lua, index1, index2);
        break;
    case LUA_OPLE:
        b = lua_lessthan(lua, index1, index2);
        if (b == 0)
        {
            b = lua_equal(lua, index1, index2);
        }
        break;
    default:
        b = 0;
        abort(); /* not implemented */
        break;
    }
    return b;
}

#endif


#endif  /* _EDSLIB_LUA51_COMPATIBILITY_H_ */

