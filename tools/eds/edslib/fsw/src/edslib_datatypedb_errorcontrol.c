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
 * \file     edslib_datatypedb_errorcontrol.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of the error control algorithms for the "ErrorControl"
 * fields within container objects.  These all follow a naming convention
 * of "EdsLib_ErrorControlAlgorithm_<ALGO>()" and the prototypes of these
 * algorithms must be in edslib_database_types.h has they are referenced
 * directly from generated DB code.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "edslib_internal.h"

/*
 * CRC Algorithm: CRC-16/CCITT-FALSE
 * Polynomial: 0x1021
 * Initilization: 0xFFFF
 * Input Reflection: False
 * Output Reflection: False
 * XOR Output: 0x0000
 */
#define EDSLIB_CRC16_CCITT_POLY     0x1021  /* x^16 + x^12 + x^5 + 1 */

/*
 * CRC Algorithm: CRC-8
 * Polynomial: 0x07
 * Initialization: 0x00
 * Input Reflection: False
 * Output Reflection: False
 * XOR Output: 0x00
 */
#define EDSLIB_CRC8_POLY            0x07    /* x^8 + x^2 + x^1 + 1 */


typedef uintmax_t (*EdsLib_ErrorControlImpl_t)(const void *Buffer, uint32_t BufferSizeBytes, uint32_t ErrCtlBitPos);

static uintmax_t EdsLib_ErrorControlAlgorithm_ZERO(const void *Base, uint32_t Size, uint32_t ErrCtlBitPos);
static uintmax_t EdsLib_ErrorControlAlgorithm_CHECKSUM_LONGITUDINAL(const void *Base, uint32_t Size, uint32_t ErrCtlBitPos);
static uintmax_t EdsLib_ErrorControlAlgorithm_CRC16_CCITT(const void *Base, uint32_t Size, uint32_t ErrCtlBitPos);
static uintmax_t EdsLib_ErrorControlAlgorithm_CRC8(const void *Base, uint32_t Size, uint32_t ErrCtlBitPos);
static uintmax_t EdsLib_ErrorControlAlgorithm_CHECKSUM(const void *Base, uint32_t Size, uint32_t ErrCtlBitPos);


static const EdsLib_ErrorControlImpl_t EDSLIB_ERRCTL_DISPATCH[EdsLib_ErrorControlType_MAX] =
{
        [EdsLib_ErrorControlType_INVALID] = EdsLib_ErrorControlAlgorithm_ZERO,
        [EdsLib_ErrorControlType_CHECKSUM] = EdsLib_ErrorControlAlgorithm_CHECKSUM,
        [EdsLib_ErrorControlType_CHECKSUM_LONGITUDINAL] = EdsLib_ErrorControlAlgorithm_CHECKSUM_LONGITUDINAL,
        [EdsLib_ErrorControlType_CRC8] = EdsLib_ErrorControlAlgorithm_CRC8,
        [EdsLib_ErrorControlType_CRC16_CCITT] = EdsLib_ErrorControlAlgorithm_CRC16_CCITT,
        [EdsLib_ErrorControlType_CRC32] = EdsLib_ErrorControlAlgorithm_ZERO /* placeholder; not implemented yet */
};


static uint16_t EDSLIB_CRC16_CCITT_TABLE[256];
static uint8_t  EDSLIB_CRC8_TABLE[256];


/*
 * This function serves two purposes:
 *  1) A place to put initialization code for all error control algorithms, such
 *     as generating a lookup table for CRCs or whatever else needs to be done
 *  2) Something is called during initialization to ensure that this compilation unit
 *     is included in the link.  In general the algorithms in here are only referenced
 *     by dynamically loaded code, so having this init function ensures that this code
 *     will be pulled into the final executable.
 */
