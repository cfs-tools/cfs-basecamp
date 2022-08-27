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
 * \file     seds_memreq.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Memory requirement calculator module declarations.
 * For full module description, see seds_memreq.c
 */

#ifndef _SEDS_MEMREQ_H_
#define _SEDS_MEMREQ_H_


#include "seds_global.h"
#include "seds_checksum.h"

typedef enum
{
    /**
     * Set when no packing has been identified yet
     */
    SEDS_BYTEPACK_STATUS_UNDEFINED,

    /**
     * Set when packing adheres to "bigendian" style:
     *  - Big Endian byte order
     *  - IEEE754 floating point encoding
     *  - Twos complement signed integer encoding
     *
     * Processors such as POWERPC and SPARC often use this
     * style natively in memory during runtime.
     */
    SEDS_BYTEPACK_STATUS_BIGENDIAN_STYLE,

    /**
     * Set when packing adheres to "littlendian" style:
     *  - Little Endian byte order
     *  - IEEE754 floating point encoding
     *  - Twos complement signed integer encoding
     *
     * Processors such as x86 and ARM often use this
     * style natively in memory during runtime.
     */
    SEDS_BYTEPACK_STATUS_LITTLEENDIAN_STYLE,

    /**
     * Set when packing adheres to none of the defined styles.
     *  - Mixed endianness in a container
     *  - Signed Integer encoding other than twos complement
     *  - Floating Point encoding other than IEEE 754
     *  - Includes embedded padding or special fields
     */
    SEDS_BYTEPACK_STATUS_OTHER
} seds_bytepack_status_t;


/**
 * Structure that tracks the size of EDS-defined objects
 *
 * Instances of these objects (in the form of Lua userdata objects) may be
 * attached to any EDS DOM node which has a size associated with it (e.g. data types).
 */
typedef struct
{
    seds_integer_t raw_bit_size;            /**< Total bits consumed by the object */
    seds_integer_t endpoint_bytes;          /**< Byte offset of the end of the object */
    seds_integer_t local_storage_bytes;     /**< Total byte storage required (may include padding) */
    seds_integer_t local_align_mask;        /**< Expected alignment requirements based on typical alignment rules */
    seds_bytepack_status_t packing_status;  /**< If the structure is packed efficiently, this allows for some added optimizations */
    seds_checksum_t checksum;               /**< Checksum/Hash value for the data type definition */
} seds_memreq_t;


/*******************************************************************************/
/*                  Function documentation and prototypes                      */
/*      (everything referenced outside this unit should be described here)     */
/*******************************************************************************/

/**
 * Registers the memreq functions into the SEDS library object
 * These are added to a table which is at the top of the stack
 */
void seds_memreq_register_globals(lua_State *lua);


#endif  /* _SEDS_MEMREQ_H_ */

