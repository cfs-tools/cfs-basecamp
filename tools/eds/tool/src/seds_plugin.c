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
 * \file     seds_plugin.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of the SEDS plug-in module loader.
 *
 * This loads a dynamic shared object (.so) file into the running sedstool instance,
 * which can in turn be used by scripts.  Facilities are provided to call functions
 * contained within the shared object.
 *
 * Most of the real tool functionality is actually _not_ implemented within the C
 * source files included in this tool.  The C source files only provide the basic
 * runtime environment.
 *
 * The real logic is implemented in supplemental runtime scripts.  These are either
 * implemented in Lua directory, or they are implemented in C and compiled to a shared
 * object (.so) file and subsequently loaded.
 *
 * To extend this even further, a Lua script may generate output code, call "make" on
 * the code to build a .so file, which in turn can be loaded into the running process.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "seds_plugin.h"
#include "seds_user_message.h"

#include "edslib_lua_objects.h"

/**
 * Plugin userdata object definition.
 * This in turn is a reference to the handle returned by dlopen()
 */
typedef struct
{
    void *dlhandle;         /**< actual handle from dlopen() */
    seds_integer_t refcount;
} seds_c_plugin_t;


/**
 * An object to simplify dynamically loaded symbol lookups
 *
 * Simplifies casting/conversion between the lookup type (void*)
 * and the actual symbol type.
 */
typedef union
{
    void *symaddr;                  /**< symbol address from dlsym() */
    lua_CFunction lua_func;         /**< address as a Lua function */
    void (*simple_func)(void);
    void (*singlearg_func)(void *arg);
    void (*dualarg_func)(void *arg1, void *arg2);
} seds_c_symbol_t;

/* slight cheat here - default the path to the same place as the C source.
 * if not overridden, then this is passed to dirname() to get the default. */
static char SEDS_DEFAULT_SOURCE_PATH[] = __FILE__;
static const char SEDS_PLUGIN_TABLE_KEY;            /**< Key for Lua registry table containing plugin objects */

/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/



/* ------------------------------------------------------------------- */
/**
 * Convert a value on the stack into a C pointer object.
 *
 * This is intended for passing into a C function that accepts a pointer.
 *
 * Because Lua and C do not share the same type model, this is not a perfect conversion.
 * The calling convention used here passes everything as a void*, regardless of what its
 * real type is.
 *
 * For userdata it is assumed that it will be an "EdsLib_Object" which is in fact a C object,
 * so that works fine.  Light user data objects are just passed directly as an opaque pointer.
 *
 * Strings are OK as well, since Lua already can convert those to C strings.  Just cannot
 * pop these off the stack before making the C call.
 *
 * For numbers it is assumed these will be integers.  These are passed as a void* value.
 *
 */
static void *lua_value_to_cptr(lua_State *lua, int narg)
{
    union
    {
        void *vptr;
        const char *vstr;   /* For strings */
        ptrdiff_t vint;     /* "ptrdiff_t" should be a signed integer type with the same width as void* */
    } value;

    switch(lua_type(lua,narg))
    {
    case LUA_TBOOLEAN:
        value.vint = lua_toboolean(lua, narg);
        break;
    case LUA_TNUMBER:
        value.vint = lua_tointeger(lua, narg);
        break;
    case LUA_TUSERDATA:
    {
        EdsLib_LuaBinding_GetNativeObject(lua, narg, &value.vptr, NULL);
        break;
    }
    case LUA_TLIGHTUSERDATA:
        value.vptr = lua_touserdata(lua, narg);
        break;
    case LUA_TSTRING:
        value.vstr = lua_tostring(lua, narg);
        break;
    case LUA_TTABLE:
    case LUA_TNIL:
    case LUA_TFUNCTION:
    default:
        value.vptr = NULL;
        break;
    }

    return value.vptr;
}

