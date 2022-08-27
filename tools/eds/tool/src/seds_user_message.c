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
 * \file     seds_user_message.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implements several functions to assist with creating console
 * output to the user terminal.  Using these functions allows
 * context information to be attached in a consistent way, and
 * aids the operator should things go wrong.
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/types.h>

#include "seds_user_message.h"


static seds_integer_t seds_global_message_counts[SEDS_USER_MESSAGE_MAX];


/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/

/**
 * Lua-callable function to get the counts associated with a given message type.
 *
 * Input argument (from Lua):
 *   upvalue 1: Type of message (integer msgtype)
 */
static int seds_get_message_count(lua_State *lua)
{
    lua_pushinteger(lua, seds_global_message_counts[lua_tointeger(lua, lua_upvalueindex(1))]);
    return 1;
}

/**
 * Lua-callable error message handler
 *
 * This attaches file/line context information from the Lua interpreter
 * to the error message at the top of the stack, and passes it to
 * seds_user_message_impl
 *
 * Input arguments (from Lua):
 *   upvalue 1: Type of message to send (integer msgtype)
 *   stack 1: Error message (string)
 */
static int seds_lua_user_message(lua_State *lua)
{
    const char *full_message = luaL_checkstring(lua, 1);
    const char *filename;
    lua_Debug ar;

    memset(&ar, 0, sizeof(ar));
    lua_getstack(lua, 1, &ar);
    lua_getinfo(lua, "Sl", &ar);

    filename = ar.source;
    if (filename != NULL && *filename == '@')
    {
        ++filename;
    }
    seds_user_message_preformat(lua_tointeger(lua, lua_upvalueindex(1)),
            filename, ar.currentline, full_message, NULL);

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
void seds_user_message_preformat(seds_user_message_t msgtype, const char *file, unsigned long line, const char *message1, const char *message2)
{
    const char *msgtag;

    switch(msgtype)
    {
    case SEDS_USER_MESSAGE_INFO:
        msgtag = "info";
        break;
    case SEDS_USER_MESSAGE_WARNING:
        msgtag = "warning";
        break;
    case SEDS_USER_MESSAGE_ERROR:
        msgtag = "error";
        break;
    case SEDS_USER_MESSAGE_FATAL:
        msgtag = "fatal";
        break;
    case SEDS_USER_MESSAGE_DEBUG:
        msgtag = "debug";
        break;
    default:
        msgtag = NULL;
        break;
    }

    if (msgtag == NULL)
    {
        return;
    }

    ++seds_global_message_counts[msgtype];

    if (((seds_integer_t)msgtype) < (SEDS_USER_MESSAGE_WARNING - sedstool.verbosity))
    {
        /* do not show message */
        return;
    }

    if (file != NULL)
    {
        fprintf(stderr, "%s:%lu:%s: ", file, line, msgtag);
    }

    fprintf(stderr, "%s", message1);
    if (message2 != NULL)
    {
        fprintf(stderr, " - %s", message2);
    }
    fprintf(stderr, "\n");

    /*
     * In case of a FATAL message, it is deemed not worthy to continue.
     * call abort() which should terminate this process and drop
     * a core file for debugging, if enabled.
     */
    if (msgtype == SEDS_USER_MESSAGE_FATAL)
    {
        abort();
    }
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_user_message_printf(seds_user_message_t msgtype, const char *file, unsigned long line, const char *format, ...)
{
    char full_message[256];
    va_list va;

    va_start(va, format);
    vsnprintf(full_message, sizeof(full_message), format, va);
    va_end(va);

    seds_user_message_preformat(msgtype, file, line, full_message, NULL);
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_user_message_errno(seds_user_message_t msgtype, const char *file, unsigned long line, const char *message)
{
    seds_user_message_preformat(msgtype, file, line, message, strerror(errno));
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
seds_integer_t seds_user_message_get_count(seds_user_message_t msgtype)
{
    seds_integer_t result;

    if (msgtype < SEDS_USER_MESSAGE_MAX)
    {
        result = seds_global_message_counts[msgtype];
    }
    else
    {
        result = 0;
    }

    return result;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_user_message_register_globals(lua_State *lua)
{
    lua_pushinteger(lua, SEDS_USER_MESSAGE_DEBUG);
    lua_pushcclosure(lua, seds_lua_user_message, 1);
    lua_setfield(lua, 1, "debug");

    lua_pushinteger(lua, SEDS_USER_MESSAGE_INFO);
    lua_pushcclosure(lua, seds_lua_user_message, 1);
    lua_setfield(lua, 1, "info");

    lua_pushinteger(lua, SEDS_USER_MESSAGE_WARNING);
    lua_pushcclosure(lua, seds_lua_user_message, 1);
    lua_setfield(lua, 1, "warning");

    lua_pushinteger(lua, SEDS_USER_MESSAGE_ERROR);
    lua_pushcclosure(lua, seds_lua_user_message, 1);
    lua_setfield(lua, 1, "error");

    lua_pushinteger(lua, SEDS_USER_MESSAGE_FATAL);
    lua_pushcclosure(lua, seds_lua_user_message, 1);
    lua_setfield(lua, 1, "fatal");

    lua_pushinteger(lua, SEDS_USER_MESSAGE_ERROR);
    lua_pushcclosure(lua, seds_get_message_count, 1);
    lua_setfield(lua, 1, "get_error_count");

    lua_pushinteger(lua, SEDS_USER_MESSAGE_WARNING);
    lua_pushcclosure(lua, seds_get_message_count, 1);
    lua_setfield(lua, 1, "get_warning_count");
}
