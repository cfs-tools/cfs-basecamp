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
 * \file     seds_global.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Defines type abstractions that apply to the entire EDS conversion/build process.
 *
 * Note that this tool library does NOT use OSAL -- this is built and executed
 * before osal ever enters the picture.  It only runs on the build/host machine
 * so there should not be any issues using the C library headers directly.
 *
 * That being said, this should still be portable to any other system out there
 * that implements basic POSIX/XOpen standards (Cygwin, OSX, BSD) and has a Lua
 * version 5.2 interpreter available.
 */

#ifndef _SEDS_GLOBAL_H_
#define _SEDS_GLOBAL_H_


/* Use C99 standard integer/boolean types.  Any viable host/build system must have these headers. */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

/*
 * Much of the tool code is actually implemented in Lua, through
 * local scripts and add-on scripts supplied with other code.
 *
 * On Debian/Ubuntu style distributions the Lua headers reside
 * in a version-specific subdirectory.
 */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* compatibility shim to support compilation with Lua5.1 */
#include "edslib_lua51_compatibility.h"


/*
 * When using GNU C, it can give helpful warnings about mismatched arguments to printf-style calls.
 * this macro enables that error checking.
 */
#if defined (__GNUC__)
#define SEDS_PRINTF(n,m)  __attribute__ ((format (printf, n, m)))
#else
#define SEDS_PRINTF(n,m)
#endif

/**
 * Use the max-width native integer type to store EDS integer numbers and values.
 *
 * This is used to hold symbol values, sizes, offsets, or any other general purpose integer.
 * Using the largest available type to avoid overflow/conversion issues.  This will be 64 bits
 * wide on any reasonable build machine, but 32 bits will likely also be enough as long as the
 * data sheet does not use any big numbers.
 */
typedef intmax_t seds_integer_t;

/** EDS Checksums are 64-bit unsigned values */
typedef uint64_t seds_checksum_t;

/**
 * Minimum value for seds_integer_t values
 */
#define SEDS_INTEGER_MIN         INTMAX_MIN

/**
 * Maximum value for seds_integer_t values
 */
#define SEDS_INTEGER_MAX         INTMAX_MAX

/** EDS booleans */
typedef bool                     seds_boolean_t;

/** EDS floating-point number values */
typedef double                   seds_number_t;

/**
 * Maximum value for seds_number_t values without losing integer precision.
 * NOTE - this is not the full range of the data type, but rather a "useful range"
 */
#define SEDS_NUMBER_MAX         ((seds_number_t)((1ULL << DBL_MANT_DIG)))

/**
 * Minimum value for seds_number_t values without losing integer precision.
 * NOTE - this is not the full range of the data type, but rather a "useful range"
 */
#define SEDS_NUMBER_MIN         (-SEDS_NUMBER_MAX)

/**
 * Structure to store the top-level state of the toolchain as a while
 * This is primarily user-supplied command line options
 */
typedef struct
{
    /**
     * Path to source code.
     * For parts of the tool that are loaded at runtime, this is the
     * path that will be searched for the source files.   This includes
     * all Lua code.  This may be set using the "-s" command line option.
     */
    const char *user_runtime_path;

    /**
     * Verbosity of console output.
     * Higher numbers include additional messages, such as debug messages.
     * The "-v" command line option or the VERBOSE environment variable
     * may be used to change this value.
     */
    seds_integer_t verbosity;

    /*
     * The following fields do not hold any values themselves,
     * but rather the address serves as a unique key into the Lua
     * registry.
     */
    const char GLOBAL_SYMBOL_TABLE_KEY;         /**< Key for Lua table containing SEDS defines */
    const char POSTPROCCESING_SCRIPT_TABLE_KEY; /**< Key for Lua table containing postprocessing scripts */
    const char CURRENT_SCRIPT_KEY;              /**< Key for currently-running script */

} seds_toplevel_t;


/**
 * A single global containing the top level state object of the tool.
 */
extern seds_toplevel_t sedstool;


#endif  /* _SEDS_GLOBAL_H_ */

