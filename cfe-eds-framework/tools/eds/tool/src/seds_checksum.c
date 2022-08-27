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
 * \file     seds_checksum.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation file for the checksum module.
 *
 * This is essentially a hash function computed on the specific binary
 * representation of EDS-defined objects.  If the binary representation ever
 * changes, the hash should change in an unpredictable way.  Likewise, if
 * two EDS binary representations are exactly identical the hash should also
 * be the same value.
 *
 * The algorithm is based on CRC, but expanded to 64 bits to reduce the chance
 * of a random collision.
 *
 * This has two purposes:
 *  1) Avoid creating duplicate output in generated files (if two object checksums
 *     are equal, then the definition may not need to be repeated).
 *  2) Verify compatibility between stored data and data definitions.  If the checksum
 *     is stored with the original data at the time it is saved, this can be compared
 *     against the data definition at the time it is loaded.  If the values are different
 *     than it is likely that the definition does not match the actual data, which
 *     would suggest that the loaded data would not be correct.
 *
 * @Note this hash is intended to protect against common use cases and operator errors.
 * This is not a cryptographic hash function and is not intended to replace a
 * secure signature for EDS objects for those applications that require it.
 *
 * Also note, if the checksum algorithm implementation ever changes, it will invalidate
 * all the checksums in any previously stored files.  This algorithm should be changed
 * with great care if any existing binary files may need to be preserved.
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "seds_checksum.h"
#include "seds_global.h"

/**
 * Polynomial used to initialize the checksum table
 */
#define SEDS_CHECKSUM_POLY        0x04C11DB70012E321U

/**
 * Internal byte-wise checksum table, to speed up calculations
 *
 * Initialized by seds_checksum_init_table()
 */
static seds_checksum_t SEDS_CHECKSUM_TABLE[256];

/*******************************************************************************/
/*                      Externally-Called Functions                            */
/*      (referenced outside this unit and prototyped in a separate header)     */
/*******************************************************************************/

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
void seds_checksum_init_table(void)
{
    seds_checksum_t v;
    seds_checksum_t mask;
    seds_checksum_t result;
    int32_t i, j;

    mask = 1;
    mask <<= 8 * sizeof(mask) - 1;
    /*
     * This is a modified / simplified crc-type checksum
     * Since there might be a lot of zeros in the input, and
     * in a conventional CRC a zero byte does not affect the hash,
     * the table is skewed to place the "no effect" byte at a
     * different spot in the table.
     */
    for(i = 0; i < 256; ++i)
    {
        v = i ^ 0x7F;
        v <<= 8 * sizeof(v) - 8;
        for (j = 0; j < 8; ++j)
        {
            if (v & mask)
            {
                v = (v << 1) ^ SEDS_CHECKSUM_POLY;
            }
            else
            {
                v <<= 1;
            }
        }

        result = 0;
        for (j = 0; j < (8 * sizeof(v)); ++j)
        {
            result = (result << 1) | (v & 1);
            v >>= 1;
        }

        SEDS_CHECKSUM_TABLE[i] = result;
    }
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
seds_checksum_t seds_update_checksum_numeric(seds_checksum_t sum, uintmax_t localvalue, seds_integer_t significant_bits)
{
    const uintmax_t LOCAL_MASK = (((uintmax_t)1) << (sizeof(uintmax_t) * CHAR_BIT - 8)) - 1;

    /*
     * update the checksum in a manner that should be both
     * endian and encoding-agnostic.
     *
     * if one were to just checksum the memory the result
     * would be dependent on the machine endianness and encoding.
     *
     * Right-shift the bits 8 at at time, padding with "0" bits
     * as the real bits are shifted out.
     */
    if (significant_bits < (sizeof(localvalue) * CHAR_BIT))
    {
        /* Ensure that unused bits are "0", in case the numbits
         * is not an exact multiple of 8. */
        localvalue &= (((uintmax_t)1) << significant_bits) - 1;
    }

    while (significant_bits > 0)
    {
        sum = (sum >> 8) ^ SEDS_CHECKSUM_TABLE[(sum ^ localvalue) & 0xFF];
        /*
         * In theory the right shift should always insert 0 bits on the left,
         * since the argument is unsigned.  However, the C spec allows for some
         * implementation-defined behavior with right shifts, so....
         * Apply a mask to ensure that the leftmost bits are indeed zero.
         */
        localvalue = (localvalue >> 8) & LOCAL_MASK;
        significant_bits -= 8;
    }

    return sum;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
seds_checksum_t seds_update_checksum_string(seds_checksum_t sum, const char *cstr)
{
    seds_integer_t nchars;

    nchars = 0;

    if (cstr != NULL)
    {
        /*
         * update the sum for each char,
         * until a null is encountered
         */
        while (*cstr != 0)
        {
            /* use 8 bits per char, even on systems where CHAR_BIT might be something else */
            sum = seds_update_checksum_numeric(sum, *cstr, 8);
            ++cstr;
            ++nchars;
        }
    }

    sum = seds_update_checksum_int(sum, 1 + nchars);

    return sum;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
seds_checksum_t seds_update_checksum_int(seds_checksum_t sum, seds_integer_t value)
{
    /*
     * Note - the exact width of "seds_integer_t" is machine-dependent, and may be
     * only 32 bits on some machines.
     *
     * The algorithm should still work, as it pads in "0" bits to fill the
     * request size regardless of the actual width of the type.  Of course, the
     * result will be undefined if someone actually uses values that cannot be
     * represented within the "seds_integer_t", but at least it will be consistent.
     */
    return seds_update_checksum_numeric(sum, value, 64);
}