void EdsLib_ErrorControl_Initialize(void)
{
    uint16_t i;
    uint16_t bit;
    uint32_t crc16_shiftreg;
    uint32_t crc8_shiftreg;

    for (i = 0; i < 256; ++i)
    {
        crc16_shiftreg = i << 8;
        crc8_shiftreg = 0;

        for (bit = 0; bit < 8; ++bit)
        {
            if (i & (1 << bit))
            {
                crc8_shiftreg |= 0x80 >> bit;
            }
        }

        for (bit = 0; bit < 8; ++bit)
        {
            crc16_shiftreg <<= 1;
            crc8_shiftreg <<= 1;
            if (crc16_shiftreg & 0x10000)
            {
                crc16_shiftreg ^= EDSLIB_CRC16_CCITT_POLY;
            }
            if (crc8_shiftreg & 0x100)
            {
                crc8_shiftreg ^= EDSLIB_CRC8_POLY;
            }
        }

        EDSLIB_CRC8_TABLE[i] = 0;
        for (bit = 0; bit < 8; ++bit)
        {
            if (crc8_shiftreg & (1 << bit))
            {
                EDSLIB_CRC8_TABLE[i] |= 0x80 >> bit;
            }
        }

        EDSLIB_CRC16_CCITT_TABLE[i] = crc16_shiftreg & 0xFFFF;
    }
}

uintmax_t EdsLib_ErrorControlAlgorithm_ZERO(const void *Base, uint32_t TotalBitSize, uint32_t ErrCtlBitPos)
{
    return 0;
}


uintmax_t EdsLib_ErrorControlAlgorithm_CHECKSUM_LONGITUDINAL(const void *Base, uint32_t TotalBitSize, uint32_t ErrCtlBitPos)
{
    const uint8_t *SrcPtr;
    uint32_t ErrCtlByte;
    uint32_t CurrByte;
    uint32_t TotalByte;
    uint8_t NextMask;
    uint8_t Byte;
    uint8_t Checksum;

    NextMask = 0xFF;
    Checksum = 0xFF;
    TotalByte = (TotalBitSize + 7) >> 3;
    ErrCtlByte = ErrCtlBitPos >> 3;
    CurrByte = 0;
    SrcPtr = Base;
    while (CurrByte < TotalByte)
    {
        Byte = *SrcPtr;
        ++SrcPtr;

        if (CurrByte == ErrCtlByte)
        {
            NextMask = (0x100 >> (ErrCtlBitPos & 0x07)) - 1;
            Byte &= ~NextMask;
        }
        else
        {
            Byte &= NextMask;
            NextMask = 0xFF;
        }

        Checksum ^= Byte;
        ++CurrByte;
    }

    return Checksum;
}

uintmax_t EdsLib_ErrorControlAlgorithm_CRC16_CCITT(const void *Base, uint32_t TotalBitSize, uint32_t ErrCtlBitPos)
{
    const uint8_t *SrcPtr;
    uint32_t NextBitPos;
    uint32_t CurrBitPos;
    uint32_t BreakpointBitPos;
    uint32_t CurrShift;
    uint16_t crc;
    uint8_t byte;

    crc = 0xFFFF;
    SrcPtr = Base;
    CurrBitPos = 0;
    if (ErrCtlBitPos < TotalBitSize)
    {
        BreakpointBitPos = ErrCtlBitPos;
    }
    else
    {
        BreakpointBitPos = TotalBitSize;
    }

    CurrShift = 0;
    while(CurrBitPos < TotalBitSize)
    {
        byte = SrcPtr[CurrBitPos >> 3];
        NextBitPos = CurrBitPos + 8;
        if (CurrShift == 0 && NextBitPos <= BreakpointBitPos)
        {
            crc = EDSLIB_CRC16_CCITT_TABLE[(byte ^ (crc >> 8)) & 0xff] ^
                    (crc << 8);
        }
        else
        {
            byte = byte << CurrShift;
            NextBitPos -= CurrShift;

            if (NextBitPos > BreakpointBitPos)
            {
                NextBitPos = BreakpointBitPos;
            }

            while(CurrBitPos < NextBitPos)
            {
                if (byte & 0x80)
                {
                    crc ^= EDSLIB_CRC16_CCITT_POLY;
                }
                byte <<= 1;
                ++CurrBitPos;
            }

            if (CurrBitPos == ErrCtlBitPos)
            {
                NextBitPos = CurrBitPos + 16;
            }

            CurrShift = (0 - NextBitPos) & 0x7;

            BreakpointBitPos = TotalBitSize;
        }
        CurrBitPos = NextBitPos;
    }


    return crc;
}

