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
 * \file     seds_preprocess.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Functions related to the EDS preprocessing logic.
 *
 * This module is responsible for translating the "${SYMBOL}" syntax that
 * may appear anywhere in an EDS file, and translating this to the actual
 * value.
 *
 * It does this by searching for the ${} syntax, and translating the content
 * into a chunk of Lua code for evaluation.  By using the Lua interpreter to do
 * this, it can handle basic arithmetic operations as well, which is a common
 * need.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "seds_global.h"
#include "seds_preprocess.h"

static const char SEDS_PREPROCESS_ENVIORNMENT;

/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/

/* ------------------------------------------------------------------- */
/**
 * Helper function to determine if a quoting is required, when converting a bare string
 * into a string that can be evaulated by Lua.
 *
 * For instance, it is desirable in a string such as "4+1" to _not_ add extra quoting,
 * so the interpreter will see those as numbers and not as strings, and therefore produce
 * the result 5.  However quoting is required in cases such as "hello .. world" where
 * the bare words would be evaluated as Lua globals, and hence be "nil".  In this case
 * it is necessary to make sure they are seen as string literals.
 *
 * There are some assumptions made here, so it is not perfect, but the common use cases
 * should all work.
 *
 * This allows for a specific set of operators to be used in expressions.  If anything
 * OUTSIDE this set is found, then the value is interpreted as a literal string, not
 * an expression or a numeric literal.
 *
 * The expression is passed to the embedded LUA state for actual evaluation, the only
 * difference is that if "string mode" is used then the value is wrapped in quotes
 * rather than being passed as a bare token, so LUA will see it as a string.
 *
 * The set of tokens that this routine recognizes and will pass through:
 *  [0-9]            -- decimal literals
 *  0x[0-9a-f]       -- hexadecimal literals (must have "0x" at beginning)
 *  true false       -- boolean literals
 *  ( )              -- arithmetic/boolean grouping
 *  + - * /          -- arithmetic operators
 *  == < > <= >= !=  -- boolean operators
 *  and or not       -- logical ops
 *  ${any-string}    -- SEDS reference
 *
 *
 * @note the code below is _NOT_ trying to actually interpret the
 * string.  LUA does that.  This code only needs to decide whether or not it needs
 * to be quoted when passing to LUA.
 */
