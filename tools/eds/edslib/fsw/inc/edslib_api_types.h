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
 * \file     edslib_api_types.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Defines the basic structures and enumerated data types for interaction with
 * the EDS runtime library.  This file contains common definitions that are shared
 * between the generated database sources and the runtime software library.
 *
 * Some structure types defined here are abstract objects; user application code
 * may only deal with _pointers_ to these objects, which must not be modified
 * or otherwise accessed directly by the user application.   The EDS library
 * defines public API calls to interpret these objects and get meaningful data
 * out of them.
 *
 * In addition to the structure types, this file defines several enumerations
 * that are shared between the generated code and the runtime code.  This file should
 * contain any definition that is intended to be globally defined throughout both
 * the runtime public API as well as in the generated sources.
 */

#ifndef _EDSLIB_API_TYPES_H_
#define _EDSLIB_API_TYPES_H_


/**
 * Abstract pointer to a basic EDS object with minimal info on the member objects
 * This is also referred to as the data dictionary since it has info on how to
 * "walk through" the data objects.
 *
 * Note that these objects are always constant; the database objects are generated
 * from the toolchain, compiled in and should never be modified at runtime.
 */
typedef const struct EdsLib_App_DataTypeDB *    EdsLib_DataTypeDB_t;

/**
 * Abstract pointer to a full EDS object with detailed info on the member objects
 * This is also referred to as the name dictionary since it includes display
 * names and information on how to format the elements in a UI.
 *
 * Note that these objects are always constant; the database objects are generated
 * from the toolchain, compiled in and should never be modified at runtime.
 */
typedef const struct EdsLib_App_DisplayDB *     EdsLib_DisplayDB_t;

/**
 * An EDS runtime database object
 *
 * Also known as a "global dictionary" (GD), this is the main top-level object
 * which encapsulates all information from the electronic data sheets.  Almost
 * all EdsLib API calls require this object as the first parameter.
 *
 * Note that this top level object can exist in both const and non-const forms.
 * EdsLib supports a dynamic per-application registration, where individual application
 * database objects can be added or removed from the top level object.
 */
struct EdsLib_DatabaseObject
{
   uint16_t AppTableSize;           /**< Length of the "DataTypeDB_Table" and "DataDisplayDB_Table" arrays */
   EdsLib_DataTypeDB_t *DataTypeDB_Table;
   EdsLib_DisplayDB_t *DisplayDB_Table;
};

typedef struct EdsLib_DatabaseObject            EdsLib_DatabaseObject_t;

/**
 * Indicates the fundamental nature of various elements within EDS basic data structures.
 * All external EdsLib calls will use this definition.
 */
typedef enum
{
    EDSLIB_BASICTYPE_NONE = 0,        /**< Reserved value, any content for this element should be ignored */
    EDSLIB_BASICTYPE_SIGNED_INT,      /**< Interpret as a signed integer.  Byte swapping applies. */
    EDSLIB_BASICTYPE_UNSIGNED_INT,    /**< Interpret as an unsigned integer. Byte swapping applies. */
    EDSLIB_BASICTYPE_FLOAT,           /**< Interpret as a floating point. Byte swapping applies. */
    EDSLIB_BASICTYPE_BINARY,          /**< Pass-through data blob.  No byte swapping.  (char strings, etc) */
    EDSLIB_BASICTYPE_CONTAINER,       /**< References to multiple other data blobs of heterogeneous types */
    EDSLIB_BASICTYPE_ARRAY,           /**< References to multiple other data blobs of homogeneous type */
    EDSLIB_BASICTYPE_COMPONENT,       /**< References to component entities */
    EDSLIB_BASICTYPE_MAX              /**< Reserved value, should always be last */
} EdsLib_BasicType_t;

typedef enum
{
   EDSLIB_DISPLAYHINT_NONE = 0,         /**< No extra display hints (use default) */
   EDSLIB_DISPLAYHINT_STRING,           /**< indicates that the binary blob contains character string data */
   EDSLIB_DISPLAYHINT_REFERENCE_TYPE,
   EDSLIB_DISPLAYHINT_MEMBER_NAMETABLE, /**< indicates that the binary blob contains sub elements, HintArg => Name Table */
   EDSLIB_DISPLAYHINT_ENUM_SYMTABLE,    /**< indicates that the integer value has associated names for display, HintArg => Name Table */
   EDSLIB_DISPLAYHINT_ADDRESS,          /**< indicates that the integer value represents a memory address */
   EDSLIB_DISPLAYHINT_BOOLEAN,          /**< indicates that the field contains a true/false value */
   EDSLIB_DISPLAYHINT_BASE64,           /**< indicates that the binary blob should use base64 for display */
   EDSLIB_DISPLAYHINT_MAX
} EdsLib_DisplayHint_t;

struct EdsLib_SizeInfo
{
    uint32_t Bits;
    uint32_t Bytes;
};

typedef struct EdsLib_SizeInfo EdsLib_SizeInfo_t;


#endif  /* _EDSLIB_API_TYPES_H_ */

