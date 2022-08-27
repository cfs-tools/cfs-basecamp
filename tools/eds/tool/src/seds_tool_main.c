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
 * \file     seds_tool_main.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Build-time utility application to read the XML data sheets and produce
 * CFE-compatible C structures matching the definitions.
 *
 * This is the main executive - the real work is done in separate files.
 */

#include "seds_global.h"
#include "seds_user_message.h"
#include "seds_generic_props.h"
#include "seds_preprocess.h"
#include "seds_tree_node.h"
#include "seds_instance_node.h"
#include "seds_memreq.h"
#include "seds_outputfile.h"
#include "seds_xmlparser.h"
#include "seds_plugin.h"

#include "edslib_init.h"
#include "edslib_datatypedb.h"

#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>



/**
 * Lua source file containing the main implementation of the SEDS
 * tool basic routines.  This is loaded at runtime.
 */
static const char SEDS_RUNTIME_SCRIPT_FILE[] = "seds_runtime.lua";

/**
 * Main global state object
 *
 * There is not much actually in this; most information is
 * in the associated Lua state object.
 */
seds_toplevel_t sedstool;


/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/


static void seds_parse_commandline_symbol(lua_State *lua, const char *sym)
{
    const char *valtxt;
    size_t len;

    /* Trim any leading space */
    while (*sym != 0 && isspace((int)sym[0]))
    {
        ++sym;
    }

    /* Find the assignment operator */
    valtxt = strchr(sym, '=');
    if (valtxt != NULL)
    {
        len = valtxt - sym;
        /* Trim any trailing whitespace on name */
        while (len > 0 && isspace((int)sym[len]))
        {
            --len;
        }
        lua_pushlstring(lua, sym, len);
        lua_pushstring(lua, valtxt + 1);
    }
    else
    {
        lua_pushstring(lua, sym);
        lua_pushboolean(lua, 1);
    }
}

/**
 * Helper function to read all files supplied on the command line.
 *
 * The handling action depends on the file extension, which is used to infer
 * the type of file and what to do with it.
 *
 * XML files (.xml) will be parsed using the xmlparser module
 * Lua files (.lua) will be added to the set of post processing scripts
 * Shared object files (.so) will be loaded as a plug-in
 *
 * The top of the Lua stack initially should be the SEDS global object.
 * At completion, this creates a "root" entry in the SEDS global object
 * that contains the DOM of all XML files that were parsed.
 */
static void seds_parse_commandline_files(lua_State *lua, int argc, char **argv)
{
    const char *fileext;
    const char *filename;
    int arg;
    int sedsmodule_pos;

    /*
     * At the top of the stack should be the table which reflects
     * the global SEDS module
     */
    sedsmodule_pos = lua_gettop(lua);
    luaL_checktype(lua, sedsmodule_pos, LUA_TTABLE);

    /*
     * Create a parser object to handle XML files on the command line
     * stack position will be sedsmodule_pos + 1
     */
    lua_pushcfunction(lua, seds_xmlparser_create);
    lua_call(lua, 0, 1);

    /*
     * Load any .xml files in the command line
     * Load any .lua files as a lua function, to be called later
     * Any other file is worth a warning.
     */
    for (arg = 0; arg < argc; arg++)
    {
        /*
         * locate the last directory separator character,
         * also considering backslash just in case of windows
         */
        filename = argv[arg];
        while ((fileext = strpbrk(filename, "\\/")) != NULL)
        {
            filename = fileext + 1;
        }
        fileext = strrchr(filename, '.');
        if (fileext != NULL)
        {
            if (strcasecmp(fileext, ".xml") == 0)
            {
                /*
                 * XML files are processed immediately in the order given
                 */
                lua_pushstring(lua, "**XML**");
                lua_rawsetp(lua, LUA_REGISTRYINDEX, &sedstool.CURRENT_SCRIPT_KEY);
                lua_pushcfunction(lua, seds_xmlparser_readfile);
                lua_pushvalue(lua, sedsmodule_pos + 1);
                lua_pushstring(lua, argv[arg]);
                lua_call(lua, 2, 0);
                lua_pushnil(lua);
                lua_rawsetp(lua, LUA_REGISTRYINDEX, &sedstool.CURRENT_SCRIPT_KEY);
            }
            else if (strcasecmp(fileext, ".lua") == 0)
            {
                /*
                 * Lua scripts are added to a table for later execution
                 */
                lua_rawgetp(lua, LUA_REGISTRYINDEX, &sedstool.POSTPROCCESING_SCRIPT_TABLE_KEY);
                luaL_checktype(lua, -1, LUA_TTABLE);
                lua_pushstring(lua, filename);
                if (luaL_loadfile(lua, argv[arg]) == LUA_OK)
                {
                    lua_rawset(lua, -3);
                }
                else
                {
                    /* error while parsing file */
                    seds_user_message_preformat(SEDS_USER_MESSAGE_ERROR, argv[arg], 0, lua_tostring(lua, -1), NULL);
                    lua_pop(lua, 2);
                }
                lua_pop(lua, 1);
            }
            else if (strcasecmp(fileext, ".so") == 0)
            {
                /*
                 * Shared objects are loaded in the order given
                 */
                seds_plugin_load_so(lua, argv[arg]);
            }
            else
            {
                seds_user_message_preformat(SEDS_USER_MESSAGE_WARNING, argv[arg], 0, "Unknown file extension", fileext);
            }
            continue;
        }

        seds_user_message_preformat(SEDS_USER_MESSAGE_WARNING, argv[arg], 0, "Cannot identify file", NULL);
    }

    /*
     * Call the finish routine, which should return a root node object (userdata)
     * This root node object can replace the parser object -- don't need it anymore
     */
    lua_pushcfunction(lua, seds_xmlparser_finish);
    lua_pushvalue(lua, sedsmodule_pos + 1);
    lua_call(lua, 1, 1);

    /*
     * Set Root node of the complete EDS document tree,
     * and remove anything that was added to the stack in the process (including the XML parser)
     */
    lua_setfield(lua, sedsmodule_pos, "root");
    lua_settop(lua, sedsmodule_pos);
}