/* ------------------------------------------------------------------- */
/**
 * Call a C function directly from Lua
 *
 * Note that this is _not_ a Lua CFunction being called, it is a plain C function.
 * To keep this sane, only 3 possible prototypes are allowed:
 * - No arguments (void)
 * - One argument (void*)
 * - Two arguments (void*, void*)
 *
 * The prototype is assumed based on the number of parameters passed in from the Lua
 * environment.  The lua_value_to_cptr() function is used to convert the Lua value
 * to a pointer value for passing.
 *
 * Note this has no way to check the prototype of the C function and verify if the
 * arguments are correct.  It will simply believe what the Lua script passed in.
 * If it is incorrect, results may be undefined.  In general it will likely still work
 * if too many args were passed, at least on systems were the caller does the push/pop
 * stack operations.  But if too few arguments are passed, the C function will see
 * an undefined value for the last arguments and may crash.
 */
static int seds_c_symbol_call(lua_State *lua)
{
    seds_c_symbol_t *c_sym = luaL_checkudata(lua, 1, "c_symbol");
    int stack_top = lua_gettop(lua);

    if (stack_top == 1)
    {
        c_sym->simple_func();
    }
    else if (stack_top == 2)
    {
        c_sym->singlearg_func(lua_value_to_cptr(lua, 2));
    }
    else
    {
        c_sym->dualarg_func(lua_value_to_cptr(lua, 2),
                lua_value_to_cptr(lua, 3));
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Helper function to Create a C symbol object.
 *
 * A C symbol object is just a Lua userdata that implements
 * a "function call" meta function, allowing the function to be
 * called from Lua.
 */
static seds_c_symbol_t *seds_new_symbol_obj(lua_State *lua)
{
    seds_c_symbol_t *c_sym = lua_newuserdata(lua, sizeof(seds_c_symbol_t));
    SEDS_ASSERT(c_sym != NULL, "failed to allocate userdata");
    if (luaL_newmetatable(lua, "c_symbol"))
    {
        lua_pushstring(lua, "__call");
        lua_pushcfunction(lua, seds_c_symbol_call);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    return c_sym;
}

/* ------------------------------------------------------------------- */
/**
 * Helper function to look up a C symbol within a shared object.
 *
 * This is basically a Lua wrapper around the dlsym() library call.
 * If the symbol is valid it can then be called from Lua.
 */
static int seds_c_plugin_get_symbol(lua_State *lua)
{
    seds_c_plugin_t *plugin = luaL_checkudata(lua, 1, "c_plugin");
    const char *tmpstr;
    void *sym;

    if (lua_type(lua, 2) != LUA_TSTRING)
    {
        return 0;
    }

    dlerror();
    sym = dlsym(plugin->dlhandle, lua_tostring(lua, 2));
    tmpstr = dlerror();
    if (tmpstr != NULL)
    {
        /*
         * Error case -- return nil for the symbol, and
         * include the error string as the second return value.
         *
         * Note: not treating this as an error here since the caller
         * can use this to check if a symbol exists or not.  If it is
         * an error the caller should call lua_error if that is appropriate
         */
        lua_pushstring(lua, tmpstr);
    }
    else
    {
        /* Success - return the symbol as a user data */
        seds_c_symbol_t *c_sym = seds_new_symbol_obj(lua);
        c_sym->symaddr = sym;
        ++plugin->refcount;
    }
    return 1;
}

/**
 * Garbage collector function for plugin objects
 *
 * Note that it is potentially dangerous to ever really unload an object.
 * This will only do anything if the refcount is zero, which is only true
 * for objects which never had any successful symbol lookups.
 */
static int seds_c_plugin_unload(lua_State *lua)
{
    seds_c_plugin_t *plugin = lua_touserdata(lua, 1);
    if (plugin->dlhandle != NULL && plugin->refcount == 0)
    {
        dlclose(plugin->dlhandle);
        plugin->dlhandle = NULL;
    }
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Push a new c_plugin userdata object onto the Lua stack
 */
static seds_c_plugin_t *seds_new_plugin_obj(lua_State *lua)
{
    seds_c_plugin_t *obj = lua_newuserdata(lua, sizeof(seds_c_plugin_t));
    SEDS_ASSERT(obj != NULL, "failed to allocate userdata");
    if (luaL_newmetatable(lua, "c_plugin"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_c_plugin_get_symbol);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__gc");
        lua_pushcfunction(lua, seds_c_plugin_unload);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    lua_rawgetp(lua, LUA_REGISTRYINDEX, &SEDS_PLUGIN_TABLE_KEY);
    lua_pushvalue(lua, -2);
    lua_rawseti(lua, -2, 1 + lua_rawlen(lua, -2));
    lua_pop(lua, 1);

    return obj;
}

/* ------------------------------------------------------------------- */
/**
 * Attempt to attach the given C symbol as an EdsLib database object.
 *
 * Since the SEDS tool contains a statically-linked copy of the EdsLib library,
 * this can dynamically attach an actual database to use the library with.
 *
 * The table object is then returned to the caller.
 */
static int seds_attach_edslib_db(lua_State *lua)
{
    seds_c_symbol_t *c_sym = luaL_checkudata(lua, 1, "c_symbol");
    EdsLib_Lua_Attach(lua, c_sym->symaddr);
    return 1;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable function to load a C shared object (.so) file
 */
static int seds_lua_load_plugin_module(lua_State *lua)
{
    seds_plugin_load_so(lua, luaL_checkstring(lua, 1));
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
void seds_plugin_load_so(lua_State *lua, const char *filename)
{
    void *dlhandle;
    const char *tmpstr;
    seds_c_symbol_t c_sym;

    seds_user_message_printf(SEDS_USER_MESSAGE_INFO, NULL, 0, "LOADING PLUGIN: %s", filename);
    dlerror();
    dlhandle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
    tmpstr = dlerror();
    if (dlhandle == NULL && tmpstr == NULL)
    {
        tmpstr = "Unknown";
    }
    if (tmpstr != NULL)
    {
        lua_pushstring(lua, tmpstr);
        lua_error(lua);
    }
    else
    {
        /*
         * If the loaded object defines a function called "seds_lua_plugin_init"
         * then treat this as a Lua library init function.  Call this function
         * and pass the return value back to the caller.
         *
         * Otherwise, treat this as a general purpose C library.  Return a userdata
         * object where the __index metamethod performs a symbol lookup and returns
         * the value of that symbol as a light user data.
         */
        c_sym.symaddr = dlsym(dlhandle, "lua_plugin_init");
        tmpstr = dlerror();
        if (c_sym.symaddr != NULL && tmpstr == NULL)
        {
            lua_pushcfunction(lua, c_sym.lua_func);
            lua_call(lua, 0, 1);
        }
        else
        {
            seds_c_plugin_t *plugin = seds_new_plugin_obj(lua);
            plugin->dlhandle = dlhandle;
            plugin->refcount = 0;
        }
    }
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_plugin_load_lua(lua_State *lua, const char *runtime_file)
{
    char runtime_file_buffer[512];
    int top_start = lua_gettop(lua);

    if (sedstool.user_runtime_path == NULL)
    {
        sedstool.user_runtime_path = dirname(SEDS_DEFAULT_SOURCE_PATH);
    }

    snprintf(runtime_file_buffer, sizeof(runtime_file_buffer), "%s/%s",
            sedstool.user_runtime_path, runtime_file);

    if (luaL_loadfile(lua, runtime_file_buffer) != LUA_OK)
    {
        /* error while parsing file */
        seds_user_message_preformat(SEDS_USER_MESSAGE_FATAL, runtime_file, 0, lua_tostring(lua, -1), NULL);
    }

    lua_settop(lua, top_start + 1);
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_plugin_register_globals(lua_State *lua)
{
    seds_c_plugin_t *mainprog;

    lua_newtable(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &SEDS_PLUGIN_TABLE_KEY);

    /*
     * Create a "fake" entry in the plugin table referring to the main program
     * This has its dlhandle value set to NULL, which dlsym interprets as
     * a request to look up a symbol in the main executable.
     *
     * This simplifies future symbol lookups.
     */
    mainprog = seds_new_plugin_obj(lua);
    mainprog->dlhandle = NULL;
    lua_pop(lua, 1);

    lua_pushcfunction(lua, seds_lua_load_plugin_module);
    lua_setfield(lua, 1, "load_plugin");

    lua_pushcfunction(lua, seds_attach_edslib_db);
    lua_setfield(lua, 1, "attach_db");
}
