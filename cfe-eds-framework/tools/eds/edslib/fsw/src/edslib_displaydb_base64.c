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
 * \file     edslib_displaydb_base64.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Utility functions to convert EDS binary data to/from
 * base64 encoded strings
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "edslib_internal.h"

static const char EdsLib_BASE64_CHARSET[64] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

/*
 * Note the reverse lookup must be nonzero for any valid base64 char.  The top
 * bit is used to accomplish this since only the lower 6 are used for decode.
 * This is how the code determines whether a byte is within the charset or not.
 */
static const uint8_t EdsLib_BASE64_REVERSE[128] =
{
        ['A'] = 0x80,['B'] = 0x81,['C'] = 0x82,['D'] = 0x83,
        ['E'] = 0x84,['F'] = 0x85,['G'] = 0x86,['H'] = 0x87,
        ['I'] = 0x88,['J'] = 0x89,['K'] = 0x8A,['L'] = 0x8B,
        ['M'] = 0x8C,['N'] = 0x8D,['O'] = 0x8E,['P'] = 0x8F,
        ['Q'] = 0x90,['R'] = 0x91,['S'] = 0x92,['T'] = 0x93,
        ['U'] = 0x94,['V'] = 0x95,['W'] = 0x96,['X'] = 0x97,
        ['Y'] = 0x98,['Z'] = 0x99,['a'] = 0x9A,['b'] = 0x9B,
        ['c'] = 0x9C,['d'] = 0x9D,['e'] = 0x9E,['f'] = 0x9F,
        ['g'] = 0xA0,['h'] = 0xA1,['i'] = 0xA2,['j'] = 0xA3,
        ['k'] = 0xA4,['l'] = 0xA5,['m'] = 0xA6,['n'] = 0xA7,
        ['o'] = 0xA8,['p'] = 0xA9,['q'] = 0xAA,['r'] = 0xAB,
        ['s'] = 0xAC,['t'] = 0xAD,['u'] = 0xAE,['v'] = 0xAF,
        ['w'] = 0xB0,['x'] = 0xB1,['y'] = 0xB2,['z'] = 0xB3,
        ['0'] = 0xB4,['1'] = 0xB5,['2'] = 0xB6,['3'] = 0xB7,
        ['4'] = 0xB8,['5'] = 0xB9,['6'] = 0xBA,['7'] = 0xBB,
        ['8'] = 0xBC,['9'] = 0xBD,['+'] = 0xBE,['/'] = 0xBF
};


void EdsLib_DisplayDB_Base64Encode(char *Out, uint32_t OutputLenBytes, const uint8_t *In, uint32_t InputLenBits)
{
    uint32_t ShiftReg;
    uint32_t NumBits;
    uint32_t OutputPos;
    uint32_t TailPos;
    uint8_t OutCh;

    /*
     * Note - this currently does an unpadded base64 conversion i.e.
     * it will not add the "=" chars for lengths that are not a multiple
     * of 3.  These are really not necessary unless concatenating data.
     */
    OutputPos = 1;
    TailPos = 0;
    ShiftReg = 0;
    NumBits = 0;
    while(InputLenBits > 0)
    {
        /* note at the start of this loop NumBits is always < 6,
         * so it is guaranteed there is enough space to read at least 8 bits */
        ShiftReg <<= 8;
        ShiftReg |= (*In);
        ++In;

        if (InputLenBits >= 8)
        {
            /* all 8 bits are valid */
            NumBits += 8;
            InputLenBits -= 8;
        }
        else
        {
            /* shift out bits that are not valid */
            NumBits += InputLenBits;
            ShiftReg >>= (8 - InputLenBits);

            /* pad with zero bits to make a multiple of 6 */
            InputLenBits = NumBits % 6;
            if (InputLenBits != 0)
            {
                InputLenBits = 6 - InputLenBits;
                ShiftReg <<= InputLenBits;
                NumBits += InputLenBits;
                InputLenBits = 0;
            }
        }

        while (NumBits >= 6)
        {
            NumBits -= 6;
            if (OutputPos < OutputLenBytes)
            {
                OutCh = (ShiftReg >> NumBits) & 0x3F;
                if (OutCh != 0)
                {
                    /* track the last nonzero bit... */
                    TailPos = OutputPos;
                }
                *Out = EdsLib_BASE64_CHARSET[OutCh];
                ++Out;
                ++OutputPos;
            }
        }
    }

    /* truncate at the last nonzero character...
     * the decode operation will fill trailing zeros back in,
     * so no need to include trailing zeros in the base64 data */
    if (TailPos < OutputLenBytes)
    {
        Out -= OutputPos - TailPos - 1;
        *Out = 0;
    }
}

void EdsLib_DisplayDB_Base64Decode(uint8_t *Out, uint32_t OutputLenBits, const char *In)
{
    uint32_t ActualLen;
    uint32_t ShiftReg;
    uint32_t NumBits;
    uint8_t Ch;

    /* the native buffer is always a multiple of 8 bits,
     * even if the packed EDS size is not */
    ActualLen = (OutputLenBits + 7) / 8;
    ShiftReg = 0;
    NumBits = 0;

    while(ActualLen > 0)
    {
        /* note at the start of this loop NumBits is always < 8,
         * so it is guaranteed there is enough space to read at least 8 bits */
        Ch = *In;
        if (Ch != 0)
        {
            Ch = EdsLib_BASE64_REVERSE[Ch & 0x7F];
            /* If the value is not within the base64 charset,
             * then just skip it.  This might be embedded newlines or spaces. */
            if (Ch != 0)
            {
                ShiftReg <<= 6;
                ShiftReg |= Ch & 0x3F;
                NumBits += 6;
            }
            ++In;
        }
        else
        {
            /* generate zero padding to fill the entire space */
            ShiftReg <<= 8;
            NumBits += 8;
        }


        if (NumBits >= 8)
        {
            NumBits -= 8;
            *Out = (ShiftReg >> NumBits) & 0xFF;
            --ActualLen;
            ++Out;
        }
    }
}

