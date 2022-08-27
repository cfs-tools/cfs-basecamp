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
 * \file     seds_checksum.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Public interface to the SEDS checksum algorithm, used throughout the
 * toolchain operation
 *
 * See full module description in seds_checksum.c
 */

#ifndef _SEDS_CHECKSUM_H_
#define _SEDS_CHECKSUM_H_


#include "seds_global.h"


/*******************************************************************************/
/*                         Macro definitions                                   */
/*******************************************************************************/

/**
 * Initial value to use for checksum calculations
 */
#define SEDS_CHECKSUM_INITIAL        0x49A42201BE3217D6U


/*******************************************************************************/
/*                  Function documentation and prototypes                      */
/*      (everything referenced outside this unit should be described here)     */
/*******************************************************************************/

/**
 * Initialization of the checksum table
 *
 * Must be called early in the startup procedure,
 * before any other functions are used.
 */
void seds_checksum_init_table(void);

/**
 * Update a checksum based on an unsigned integer value
 *
 * The checksum is updated based on the supplied integer.
 *
 * IMPORTANT: all checksum updates are done by value, not by binary representation of the
 * dat.  It should produce the same value on big endian and little endian machines, regardless
 * of how binary integers are represented in memory.
 *
 * @param sum previous checksum value
 * @param localvalue the integer value to incorporate into the checksum
 * @param signficant_bits the number of bits in localvalue to consider
 * @return updated checksum
 */
seds_checksum_t seds_update_checksum_numeric(seds_checksum_t sum, uintmax_t localvalue, seds_integer_t significant_bits);

/**
 * Update a checksum based on a string value
 *
 * The checksum is updated based on the supplied string, which must be terminated by
 * a null character.
 *
 * @note this currently does not take character encoding into account, and in general
 * will probably only produce consistent results if the string is limited to 7-bit ASCII.
 * Any string containing extended character sequences may generate different checksums
 * on different machines.  This could be a candidate for future enhancement, but currently
 * there is not much driving need to support special characters into EDS object names.
 *
 * @param sum previous checksum value
 * @param cstr the string value to incorporate into the checksum
 * @return updated checksum
 */
seds_checksum_t seds_update_checksum_string(seds_checksum_t sum, const char *cstr);

/**
 * Update a checksum based on an integer value
 *
 * The checksum is updated based on the supplied integer.  This is a simplified interface
 * that accepts a seds_integer_t type, as opposed to seds_update_checksum_numeric().
 *
 * @param sum previous checksum value
 * @param value the integer value to incorporate into the checksum
 * @return updated checksum
 */
seds_checksum_t seds_update_checksum_int(seds_checksum_t sum, seds_integer_t value);


#endif  /* _SEDS_CHECKSUM_H_ */