uintmax_t EdsLib_ErrorControlAlgorithm_CRC8(const void *Base, uint32_t TotalBitSize, uint32_t ErrCtlBitPos)
{
    const uint8_t *SrcPtr;
    uint32_t NextBitPos;
    uint32_t CurrBitPos;
    uint32_t BreakpointBitPos;
    uint32_t CurrShift;
    uint8_t crc;
    uint8_t byte;

    crc = 0;
    SrcPtr = Base;
    CurrBitPos = 0;
    if (ErrCtlBitPos < TotalBitSize)
    {
        BreakpointBitPos = ErrCtlBitPos;
    }
    else
    {
        BreakpointBitPos = TotalBitSize;
    }

    CurrShift = 0;
    while(CurrBitPos < TotalBitSize)
    {
        byte = SrcPtr[CurrBitPos >> 3];
        NextBitPos = CurrBitPos + 8;
        if (CurrShift == 0 && NextBitPos <= BreakpointBitPos)
        {
            crc = EDSLIB_CRC8_TABLE[crc ^ byte];
        }
        else
        {
            byte = byte << CurrShift;
            NextBitPos -= CurrShift;

            if (NextBitPos > BreakpointBitPos)
            {
                NextBitPos = BreakpointBitPos;
            }

            while(CurrBitPos < NextBitPos)
            {
                if (byte & 0x80)
                {
                    crc ^= EDSLIB_CRC8_POLY;
                }
                byte <<= 1;
                ++CurrBitPos;
            }

            if (CurrBitPos == ErrCtlBitPos)
            {
                NextBitPos = CurrBitPos + 8;
            }

            CurrShift = (0 - NextBitPos) & 0x7;
            BreakpointBitPos = TotalBitSize;
        }
        CurrBitPos = NextBitPos;
    }


    return crc;
}

uintmax_t EdsLib_ErrorControlAlgorithm_CHECKSUM(const void *Base, uint32_t TotalBitSize, uint32_t ErrCtlBitPos)
{
    const uint8_t *SrcPtr;
    uintmax_t sum = 0;
    uint32_t NextMask = 0xFFFFFFFF;
    uint32_t offset = 0;
    uint32_t intermediate = 0;

    SrcPtr = Base;
    for (offset=0; offset < TotalBitSize; offset += 8)
    {
        intermediate = (intermediate << 8) | *SrcPtr;
        if ((offset & 0x18) == 0x18)
        {
            /* mask out bits that are part of the error control field itself */
            if ((offset >> 5) == (ErrCtlBitPos >> 5))
            {
                NextMask = (UINT32_C(1) << (32-(ErrCtlBitPos & 0x1F))) - 1;
                intermediate &= ~NextMask;
            }
            else
            {
                intermediate &= NextMask;
                NextMask = 0xFFFFFFFF;
            }

            sum += intermediate;
        }
        ++SrcPtr;
    }

    if ((offset & 0x1F) != 0)
    {
        intermediate <<= 32 - (offset & 0x1F);
        intermediate &= NextMask;
        sum += intermediate;
    }

    return sum;
}


/*******************************************************
 * MAIN ERROR CONTROL IMPLEMENTATION FUNCTION
 *******************************************************/
uintmax_t EdsLib_ErrorControlCompute(EdsLib_ErrorControlType_t Algorithm, const void *Buffer, uint32_t BufferSizeBytes, uint32_t ErrCtlBitPos)
{
    uint32_t Idx;

    Idx = Algorithm;
    if (Idx >= EdsLib_ErrorControlType_MAX)
    {
        Idx = EdsLib_ErrorControlType_INVALID;
    }

    return (EDSLIB_ERRCTL_DISPATCH[Idx])(Buffer, BufferSizeBytes, ErrCtlBitPos);
}