static void seds_gencdecl_initialize_post_processing(lua_State *lua)
{
    const char *verbose_mode;
    seds_integer_t verbosity;

    verbose_mode = getenv("VERBOSE");
    if (verbose_mode != NULL)
    {
        verbosity = strtol(verbose_mode, NULL, 0);
        if (verbosity > sedstool.verbosity)
        {
            sedstool.verbosity = verbosity;
        }
    }

    lua_newtable(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &sedstool.POSTPROCCESING_SCRIPT_TABLE_KEY);

    lua_newtable(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &sedstool.GLOBAL_SYMBOL_TABLE_KEY);
}

/**
 * Lua protected mode error handler function
 *
 * This function can be used in conjunction with a lua_pcall()
 * to provide a useful error message in case a Lua script throws
 * an error during execution.
 *
 * The printed message will include a traceback from the interpreter,
 * along with the string message that was thrown.
 */
static int seds_error_handler(lua_State *lua)
{
    lua_getglobal(lua, "debug");
    lua_getfield(lua, -1, "traceback");
    lua_remove(lua, -2);
    lua_pushnil(lua);
    lua_pushinteger(lua, 3);
    lua_call(lua, 2, 1);
    seds_user_message_preformat(SEDS_USER_MESSAGE_FATAL, NULL, 0, lua_tostring(lua, 1), lua_tostring(lua, -1));
    return 1;
}

/**
 * Helper function to call the processing scripts
 *
 * Execute any lua scripts present in the registry "POSTPROCCESING_SCRIPT_TABLE_KEY"
 *
 * These include toolchain post processing as well as user defined scripts (which should be last)
 * Scripts should be executed in alphanumeric order, but the Lua "next" function
 * does not adhere to any order, so we must sort it on the fly.
 *
 * The Lua stack should be empty at the start of this function
 */
static void seds_call_user_scripts(lua_State *lua)
{
    lua_pushcfunction(lua, seds_error_handler);
    lua_newtable(lua);
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &sedstool.POSTPROCCESING_SCRIPT_TABLE_KEY);

    lua_pushnil(lua);
    while (lua_next(lua, 3) != 0)
    {
        lua_pushvalue(lua, 4);
        lua_rawseti(lua, 2, 1 + lua_rawlen(lua, 2));
        lua_pop(lua, 1);
    }

    lua_getglobal(lua, "table");
    lua_getfield(lua, -1, "sort");
    lua_pushvalue(lua, 2);
    lua_call(lua, 1, 0);
    lua_settop(lua, 3);

    /*
     * MAIN PROCESSING LOOP
     */
    lua_pushnil(lua);
    while (lua_next(lua, 2) != 0)
    {
        lua_pushvalue(lua, 5);
        lua_rawsetp(lua, LUA_REGISTRYINDEX, &sedstool.CURRENT_SCRIPT_KEY);
        lua_pushvalue(lua, 5);
        lua_rawget(lua, 3);
        if (lua_pcall(lua, 0, 0, 1) != LUA_OK)
        {
            break;
        }
        lua_pop(lua, 1);
    }

    lua_pushnil(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &sedstool.CURRENT_SCRIPT_KEY);
}

/**
 * Implementation of the help command line option
 *
 * This prints a usage summary in case an unknown command line
 * option was specified, including "-?"
 */