static seds_boolean_t seds_resolve_is_lua_quoting_required(const char *p)
{
    seds_boolean_t quoting_required;
    uint32_t token_size;
    enum TokenType
    {
        TOKENTYPE_UNKNOWN,
        TOKENTYPE_BLANK,
        TOKENTYPE_VALUE,
        TOKENTYPE_OPERATOR,
        TOKENTYPE_MAX
    } curr_token, prev_token;


    quoting_required = false;
    curr_token = prev_token = TOKENTYPE_UNKNOWN;
    token_size = 0;
    while (*p != 0)
    {
        if (strncasecmp(p, "false", 5) == 0)
        {
            /* boolean literal */
            token_size = 5;
            curr_token = TOKENTYPE_VALUE;
        }
        else if (strncasecmp(p, "true", 4) == 0)
        {
            /* boolean literal */
            token_size = 4;
            curr_token = TOKENTYPE_VALUE;
        }
        else if (strncasecmp(p, "and", 3) == 0)
        {
            /* boolean and */
            token_size = 3;
            curr_token = TOKENTYPE_OPERATOR;
        }
        else if (strncasecmp(p, "and", 3) == 0)
        {
            /* boolean and */
            token_size = 3;
            curr_token = TOKENTYPE_OPERATOR;
        }
        else if (strncasecmp(p, "not", 3) == 0)
        {
            /* boolean not */
            token_size = 3;
            curr_token = TOKENTYPE_OPERATOR;
        }
        else if (strncasecmp(p, "or", 2) == 0)
        {
            /* boolean or */
            token_size = 2;
            curr_token = TOKENTYPE_OPERATOR;
        }
        else if (strncmp(p, "==", 2) == 0 || strncmp(p, "~=", 2) == 0 ||
                strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0)
        {
            /* relational operators */
            token_size = 2;
            curr_token = TOKENTYPE_OPERATOR;
        }
        else if (strncmp(p, "${", 2) == 0)
        {
            token_size = 2;
            while (p[token_size] != '}' && p[token_size] != 0) ++token_size;
            if (p[token_size] == '}')
            {
                /* complete reference -- include the terminator itself */
                ++token_size;
                curr_token = TOKENTYPE_VALUE;
            }
            else
            {
                /* Not a properly terminated reference */
                token_size = 0;
                curr_token = TOKENTYPE_UNKNOWN;
            }
        }
        else if (isblank(*p))
        {
            token_size = 1;
            while (isblank(p[token_size])) ++token_size;
            curr_token = TOKENTYPE_BLANK;
        }
        else if (strncmp(p, "0x", 2) == 0)
        {
            /* Hex literals start with 0x and only contain hex digits */
            token_size = 2;
            while (isxdigit(p[token_size])) ++token_size;
            if (token_size <= 2)
            {
                /* if not followed by actual hex digits, it is not a valid literal */
                curr_token = TOKENTYPE_UNKNOWN;
                token_size = 0;
            }
            else
            {
                curr_token = TOKENTYPE_VALUE;
            }
        }
        else if (isdigit(*p))
        {
            /* Decimal literals may contain decimal points and/or may be in scientific notation */
            token_size = 1;
            while (isdigit(p[token_size]) || p[token_size] == '.') ++token_size;
            if (p[token_size] == 'e')
            {
                if (isdigit(p[token_size + 1]))
                {
                    token_size += 2;
                }
                else if ((p[token_size + 1] == '-' || p[token_size + 1] == '+') && isdigit(p[token_size + 2]))
                {
                    token_size += 3;
                }

                while (isdigit(p[token_size])) ++token_size;
            }
            curr_token = TOKENTYPE_VALUE;
        }
        else if (*p == '(' || *p == ')' || *p == '+' || *p == '-' || *p == '/' || *p == '*' || *p == '^')
        {
            /* tokens allowed in expressions */
            token_size = 1;
            curr_token = TOKENTYPE_OPERATOR;
        }
        else if (*p == '<' || *p == '>')
        {
            /* relational operators */
            token_size = 1;
            curr_token = TOKENTYPE_OPERATOR;
        }
        else
        {
            curr_token = TOKENTYPE_UNKNOWN;
            token_size = 0;
        }

        /*
         * If the token is not in the allowed set, then consider
         * this value to be a string and enable quoting for passing to LUA
         *
         * Two values next to each other with no operator between them also
         * is an indicator that this should be a string
         */
        if (token_size == 0 || curr_token == TOKENTYPE_UNKNOWN ||
                (prev_token == TOKENTYPE_VALUE && curr_token == TOKENTYPE_VALUE))
        {
            quoting_required = true;
            break;
        }

        if (curr_token != TOKENTYPE_BLANK)
        {
            prev_token = curr_token;
        }

        p += token_size;
    }

    return quoting_required;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable function to convert an arbitrary string into a Lua
 * code chunk for evaluation.
 *
 * Any literals are quoted according to the seds_resolve_is_lua_quoting_required()
 * logic, and the ${} replacement syntax is converted to a call to a global function
 * called "Evaluate".
 *
 * This does not load the chunk, it returns the chunk as a string for the caller
 * to load, as the caller may want to set a specific environment for execution.
 */
static int seds_resolve_luafy_seds_reference(lua_State *lua)
{
    luaL_Buffer buffer;
    const char *input;
    const char *p, *q;
    seds_boolean_t quoting_required;

    luaL_buffinit(lua, &buffer);
    luaL_addstring(&buffer, "return ");
    input = lua_tostring(lua, -1);
    quoting_required = seds_resolve_is_lua_quoting_required(input);

    if (quoting_required)
    {
        /*
         * If quoting is required, add opening quote now
         */
        luaL_addchar(&buffer, '"');
    }
    else if (*input == 0)
    {
        /*
         * In case an EMPTY value was supplied, use nil
         */
        luaL_addstring(&buffer, "nil");
    }

    /*
     * Locate any ${REF} sequences within the string;
     * this needs to be tweaked into a valid lua evaluation call syntax i.e. Evaluate()
     * (unfortunately lua does not allow a function to be named '$' which would have been great if it did)
     *
     * Note that if passed directly to LUA the {} syntax would be a table constructor; this would almost
     * work except there would be some ambiguity if used within a compound statement.  Therefore to avoid
     * syntactical errors in the generate strings this is replaced with the () syntax.
     */
    p = q = input;
    while (*input != 0)
    {
        if (*input == '$')
        {
            /*
             * Note that this allows a double dollarsign ($$) to be used in case
             * the user actually wants to have a dollarsign and not evaluate the contents
             */
            ++input;
            p = input;
            if (*p == '{')
            {
                q = p + 1;
                while (*q != '}' && *q != 0) ++q;
            }
        }

        if (q > p && *p == '{' && *q == '}')
        {
            ++p;
            /*
             * Note that LUA syntax cannot evaluate references within strings like
             * perl or bash or other script languages can do. So, if quoting is enabled,
             * the string must be closed, concatenation operators added, and a new
             * string opened after the call.
             */
            if (quoting_required)
            {
                /* Close the previous string and add concatenation operator */
                luaL_addstring(&buffer, "\" .. ");
            }

            /*
             * This also passes a stringified (quoted) version of the value.
             * This is because if the value evaluates to "nil" or is otherwise bad, the string version
             * is used to generate a more user-friendly error message for debugging.
             */
            luaL_addstring(&buffer, "Evaluate(\"");
            while (p < q)
            {
                if (isprint(*p))
                {
                    if (*p == '"')
                    {
                        luaL_addchar(&buffer, '\\');
                    }
                    luaL_addchar(&buffer, *p);
                }
                ++p;
            }
            luaL_addstring(&buffer, "\")");
            input = q;

            if (quoting_required)
            {
                /* Add concatenation operator and open a new string */
                luaL_addstring(&buffer, " .. \"");
            }

        }
        else if (isprint(*input))
        {
            /*
             * Regardless of quoting or not,
             * only regular (printable) characters will be passed to LUA
             */

            /*
             * Add necessary quoting if it was indicated by the previous check
             */
            if (quoting_required)
            {
                /*
                 * Escape any embedded quote characters
                 */
                if (*input == '"' || *input == '\\')
                {
                    luaL_addchar(&buffer, '\\');
                }

                /*
                 * Add the input character verbatim
                 */
                luaL_addchar(&buffer, *input);
            }
            else
            {
                /*
                 * If quoting is NOT required, then convert any alpha
                 * characters to lowercase.  LUA is case sensitive so it
                 * only recognizes e.g. true,false but not TRUE or FALSE.
                 */
                luaL_addchar(&buffer, tolower((int)*input));
            }
        }

        ++input;
    }

    if (quoting_required)
    {
        /* Add final closing quote */
        luaL_addchar(&buffer, '"');
    }
    luaL_pushresult(&buffer);

    return 1;
}


#if (LUA_VERSION_NUM <= 501)

/* in lua <= 5.1, the environment is set after the load */
static int seds_resolve_load_references(lua_State *lua)
{
    seds_resolve_luafy_seds_reference(lua);

    lua_pushnil(lua);
    if (luaL_loadstring(lua, lua_tostring(lua, -2)) == LUA_OK)
    {
        lua_rawgetp(lua, LUA_REGISTRYINDEX, &SEDS_PREPROCESS_ENVIORNMENT);
        lua_setfenv(lua, -2);
        lua_pushnil(lua);
    }
    return 2;
}

#else

/* in lua >= 5.2, the environment is set prior to load via LUA_RIDX_GLOBALS */
static int seds_resolve_load_references(lua_State *lua)
{
    int defenv;
    seds_resolve_luafy_seds_reference(lua);

    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    defenv = lua_gettop(lua);
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &SEDS_PREPROCESS_ENVIORNMENT);
    lua_rawseti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);

    lua_pushnil(lua);
    if (luaL_loadstring(lua, lua_tostring(lua, 2)) == LUA_OK)
    {
        lua_pushnil(lua);
    }

    /* restore the previous LUA_RIDX_GLOBALS */
    lua_pushvalue(lua, defenv);
    lua_rawseti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);

    return 2;
}

#endif


static int seds_resolve_set_eval_func(lua_State *lua)
{
    lua_settop(lua, 1);
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &SEDS_PREPROCESS_ENVIORNMENT);
    lua_insert(lua, 1);
    lua_setfield(lua, 1, "Evaluate");
    return 0;
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
void seds_preprocess_register_globals(lua_State *lua)
{
    luaL_checktype(lua, 1, LUA_TTABLE);

    lua_pushcfunction(lua, seds_resolve_load_references);
    lua_setfield(lua, 1, "load_refs");

    lua_pushcfunction(lua, seds_resolve_set_eval_func);
    lua_setfield(lua, 1, "set_eval_func");

    lua_newtable(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &SEDS_PREPROCESS_ENVIORNMENT);
}