static void seds_usage_summary(void)
{
    printf("\nUSAGE:\n\n");
    printf("sedstool [-D <VAR>=<VALUE>] [-v] [-s <source_path>] file [...]\n\n");
    printf("   -D <VAR>=<VALUE>:\n");
    printf("      adds VAR to the symbol table, similar to the \'Define\' element\n");
    printf("      in a design parameter XML file.  May be used multiple times.\n\n");
    printf("   -v:\n");
    printf("      Increase the verbosity level.  Specify twice for full debug output.\n\n");
    printf("   -s <source_path>:\n");
    printf("      specify the source path to search for supplemental Lua scripts.  This\n");
    printf("      defaults to the same location the source code was built from, but may\n");
    printf("      change if the code is moved\n\n");
}

/*******************************************************************************/
/*                      Externally-Called Functions                            */
/*      (referenced outside this unit and prototyped in a separate header)     */
/*******************************************************************************/

/*
 * ------------------------------------------------------
 *   MAIN ENTRY POINT
 * ------------------------------------------------------
 */
int main(int argc, char *argv[])
{
    lua_State *lua;
    int arg;

    seds_checksum_init_table();
    EdsLib_Initialize();

    memset(&sedstool,0,sizeof(sedstool));

    /*
     * Create a new Lua state and load the standard libraries
     */
    lua = luaL_newstate();
    luaL_openlibs(lua);

    /*
     * Load the remainder of processing routines.
     * There should be no errors here (otherwise indicates an installation/path problem)
     */
    seds_gencdecl_initialize_post_processing(lua);
    if (seds_user_message_get_count(SEDS_USER_MESSAGE_ERROR) != 0)
    {
        return EXIT_FAILURE;
    }

    while ((arg = getopt (argc, argv, "vD:s:")) != -1)
    {
        switch (arg)
        {
        case 'D':
            lua_rawgetp(lua, LUA_REGISTRYINDEX, &sedstool.GLOBAL_SYMBOL_TABLE_KEY);
            seds_parse_commandline_symbol(lua, optarg);
            lua_rawset(lua, -3);
            lua_pop(lua, 1);
            break;

        case 'v':
            ++sedstool.verbosity;
            break;

        case 's':
            sedstool.user_runtime_path = optarg;
            break;

        default:
            if (isprint (arg))
            {
                seds_user_message_printf(SEDS_USER_MESSAGE_ERROR, "cmdline", 0,
                        "Unknown argument `-%c'.\n", arg);
            }
            else
            {
                seds_user_message_printf(SEDS_USER_MESSAGE_ERROR, "cmdline", 0,
                        "Unknown argument character `\\x%x'.\n", arg);
            }
            break;
        }
    }

    if (seds_user_message_get_count(SEDS_USER_MESSAGE_ERROR) != 0)
    {
        seds_usage_summary();
        return EXIT_FAILURE;
    }

    /*
     * Execute the SEDS runtime library file to create the basic SEDS module
     * this is basically the same effect as "require" in native Lua but this
     * adds special sauce to find the necessary script.
     */
    seds_plugin_load_lua(lua, SEDS_RUNTIME_SCRIPT_FILE);


    /*
     * Execute the runtime script.
     *
     * In module pardigm, the returned object is a table of functions
     * This will be at the top of the stack in position 1
     */
    lua_call(lua, 0, 1);

    /*
     * Preemptively save the table to a global called "SEDS"
     * but do remove the table from the stack since it still
     * needs additional members added.
     */
    lua_pushvalue(lua, -1);
    lua_setglobal(lua, "SEDS");

    /*
     * Add all additional SEDS-related processing functions which
     * are defined in the local C code into the SEDS object just created.
     *
     * Each of these appends to the table at the top of the stack.
     */
    seds_user_message_register_globals(lua);
    seds_generic_props_register_globals(lua);
    seds_tree_node_register_globals(lua);
    seds_instance_node_register_globals(lua);
    seds_memreq_register_globals(lua);
    seds_preprocess_register_globals(lua);
    seds_outputfile_register_globals(lua);
    seds_plugin_register_globals(lua);

    /*
     * Store all command line defines in a "commandline_defines" member
     * inside the SEDS global object
     */
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &sedstool.GLOBAL_SYMBOL_TABLE_KEY);
    lua_setfield(lua, 1, "commandline_defines");

    seds_parse_commandline_files(lua, argc - optind, argv + optind);

    lua_pop(lua, 1);    /* SEDS global, not needed on stack anymore */

    if (seds_user_message_get_count(SEDS_USER_MESSAGE_ERROR) == 0)
    {
        seds_call_user_scripts(lua);
    }

    lua_close(lua);

    printf("SEDS tool complete -- %ld error(s) and %ld warning(s)\n",
            (long)seds_user_message_get_count(SEDS_USER_MESSAGE_ERROR),
            (long)seds_user_message_get_count(SEDS_USER_MESSAGE_WARNING));

    if (seds_user_message_get_count(SEDS_USER_MESSAGE_ERROR) != 0 ||
            seds_user_message_get_count(SEDS_USER_MESSAGE_FATAL) != 0)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
