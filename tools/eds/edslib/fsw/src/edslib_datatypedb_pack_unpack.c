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
 * \file     edslib_datatypedb_pack_unpack.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of byte-swapping logic for EDS-defined data structures.
 * Linked as part of the "basic" EDS runtime library.
 *
 * This code is designed to work on both big-endian and little-endian architectures,
 * and the packing routines will be done either in an entirely endian-neutral manner
 * or, when warranted, by optimizing it based on the EDSLIB_BYTE_ORDER constants.
 * (For instance, this can, in some circumstances, figure out that a structure can
 * be safely memcpy()'ed therefore and avoid unnecessary recursion)
 *
 * -- IMPORTANT --
 * With respect to value encoding, the following is assumed of the host architecture:
 *  - Unsigned integers are simple base-2 values
 *  - Signed integers are twos complement encoded
 *  - float/double are IEEE-754 single/double precision
 *
 * In cases where the EDS also indicates these same encodings, these values will be
 * copied direct from memory (adjusting for byte order) which avoids potentially
 * expensive re-encoding operations.
 *
 * If using this library on any machine that does NOT meet the above assumptions,
 * the optimization is not applicable and will not work.  However, because these
 * encodings are nearly ubiquitous, it is not anticipated this will ever be a problem,
 * and the speed/efficiency tradeoff is worth it for the vast majority of machines
 * that do use this encoding internally.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "edslib_internal.h"

#ifdef EDSLIB_HAVE_LONG_DOUBLE

/*
 * Preferred:
 * Use "long double" internally during encode/decode
 * to minimize loss of precision
 */
typedef long double         EdsLib_Internal_Float_t;

/*
 * Use the math library calls with an "l" suffix
 * These should operate on long doubles
 */
#define EDSLIB_FABS         fabsl
#define EDSLIB_LDEXP        ldexpl
#define EDSLIB_FREXP        frexpl
#define EDSLIB_MODF         modfl
#define EDSLIB_FLOAT_C(x)   (x ## L)

#else

/*
 * Backup:
 * Use "double" internally, may lose some precision
 * but should have wider C library support
 */
typedef double              EdsLib_Internal_Float_t;

/*
 * Use the math library calls without the "l" suffix
 * These should operate on regular doubles
 */
#define EDSLIB_FABS         fabs
#define EDSLIB_LDEXP        ldexp
#define EDSLIB_FREXP        frexp
#define EDSLIB_MODF         modf
#define EDSLIB_FLOAT_C(x)   (x)
#endif

/**
 * How to prune/mask bits in the native memory
 *
 * EDS defines fields in arbitrary bit widths, but the
 * native system always works in octets.  The native values
 * in memory will be padded out to their native size.
 *
 * This padding is typically some number of whole bytes followed
 * by some number of remainder bits, which might be the right
 * side (LSB) or left side (MSB) of the first valid byte.
 *
 * This enum indicates how to handle this when packing/unpacking.
 * It indicates which bits to discard during packing or inject
 * during unpacking.
 */
typedef enum
{
    LEADING_PAD_UNDEFINED = 0,  /**< Unknown or no leading invalid bits */
    LEADING_PAD_MSB,            /**< MSB from leading byte(s) are invalid */
    LEADING_PAD_LSB             /**< LSB From leading byte(s) are invalid */
} EdsLib_BitPackLeadingPad_Enum_t;


typedef struct
{
    EdsLib_NumberByteOrder_Enum_t ByteOrder;
    EdsLib_BasicType_t IntermediateType;
    size_t IntermediateSize;
    ptrdiff_t MemStride;
    ptrdiff_t LeadingPadBits;
    ptrdiff_t TrailingPadBits;
    int32_t IntermediateShift;
    bool Invert;
} EdsLib_Internal_PackStyleInfo_t;

typedef size_t (*EdsLib_Internal_PackFunc_t)(EdsLib_GenericValueBuffer_t *ValBuf, const uint8_t *SrcPtr, size_t SrcSizeBytes, EdsLib_NumberEncoding_Enum_t EncodingStyle, uint32_t EncodingBits);
typedef void (*EdsLib_Internal_UnpackFunc_t)(uint8_t *DstPtr, size_t DstSizeBytes, EdsLib_GenericValueBuffer_t *ValBuf, EdsLib_NumberEncoding_Enum_t EncodingStyle, uint32_t EncodingBits);


static const union
{
    uint16_t   Value;
    int8_t     Bytes[2];
}  EDSLIB_NATIVE_BYTEORDER = { 0x0102 };

/* An expression that evaluates to 1 on big endian and -1 on little endian */
#define EDSLIB_BIG_ENDIAN_STRIDE    \
    (EDSLIB_NATIVE_BYTEORDER.Bytes[1] - EDSLIB_NATIVE_BYTEORDER.Bytes[0])

/* An expression that evaluates to 1 on little endian and -1 on big endian */
#define EDSLIB_LITTLE_ENDIAN_STRIDE    \
    (EDSLIB_NATIVE_BYTEORDER.Bytes[0] - EDSLIB_NATIVE_BYTEORDER.Bytes[1])

/*
 * An expression that evaluates to 1 on big endian and 2 on little endian machines
 * This is intended to line up with the PACK flags in the database:
 *  EDSLIB_DATATYPE_FLAG_PACKED_BE
 *  EDSLIB_DATATYPE_FLAG_PACKED_LE
 */
#define EDSLIB_NATIVE_BYTE_PACK  (EDSLIB_NATIVE_BYTEORDER.Bytes[0])

static bool EdsLib_Internal_GetPackStyle(EdsLib_Internal_PackStyleInfo_t *PackStyle, const EdsLib_DataTypeDB_Entry_t *DataDictPtr)
{
    bool IsValid = true;
    EdsLib_BasicType_t RefType;
    size_t RefSize;
    ptrdiff_t PadBits;

    memset(PackStyle, 0, sizeof(*PackStyle));

    if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_SIGNED_INT ||
            DataDictPtr->BasicType == EDSLIB_BASICTYPE_UNSIGNED_INT ||
            DataDictPtr->BasicType == EDSLIB_BASICTYPE_FLOAT)
    {
        PackStyle->Invert = (DataDictPtr->Detail.Number.BitInvertFlag != 0);
        PackStyle->ByteOrder = DataDictPtr->Detail.Number.ByteOrder;

        if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_SIGNED_INT)
        {
            if (DataDictPtr->Detail.Number.Encoding == EDSLIB_NUMBERENCODING_BCD_OCTET ||
                    DataDictPtr->Detail.Number.Encoding == EDSLIB_NUMBERENCODING_BCD_PACKED)
            {
                PackStyle->IntermediateType = EDSLIB_BASICTYPE_BINARY; /* anticipated intermediate */
                PackStyle->IntermediateSize = (DataDictPtr->SizeInfo.Bits + 7) / 8;
                if (PackStyle->IntermediateSize > sizeof(EdsLib_GenericValueUnion_t))
                {
                    IsValid = false;
                }
            }
            else if (DataDictPtr->Detail.Number.Encoding != EDSLIB_NUMBERENCODING_TWOS_COMPLEMENT &&
                    DataDictPtr->Detail.Number.Encoding != EDSLIB_NUMBERENCODING_UNDEFINED)
            {
                PackStyle->IntermediateType = EDSLIB_BASICTYPE_UNSIGNED_INT; /* anticipated intermediate */
                PackStyle->IntermediateSize = sizeof(EdsLib_Generic_UnsignedInt_t);
            }
            else
            {
                /* twos complement is pass thru - no intermediate value.
                 * Should work on any system using twos complement natively (most everything) */
            }
        }
        else if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_FLOAT)
        {
            if (DataDictPtr->Detail.Number.Encoding == EDSLIB_NUMBERENCODING_MILSTD_1750A)
            {
                /* MIL-STD-1750A supports encodings of 32 or 48 bits */
                if (DataDictPtr->SizeInfo.Bits == 32)
                {
                    PackStyle->IntermediateType = EDSLIB_BASICTYPE_UNSIGNED_INT; /* anticipated intermediate */
                    PackStyle->IntermediateSize = sizeof(uint32_t);
                }
                else if (DataDictPtr->SizeInfo.Bits == 48)
                {
                    PackStyle->IntermediateType = EDSLIB_BASICTYPE_UNSIGNED_INT; /* anticipated intermediate */
                    PackStyle->IntermediateSize = sizeof(uint64_t);
                }
                else
                {
                    /* undefined */
                    IsValid = false;
                }
            }
            else if (DataDictPtr->Detail.Number.Encoding == EDSLIB_NUMBERENCODING_IEEE_754 ||
                    DataDictPtr->Detail.Number.Encoding == EDSLIB_NUMBERENCODING_UNDEFINED)
            {
                /*
                 * IEEE-754 supports encodings of 32, 64, or 128 bits.
                 *
                 * Most systems support 32bit(float) 64bit(double) natively, but not 128bit.
                 *
                 * Some FPUs only do single precision, where float and double are 32 bits,
                 * and there is no native 64-bit float type.
                 * It is also possible that everything natively is double precision, and there
                 * is no native 32-bit float type.
                 */
                if (DataDictPtr->SizeInfo.Bits == 128)
                {
                    /* For IEEE-754 quad, assume the native machine does not implement it.
                     * This will encode/decode to a binary blob, and preserve up to 64 bits
                     * of mantissa precision - the other 48 will be interpolated/filled. */
                    PackStyle->IntermediateType = EDSLIB_BASICTYPE_BINARY;
                    PackStyle->IntermediateSize = 16; /* fixed */
                }
                else if (DataDictPtr->SizeInfo.Bits != (8 * DataDictPtr->SizeInfo.Bytes))
                {
                    /*
                     * In this situation the size in bits of the native type (float or double)
                     * does not match the size in bits of the EDS value (IEEE-754 single or double)
                     * This can happen on some machines with limited FPUs that don't implement both
                     * single and double, where float and double actually alias to the same type.
                     */
                    if (DataDictPtr->SizeInfo.Bits == 64)
                    {
                        PackStyle->IntermediateType = EDSLIB_BASICTYPE_UNSIGNED_INT; /* anticipated intermediate */
                        PackStyle->IntermediateSize = sizeof(uint64_t);
                    }
                    else if (DataDictPtr->SizeInfo.Bits == 32)
                    {
                        PackStyle->IntermediateType = EDSLIB_BASICTYPE_UNSIGNED_INT; /* anticipated intermediate */
                        PackStyle->IntermediateSize = sizeof(uint32_t);
                    }
                    else
                    {
                        /* unsupported size */
                        IsValid = false;
                    }
                }
                else
                {
                    /*
                     * EDS size in bits is the same as native size...
                     * pass-thru - should work on any system
                     * that uses IEEE-754 encoding natively
                     */
                }
            }
            else
            {
                IsValid = false;
            }
        }
    }

    if (!IsValid)
    {
        RefSize = -1;
        PadBits = -1;
    }
    else
    {
        if (PackStyle->IntermediateType != EDSLIB_BASICTYPE_NONE)
        {
            RefSize = PackStyle->IntermediateSize;
            RefType = PackStyle->IntermediateType;
        }
        else
        {
            RefSize = DataDictPtr->SizeInfo.Bytes;
            RefType = DataDictPtr->BasicType;
        }
        PadBits = (RefSize * 8) - DataDictPtr->SizeInfo.Bits;
        IsValid = (PadBits >= 0);
    }

    if (IsValid)
    {
        /* Determine the correct stride for the anticipated data */
        if (PackStyle->ByteOrder == EDSLIB_NUMBERBYTEORDER_LITTLE_ENDIAN)
        {
            if (RefType == EDSLIB_BASICTYPE_BINARY)
            {
                /*
                 * Little endian EDS, binary data:
                 *  Load/Store bytes in reverse address order
                 *  pad bits in first byte, in the LSB's (right-side)
                 *
                 * (applies to both big endian and little endian native CPU types)
                 */
                PackStyle->MemStride = -1;
            }
            else
            {
                /*
                 * Little endian EDS format, numeric data:
                 *  Load/Store bytes in little endian order
                 *  pad bits in last byte, in the MSB's (left-side)
                 *
                 * Extra Bits in the MSB of the last byte is hard to handle as it
                 * will create a "gap" in the stream.  But because it is numeric
                 * in nature, it can be shifted.  This makes it a mirror image and
                 * puts the pad bits at the leading LSB instead.
                 *
                 * This shift also helps when unpacking, as it can be used to
                 * sign-extend twos complement data as needed.
                 */
                PackStyle->MemStride = EDSLIB_LITTLE_ENDIAN_STRIDE;
                PackStyle->IntermediateShift = PadBits;
            }

            /* pad/extra bits are bits are at the leading edge, in LSBs. */
            PackStyle->LeadingPadBits = PadBits;
        }
        else
        {
            if (RefType == EDSLIB_BASICTYPE_BINARY)
            {
                /*
                 * Big endian EDS format, binary data:
                 *  Load/Store bytes in address order
                 *  Pad bits in last byte, in the LSB's (right-side)
                 *
                 * (applies to both big endian and little endian native CPU types)
                 */
                PackStyle->MemStride = 1;
            }
            else
            {
                /*
                 * Big endian input, numeric data:
                 *  Load/Store bytes in big endian order (default)
                 *  pad bits in first byte, in the MSB's (left-side)
                 *
                 * Shift bits here as well, this helps when unpacking, as it
                 * can be used to sign-extend twos complement data.  This
                 * puts extra bits into the last byte LSB instead.
                 */
                PackStyle->MemStride = EDSLIB_BIG_ENDIAN_STRIDE;
                PackStyle->IntermediateShift = PadBits;
            }

            /* pad/extra bits are bits are at the trailing edge, in LSBs. */
            PackStyle->TrailingPadBits = PadBits;
        }
    }

    return IsValid;
}


static size_t EdsLib_Internal_DoSignedIntPack(EdsLib_GenericValueUnion_t *ValBuf, const uint8_t *SrcPtr, size_t SrcSizeBytes, EdsLib_NumberEncoding_Enum_t EncodingStyle, uint32_t EncodingBits)
{
    EdsLib_ConstPtr_t TempSrc = { .Addr = SrcPtr };
    size_t ConvSize;

    switch(SrcSizeBytes)
    {
    case sizeof(TempSrc.u->i8):
        ValBuf->SignedInteger = TempSrc.u->i8;
        break;
    case sizeof(TempSrc.u->i16):
        ValBuf->SignedInteger = TempSrc.u->i16;
        break;
    case sizeof(TempSrc.u->i32):
        ValBuf->SignedInteger = TempSrc.u->i32;
        break;
    case sizeof(TempSrc.u->i64):
        ValBuf->SignedInteger = TempSrc.u->i64;
        break;
    default:
        /* should never happen, but avoid leaving garbage data into the output */
        ValBuf->SignedInteger = 0;
        break;
    }

    switch(EncodingStyle)
    {
    case EDSLIB_NUMBERENCODING_SIGN_MAGNITUDE:
    {
        if (ValBuf->SignedInteger < 0)
        {
            /* convert to a positive unsigned, and set the sign bit */
            ValBuf->UnsignedInteger = -ValBuf->SignedInteger;
            ValBuf->UnsignedInteger |= 1ULL << (EncodingBits-1);
        }
        else
        {
            ValBuf->UnsignedInteger = ValBuf->SignedInteger;
        }
        ConvSize = sizeof(ValBuf->UnsignedInteger);
        break;
    }
    case EDSLIB_NUMBERENCODING_ONES_COMPLEMENT:
    {
        if (ValBuf->SignedInteger < 0)
        {
            /* convert to a positive unsigned, and invert all bits */
            ValBuf->UnsignedInteger = -ValBuf->SignedInteger;
            ValBuf->UnsignedInteger = ~ValBuf->UnsignedInteger;
        }
        else
        {
            ValBuf->UnsignedInteger = ValBuf->SignedInteger;
        }
        ConvSize = sizeof(ValBuf->UnsignedInteger);
        break;
    }
    case EDSLIB_NUMBERENCODING_BCD_OCTET:
    {
        char *p = ValBuf->StringData;
        uint8_t *q = ValBuf->BinaryData;
        size_t len = sizeof(ValBuf->StringData);

        snprintf(p, len, "%0*lld",
                (int)(EncodingBits / 8), ValBuf->SignedInteger);
        len = strlen(p);
        ConvSize = len;

        while(len > 0)
        {
            if (isdigit((int)*p))
            {
                *q = (unsigned int)(*p - '0');
            }
            ++p;
            ++q;
            --len;
        }
        break;
    }
    case EDSLIB_NUMBERENCODING_BCD_PACKED:
    {
        char *p = ValBuf->StringData;
        uint8_t *q = ValBuf->BinaryData;
        size_t len = sizeof(ValBuf->StringData);
        uint8_t out;

        snprintf(p, len, "%0*lld",
                (int)(EncodingBits / 4), ValBuf->SignedInteger);
        len = (strlen(p) + 1) / 2;
        ConvSize = len;

        /*
         * NOTE: This doesn't handle negative values properly.
         * The SOIS Red Book is not exactly clear on this.
         */
        while(len > 0)
        {
            out = 0;
            if (isdigit((int)p[0]))
            {
                out |= (unsigned int)(p[0] - '0') << 4;
            }
            if (isdigit((int)p[1]))
            {
                out |= (unsigned int)(p[1] - '0');
            }
            *q = out;
            p += 2;
            ++q;
            --len;
        }
        break;
    }
    case EDSLIB_NUMBERENCODING_TWOS_COMPLEMENT:
    case EDSLIB_NUMBERENCODING_UNDEFINED:
    {
        /* default encoding type is a pass thru
         * This should work on twos complement machines,
         * which is basically everything. */
        ConvSize = sizeof(ValBuf->SignedInteger);
        break;
    }
    default:
    {
        ConvSize = 0;
        break;
    }
    }

    return ConvSize;
}

static size_t EdsLib_Internal_DoFloatPack(EdsLib_GenericValueUnion_t *ValBuf, const uint8_t *SrcPtr, size_t SrcSizeBytes, EdsLib_NumberEncoding_Enum_t EncodingStyle, uint32_t EncodingBits)
{
    /*
     * All floating point notations (including the less common ones) express
     * the value in some form scientific notation, usually as a power of two.
     *
     * To allow conversion, the value will be broken into its constituent parts
     * (sign bit, exponent, and mantissa).  The values can then be adjusted
     * per the specified encoding style, and bit-packed using the format
     * specified by the EDS.
     *
     * Note these conversions are only intended for real numbers.  For exceptions
     * like NaN or Infinity it will currently encode a zero.  (this could be fixed
     * in a future version if there is a need for these exception values).
     */
    EdsLib_ConstPtr_t TempSrc = { .Addr = SrcPtr };
    size_t x;
    size_t ConvSize;

    int Exponent = 0;
    EdsLib_Internal_Float_t Significand = EDSLIB_FLOAT_C(0.0);

    /* the base-2 exponent/mantissa can be extracted using the
     * frexp() math call */
    if (SrcSizeBytes == sizeof(TempSrc.u->u32))
    {
        Significand = frexpf(TempSrc.u->fpsgl, &Exponent);
    }
    else if (SrcSizeBytes == sizeof(TempSrc.u->u64))
    {
        Significand = frexp(TempSrc.u->fpdbl, &Exponent);
    }
    else if (SrcSizeBytes == sizeof(TempSrc.u->fpmax))
    {
        Significand = EDSLIB_FREXP(TempSrc.u->fpmax, &Exponent);
    }

    if (EncodingStyle == EDSLIB_NUMBERENCODING_MILSTD_1750A)
    {
        /*
         * In MIL-STD-1750A the mantissa uses range of [-1, 1),
         * with the range between [-0.5,0.5) considered invalid.
         * Valid mantissa values are [-1,-0.5), [0.5,1).
         *
         * This differs slightly from the output of frexp() -
         * The latter outputs a range of (-1,-0.5], [0.5,1)
         * The case of exactly -0.5 needs an explicit adjustment
         */
        uint64_t MantissaBits;

        if (Significand == EDSLIB_FLOAT_C(0.0))
        {
            /* Zero is represented by all fields set to zero */
            MantissaBits = 0;
            Exponent = 0;
        }
        else if (Significand == EDSLIB_FLOAT_C(-0.5))
        {
            /* Special case: bump up exponent when exactly -0.5 */
            Significand *= EDSLIB_FLOAT_C(2.0);
            --Exponent;
        }

        /* Get 40 bits worth of mantissa (signed twos complement) */
        MantissaBits = EDSLIB_LDEXP(Significand,39);

        /* Pack into the uint32, filling all 32 bits of it */
        ValBuf->u32 = (uint32_t)(MantissaBits >> 8) & 0xFFFFFF00;
        ValBuf->u32 |= (uint32_t)Exponent & 0xFF;

        if (EncodingBits == 32)
        {
            ConvSize = sizeof(ValBuf->u32);
        }
        else if (EncodingBits == 48)
        {
            /*
             * 1750A extended merely takes the 32 bit value and
             * tacks on an additional 16 bits of mantissa in the LSBs.
             *
             * This will be held in the uint64_t value, with 48 bits valid.
             */
            ValBuf->u64 = ValBuf->u32;  /* extend to 64 bits first */
            ValBuf->u64 = (ValBuf->u64 << 16) | (MantissaBits & 0xFFFF);
            ConvSize = sizeof(ValBuf->u64);
        }
        else
        {
            /* unimplemented */
            ConvSize = 0;
        }
    }
    else if (EncodingStyle == EDSLIB_NUMBERENCODING_IEEE_754 ||
            EncodingStyle == EDSLIB_NUMBERENCODING_UNDEFINED)
    {
        if (EncodingBits == 128)
        {
            /*
             * Because the mantissa is stored locally in a uint64_t,
             * This will never be a full quad precision output which
             * has 112 bits of mantissa.  This will output the
             * 64 bits of precision that we do have, and backfill the
             * remaining 48 bits.
             */

            if (Significand == EDSLIB_FLOAT_C(0.0))
            {
                memset(ValBuf->BinaryData, 0, sizeof(ValBuf->BinaryData));
            }
            else
            {
                /* The first two bytes are the sign and exponent */
                ValBuf->BinaryData[0] = ((Exponent + 16382) >> 8) & 0x7F;
                ValBuf->BinaryData[1] = (Exponent + 16382) & 0xFF;
                if (signbit(Significand))
                {
                    /* set the sign bit */
                    ValBuf->BinaryData[0] |= 0x80;
                }
                /*
                 * Fill all bits of mantissa value from MSB to LSB.
                 */
                EdsLib_Internal_Float_t Frac = EDSLIB_FABS(Significand);
                EdsLib_Internal_Float_t Whole;

                Frac *= EDSLIB_FLOAT_C(2.0);
                for (x=2; x < 16; ++x)
                {
                    Frac = EDSLIB_MODF(EDSLIB_LDEXP(Frac, 8), &Whole);
                    ValBuf->BinaryData[x] = ((uint8_t)Whole) & 0xFF;
                }
            }

            ConvSize = 16;
        }
        else if (EncodingBits == 64)
        {
            if (Significand == EDSLIB_FLOAT_C(0.0))
            {
                ValBuf->u64 = 0;
            }
            else
            {
                ValBuf->u64 = EDSLIB_LDEXP(EDSLIB_FABS(Significand),53);

                ValBuf->u64 &= UINT64_C(0xFFFFFFFFFFFFF);
                ValBuf->u64 |= ((uint64_t)((Exponent + 1022) & 0x7FF)) << 52;

                if (signbit(Significand))
                {
                    ValBuf->u64 |= UINT64_C(1) << 63; /* set the sign bit */
                }
            }

            ConvSize = sizeof(ValBuf->u64);
        }
        else if (EncodingBits == 32)
        {
            if (Significand == EDSLIB_FLOAT_C(0.0))
            {
                ValBuf->u32 = 0;
            }
            else
            {
                ValBuf->u32 = EDSLIB_LDEXP(EDSLIB_FABS(Significand),24);

                ValBuf->u32 &= UINT32_C(0x7FFFFF);
                ValBuf->u32 |= (uint32_t)((Exponent + 126) & 0xFF) << 23;

                if (signbit(Significand))
                {
                    ValBuf->u32 |= UINT32_C(1) << 31; /* set the sign bit */
                }
            }

            ConvSize = sizeof(ValBuf->u32);
        }
        else
        {
            ConvSize = 0;
        }
    }
    else
    {
        /* This is an unimplemented conversion */
        ConvSize = 0;
    }

    return ConvSize;
}

static void EdsLib_Internal_DoBitwisePack(uint8_t *DstPtr, const uint8_t *SrcPtr, const EdsLib_DataTypeDB_Entry_t *DataDictPtr, uint32_t DstBitOffset)
{
    uint32_t ShiftRegister;
    uint32_t HighBitPos;
    uint32_t TotalBits;
    EdsLib_GenericValueUnion_t NumBuf;
    const uint8_t *LoadPtr;
    EdsLib_Internal_PackStyleInfo_t Pack;
    size_t ConvSize;

    LoadPtr = NULL;

    /*
     * General process of packing ints/floats
     *  1) convert native machine encoding into EDS-specified encoding:
     *     repack bits according to encoding scheme
     *     In many cases the EDS will specify encodings that match the
     *      machine encoding but are nonstandard bit widths (not 8/16/32 etc).
     *      In these cases the bits don't need repacking, just aligning
     *      These simple cases are:
     *          - Strings or other binary blobs
     *          - Unsigned Integers
     *          - Twos Complement Signed Integers
     *          - IEEE 754 Single/Double precision
     *     In other cases like MIL-STD-1750A the bits must be rearranged
     *      explicitly.
     *
     *  2) load bits into shift register from "native" memory (source)
     *     if EDS byte order matches in-memory byte order, then bytes are
     *          loaded in order (stride = 1)
     *     if EDS byte order does NOT match in-memory byte order, then bytes
     *          are loaded in reverse (stride = -1)
     *
     *     This handles the normal big/little endian, but oddballs like PDP
     *     mixed endian are not currently implemented.  But it is unlikely
     *     this will be needed and the current EDS spec does not
     *     specify anything other than big or little endian byte orders.
     *
     *  3) when shift register as at least one complete octet, write it to
     *      the output buffer.
     */

    /* Get the generalized setup for this conversion */
    if (!EdsLib_Internal_GetPackStyle(&Pack, DataDictPtr))
    {
        return;
    }

    /* Do conversion to intermediate type, if indicated */
    if (Pack.IntermediateType == EDSLIB_BASICTYPE_NONE ||
            Pack.IntermediateSize == 0)
    {
        LoadPtr = SrcPtr;
        ConvSize = DataDictPtr->SizeInfo.Bytes;
    }
    else
    {
        if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_SIGNED_INT)
        {
            ConvSize = EdsLib_Internal_DoSignedIntPack(&NumBuf, SrcPtr, DataDictPtr->SizeInfo.Bytes,
                    DataDictPtr->Detail.Number.Encoding, DataDictPtr->SizeInfo.Bits);
        }
        else if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_FLOAT)
        {
            ConvSize = EdsLib_Internal_DoFloatPack(&NumBuf, SrcPtr, DataDictPtr->SizeInfo.Bytes,
                    DataDictPtr->Detail.Number.Encoding, DataDictPtr->SizeInfo.Bits);
        }
        else
        {
            ConvSize = 0;
        }

        LoadPtr = NumBuf.BinaryData;
        if (ConvSize != Pack.IntermediateSize)
        {
            /* error: pack routine did not output
             * expected size object */
            NumBuf.BinaryData[0] = 0xFF;
            Pack.MemStride = 0;
        }
    }

    if (LoadPtr != NULL && (Pack.IntermediateShift > 0 || Pack.Invert))
    {
        EdsLib_ConstPtr_t Src = { .Addr = LoadPtr };

        switch(ConvSize)
        {
        case sizeof(Src.u->u8):
            NumBuf.u8 = Src.u->u8 << Pack.IntermediateShift;
            break;
        case sizeof(Src.u->u16):
            NumBuf.u16 = Src.u->u16 << Pack.IntermediateShift;
            break;
        case sizeof(Src.u->u32):
            NumBuf.u32 = Src.u->u32 << Pack.IntermediateShift;
            break;
        case sizeof(Src.u->u64):
            NumBuf.u64 = Src.u->u64 << Pack.IntermediateShift;
            break;
        default:
            /* error/invalid */
            break;
        }

        LoadPtr = NumBuf.BinaryData;

        if (Pack.Invert)
        {
            switch(ConvSize)
            {
            case sizeof(Src.u->u8):
                NumBuf.u8 ^= UINT8_C(0xFF);
                break;
            case sizeof(Src.u->u16):
                NumBuf.u16 ^= UINT16_C(0xFFFF);
                break;
            case sizeof(Src.u->u32):
                NumBuf.u32 ^= UINT32_C(0xFFFFFFFF);
                break;
            case sizeof(Src.u->u64):
                NumBuf.u64 ^= UINT64_C(0xFFFFFFFFFFFFFFFF);
                break;
            default:
                /* error/invalid */
                break;
            }
        }
    }


    /*
     * Prepare to copy the encoded value to the output buffer.
     *
     * At this point there exists one of four possibilities:
     *
     *  ---> An unsupported encoding/conversion was attempted
     *    This is indicated by:
     *       LocalSize set to zero (or fewer bits than what is required).
     *
     *
     *  ---> The value will be read direct from the source.
     *    This is indicated by:
     *       NumBuf.ValueType is set to EDSLIB_BASICTYPE_NONE,
     *    The NumBuf will not be loaded with any value yet.
     *    LocalSize represents the size from the original source, in bytes
     *    DataDictPtr->BasicType represents the actual datatype being copied.
     *
     *  ---> The value is packed into the NumBuf as an unsigned int.
     *    This is indicated by:
     *       NumBuf.ValueType is set to EDSLIB_BASICTYPE_UNSIGNED_INT,
     *    LocalSize represents the size of that integer, in bytes.
     *          (either 1, 2, 4, or 8)
     *    DataDictPtr->BasicType is no longer relevant
     *    The unsigned int will still be in machine byte order
     *
     *  ---> The value is packed into the NumBuf as a binary data blob.
     *    This is indicated by:
     *       NumBuf.ValueType is set to EDSLIB_BASICTYPE_BINARY,
     *    LocalSize represents the size of that blob, in bytes.
     *          (could be any value in range 0-16)
     *    DataDictPtr->BasicType is no longer relevant
     *    The binary blob will always be in "big-endian" byte order
     */

    /*
     * The intent is to load the local shift register from
     * bytes stored in local memory.  To optimize this
     * copy, the initial conditions are tuned depending on the
     * endianness of the native CPU, the endianness of the output,
     * and the type of data being packed.
     *
     * Note that if data is integer in nature (UNSIGNED_INT) then it
     * is justified such that the least significant bit of output is aligned
     * in the least significant bit of the buffer, but it will be in
     * machine byte order, so the address of the LSB is machine-dependent.
     *
     * Conversely if the data non integer e.g. is BINARY, then
     * it is justified such that the most significant bit of output
     * is aligned in the most significant bit of the buffer, and this
     * is always at the first memory address regardless of the native
     * machine endianness.
     */

    /* When reading in reverse order,
     * move to the highest address first
     */
    if (Pack.MemStride < 0)
    {
        LoadPtr += ConvSize - 1;
    }

    if (DstBitOffset == 0)
    {
        /* no existing bits to load */
        ShiftRegister = 0;
    }
    else
    {
        /* preload ShiftRegister with exiting bits in output buffer
         * This avoids clobbering any partial byte that was already packed */
        ShiftRegister = *DstPtr;
        ShiftRegister >>= 8 - DstBitOffset;
    }
    HighBitPos = DstBitOffset;
    TotalBits = HighBitPos + DataDictPtr->SizeInfo.Bits;

    /*
     * If the first octet is not a whole octet,
     * then read it accordingly.
     */

    if (Pack.LeadingPadBits > 0)
    {
        size_t RemainderBits = (Pack.LeadingPadBits & 0x07); /* number of pad bits remaining [0-7] */

        LoadPtr += (Pack.LeadingPadBits / 8) * Pack.MemStride; /* skip whole bytes */

        /* Load the entire byte, then right-shift out the padding */
        ShiftRegister = (ShiftRegister << 8) | *LoadPtr;
        ShiftRegister >>= RemainderBits;
        HighBitPos += 8 - RemainderBits; /* add number of REAL bits */

        LoadPtr += Pack.MemStride;   /* account for leading byte just loaded */
    }

    /* Now load entire bytes into the shift register */
    while(TotalBits > HighBitPos)
    {
        if (HighBitPos < 24)
        {
            /* Read 8 more bits of data from source into bottom of Shift Register */
            ShiftRegister = (ShiftRegister << 8) | *LoadPtr;
            HighBitPos += 8;
            LoadPtr += Pack.MemStride;
        }
        else
        {
            /* Output 8 bits of data from top of shift register to destination */
            HighBitPos -= 8;
            TotalBits -= 8;
            *DstPtr = ShiftRegister >> HighBitPos;
            ++DstPtr;
        }
    }

    /* Output any remaining full octets */
    while(TotalBits >= 8)
    {
        HighBitPos -= 8;
        TotalBits -= 8;
        *DstPtr = ShiftRegister >> HighBitPos;
        ++DstPtr;
    }

    /* Output any remaining partial octet */
    if (TotalBits > 0)
    {
        if (HighBitPos <= 8)
        {
            ShiftRegister <<= 8 - HighBitPos;
        }
        else
        {
            ShiftRegister >>= HighBitPos - 8;
        }
        /* avoid clobbering any bits _after_ the bits being written.
         * This is important when updating a field in the middle of
         * a packet, such as a CRC/Checksum. */
        ShiftRegister |= *DstPtr & ((1U << (8 - TotalBits)) - 1);
        *DstPtr = ShiftRegister;
        HighBitPos = 0;
    }
}

static void EdsLib_Internal_DoSignedIntUnpack(uint8_t *DstPtr, size_t DstSizeBytes, EdsLib_GenericValueUnion_t *ValBuf, EdsLib_NumberEncoding_Enum_t EncodingStyle, uint32_t EncodingBits)
{
    EdsLib_Ptr_t TempDst = { .Addr = DstPtr };
    EdsLib_Generic_UnsignedInt_t TempVal;

    switch(EncodingStyle)
    {
    case EDSLIB_NUMBERENCODING_SIGN_MAGNITUDE:
    {
        TempVal = ((EdsLib_Generic_UnsignedInt_t)1) << (EncodingBits-1);
        if (ValBuf->UnsignedInteger & TempVal)
        {
            /* convert to a negative signed */
            ValBuf->SignedInteger = 0 - (ValBuf->UnsignedInteger & (TempVal - 1));
        }
        else
        {
            ValBuf->SignedInteger = ValBuf->UnsignedInteger;
        }
        break;
    }
    case EDSLIB_NUMBERENCODING_ONES_COMPLEMENT:
    {
        TempVal = ((EdsLib_Generic_UnsignedInt_t)1) << (EncodingBits-1);
        if (ValBuf->UnsignedInteger & TempVal)
        {
            /* convert to a negative signed, and invert all bits */
            ValBuf->SignedInteger = 0 - (~ValBuf->UnsignedInteger & (TempVal - 1));
        }
        else
        {
            ValBuf->SignedInteger = ValBuf->UnsignedInteger;
        }
        break;
    }
    case EDSLIB_NUMBERENCODING_BCD_OCTET:
    {
        uint8_t *p = ValBuf->BinaryData;
        size_t len = EncodingBits / 8;
        TempVal = 0;
        while(len > 0)
        {
            if (*p < 10)
            {
                TempVal = (TempVal * 10) + *p;
            }
            ++p;
            --len;
        }
        if (ValBuf->StringData[0] == '-')
        {
            TempVal = -TempVal;
        }
        ValBuf->SignedInteger = TempVal;
        break;
    }
    case EDSLIB_NUMBERENCODING_BCD_PACKED:
    {
        uint8_t digit;
        size_t i = 0;
        TempVal = 0;

        while(i < EncodingBits)
        {
            digit = ValBuf->BinaryData[i >> 3];
            if (digit != '-')
            {
                digit >>= (~i & 4);
                digit &= 0x0F;
                if (digit < 10)
                {
                    TempVal = (TempVal * 10) + digit;
                }
                i += 4;
            }
        }

        if (ValBuf->StringData[0] == '-')
        {
            TempVal = -TempVal;
        }
        ValBuf->SignedInteger = TempVal;
        break;
    }
    default:
    {
        /* default encoding type is a pass thru.
         * This should work on twos complement machines */
        break;
    }
    }

    switch(DstSizeBytes)
    {
    case sizeof(TempDst.u->i8):
        TempDst.u->i8 = ValBuf->SignedInteger;
        break;
    case sizeof(TempDst.u->i16):
        TempDst.u->i16 = ValBuf->SignedInteger;
        break;
    case sizeof(TempDst.u->i32):
        TempDst.u->i32 = ValBuf->SignedInteger;
        break;
    case sizeof(TempDst.u->i64):
        TempDst.u->i64 = ValBuf->SignedInteger;
        break;
    default:
        /* should never happen, but avoid leaving garbage data into the output */
        memset(TempDst.Ptr, 0, DstSizeBytes);
        break;
    }
}

static void EdsLib_Internal_DoFloatUnpack(uint8_t *DstPtr, size_t DstSizeBytes, EdsLib_GenericValueUnion_t *ValBuf, EdsLib_NumberEncoding_Enum_t EncodingStyle, uint32_t EncodingBits)
{
    /*
     * All floating point notations (including the less common ones) express
     * the value in some form scientific notation, usually as a power of two.
     *
     * To allow conversion, the value will be broken into its constituent parts
     * (sign bit, exponent, and mantissa).  The values can then be adjusted
     * per the specified encoding style, and bit-packed using the format
     * specified by the EDS.
     *
     * Note these conversions are only intended for real numbers.  For exceptions
     * like NaN or Infinity it will currently encode a zero.  (this could be fixed
     * in a future version if there is a need for these exception values).
     */
    EdsLib_Ptr_t TempDst = { .Addr = DstPtr };
    size_t x;
    int Exponent = -1;
    int64_t MantissaBits = 0;
    EdsLib_Internal_Float_t Value = EDSLIB_FLOAT_C(0.0);

    if (EncodingStyle == EDSLIB_NUMBERENCODING_MILSTD_1750A)
    {
        /* Extract mantissa and exponent from 1750A packed bits */

        if (EncodingBits == 32 && ValBuf->u32 != 0)
        {
            /* 24 bits of mantissa as sign bit + 23 bits, 8 bits of exponent */
            Exponent = (ValBuf->u32 & UINT32_C(0x000000FF));
            MantissaBits = (ValBuf->u32 & UINT32_C(0xFFFFFF00));

            /* Interpolate mantissa to 64 bits -
             * align sign bit into MSB of int64_t */
            MantissaBits = (MantissaBits << 32);
        }
        else if (EncodingBits == 48 && ValBuf->u64 != 0)
        {
            /* 40 actual bits of mantissa, split between sign bit, 23 bits and 16 bits */
            Exponent = (ValBuf->u64 & UINT64_C(0x000000FF0000)) >> 16;
            MantissaBits =  (ValBuf->u64 & UINT64_C(0xFFFFFF000000)) >> 8;
            MantissaBits |= (ValBuf->u64 & UINT64_C(0x00000000FFFF));

            /* Interpolate mantissa to 64 bits -
             * align sign bit into MSB of int64_t */
            MantissaBits = (MantissaBits << 24);
        }

        if (Exponent >= 0)
        {
            Exponent -= (Exponent & 0x80) << 1; /* sign extend */
            Value = EDSLIB_LDEXP(MantissaBits, Exponent - 63);
        }
    }
    else if (EncodingStyle == EDSLIB_NUMBERENCODING_IEEE_754)
    {
        if (EncodingBits == 128)
        {
            /* The first two bytes are the sign and exponent */
            Exponent = ((ValBuf->BinaryData[0] & 0x7F) << 8) +
                    (ValBuf->BinaryData[1] & 0xFF);

            for (x=15; x >= 2; --x)
            {
                Value += ValBuf->BinaryData[x];
                Value = EDSLIB_LDEXP(Value,-8);
            }

            if (Value != EDSLIB_FLOAT_C(0.0) || Exponent != 0)
            {
                Value = EDSLIB_LDEXP(Value + EDSLIB_FLOAT_C(1.0), Exponent - 16383);
            }

            if (ValBuf->BinaryData[0] & 0x80)
            {
                Value = -Value;
            }
        }
        else if (EncodingBits == 64)
        {
            /* Interpret as uint64, mask out bits for IEEE double */
            MantissaBits = (ValBuf->u64 & UINT64_C(0xFFFFFFFFFFFFF));
            Exponent = (ValBuf->u64 >> 52) & 0x7FF;

            if (MantissaBits != 0 || Exponent != 0)
            {
                /* subtract 1076 = 1023 (bias) + 53 (number of mantissa bits) */
                Value = EDSLIB_LDEXP(MantissaBits, Exponent - 1076);
            }
        }
        else if (EncodingBits == 32)
        {
            /* Interpret as uint32, mask out bits for IEEE single */
            MantissaBits = (ValBuf->u32 & UINT32_C(0x7FFFFF));
            Exponent = (ValBuf->u32 >> 23) & 0xFF;

            if (MantissaBits != 0 || Exponent != 0)
            {
                /* subtract 151 = 127 (bias) + 24 (number of mantissa bits) */
                Value = EDSLIB_LDEXP(MantissaBits, Exponent - 151);
            }
        }
    }

    /* Now store the result into the correct-size output */
    if (DstSizeBytes == sizeof(TempDst.u->fpsgl))
    {
        TempDst.u->fpsgl = Value;
    }
    else if (DstSizeBytes == sizeof(TempDst.u->fpdbl))
    {
        TempDst.u->fpdbl = Value;
    }
    else if (DstSizeBytes == sizeof(TempDst.u->fpmax))
    {
        TempDst.u->fpmax = Value;
    }
}

static void EdsLib_Internal_DoBitwiseUnpack(uint8_t *DstPtr, const uint8_t *SrcPtr, const EdsLib_DataTypeDB_Entry_t *DataDictPtr, uint32_t SrcBitOffset)
{
    uint32_t ShiftRegister;
    uint32_t LowBitPos;
    uint32_t TotalBits;
    EdsLib_GenericValueUnion_t NumBuf;
    size_t FillBits;
    uint8_t *StorePtr;
    EdsLib_Internal_PackStyleInfo_t Pack;
    size_t ConvSize;


    /*
     * General process of unpacking ints/floats
     *
     * Unpacking is generally the inverse of packing, but the
     * details make it a bit different.  There is an extra step
     * as the decoding operation must first be "predicted" before
     * data is known, then actually decoded after the data is ready.
     *
     *  1) determine whether value will require additional decoding after
     *      reading the bits.  This means whether or not the EDS encoding
     *      is compatible with the machine encoding.  It is essentially the
     *      same as the first step of packing, but without the actual data.
     *
     *     If additional decoding is needed, then data must be written
     *     to the intermediate storage buffer first.  If no additional
     *     decoding is needed, then data can be written directly to output.
     *
     *     These simple cases (direct output) are:
     *          - Strings or other binary blobs
     *          - Unsigned Integers
     *          - Twos Complement Signed Integers
     *          - IEEE 754 Single/Double precision
     *
     *
     *  2) load bits into shift register from packed buffer (source)
     *
     *     This handles the normal big/little endian, but oddballs like PDP
     *     mixed endian are not currently implemented.  But it is unlikely
     *     this will be needed and the current EDS spec does not
     *     specify anything other than big or little endian byte orders.
     *
     *  3) when shift register has at least one complete octet, store it to
     *      the native memory buffer.
     *     if EDS byte order matches in-memory byte order, then bytes are
     *          stored in order (stride = 1)
     *     if EDS byte order does NOT match in-memory byte order, then bytes
     *          stored in reverse (stride = -1)
     *
     *  4) convert EDS-specified encoding into native machine encoding:
     *     This is to actually do the decoding that was anticipated in step 0,
     *     now that the data is available.
     *
     */
    if (!EdsLib_Internal_GetPackStyle(&Pack, DataDictPtr))
    {
        return;
    }

    /* Do conversion to intermediate type, if indicated */
    if (Pack.IntermediateType != EDSLIB_BASICTYPE_NONE &&
            Pack.IntermediateSize != 0)
    {
        StorePtr = NumBuf.BinaryData;
        ConvSize = Pack.IntermediateSize;
    }
    else if (Pack.Invert || Pack.IntermediateShift > 0)
    {
        /* an intermediate shift is required, so store to
         * the temporary buffer, but use the
         */
        StorePtr = NumBuf.BinaryData;
        ConvSize = DataDictPtr->SizeInfo.Bytes;
    }
    else
    {
        StorePtr = DstPtr;
        ConvSize = DataDictPtr->SizeInfo.Bytes;
    }


    ShiftRegister = 0;
    LowBitPos = 24 + SrcBitOffset;
    TotalBits = SrcBitOffset + DataDictPtr->SizeInfo.Bits;

    /*
     * When reading in reverse order,
     * move to the highest address first.
     */
    if (Pack.MemStride < 0)
    {
        StorePtr += ConvSize - 1;
    }

    FillBits = 0;
    while ((FillBits + 8) <= Pack.LeadingPadBits)
    {
        *StorePtr = 0;
        StorePtr += Pack.MemStride;
        FillBits += 8;
    }

    ConvSize *= 8;
    while (FillBits < ConvSize)
    {
        /* Load an octet from the bitstream */
        if (LowBitPos > 8)
        {
            if (TotalBits > 0)
            {
                ShiftRegister |= ((uint32_t)*SrcPtr) << (LowBitPos - 8);
                ++SrcPtr;

                if (TotalBits < 8)
                {
                    /* This could have gotten some extra bits beyond the last valid bit
                     * if so, clear them out of the shift register here */
                    ShiftRegister &= ~((UINT32_C(1) << (LowBitPos - TotalBits)) - 1);
                    TotalBits = 0;
                    LowBitPos = 0;
                }
                else
                {
                    TotalBits -= 8;
                    LowBitPos -= 8;
                }
            }
            else
            {
                LowBitPos = 0;
            }
        }

        if (LowBitPos <= 16)
        {
            if (FillBits < Pack.LeadingPadBits)
            {
                /* store a partial octet -
                 * the valid bits aligned to the MSB, and clear any LSBs */
                *StorePtr = (ShiftRegister >> 16) &
                        (((UINT32_C(1) << (Pack.LeadingPadBits - FillBits)) - 1) ^ 0xFF);

                FillBits += 8;
                ShiftRegister <<= (FillBits - Pack.LeadingPadBits);
                LowBitPos += (FillBits - Pack.LeadingPadBits);
            }
            else
            {
                /* store a whole octet */
                *StorePtr = ShiftRegister >> 16;
                ShiftRegister <<= 8;
                LowBitPos += 8;
                FillBits += 8;
            }
            StorePtr += Pack.MemStride;
        }
    }

    /* Un-shift the numeric data if indicated */
    if (Pack.Invert || Pack.IntermediateShift > 0)
    {
        EdsLib_Ptr_t ShiftDst;
        EdsLib_BasicType_t ShiftType;

        if (Pack.IntermediateType != EDSLIB_BASICTYPE_NONE)
        {
            ShiftDst.u = &NumBuf;
            ShiftType = Pack.IntermediateType;
        }
        else
        {
            ShiftType = DataDictPtr->BasicType;
            ShiftDst.Addr = DstPtr;
        }

        if (Pack.Invert)
        {
            switch(ConvSize)
            {
            case 8:
                NumBuf.u8 = ~NumBuf.u8;
                break;
            case 16:
                NumBuf.u16 = ~NumBuf.u16;
                break;
            case 32:
                NumBuf.u32 = ~NumBuf.u32;
                break;
            case 64:
                NumBuf.u64 = ~NumBuf.u64;
                break;
            default:
                break;
            }
        }

        /*
         * When unpacking there must be a distinction between
         * signed or unsigned (in contrast to packing where there is
         * need to differentiate).
         */
        if (ShiftType == EDSLIB_BASICTYPE_SIGNED_INT)
        {
            /*
             * Shift as a signed integer.
             * This should be an "arithmetic" shift that extends the sign bit
             * Using the >> operator is technically implementation-defined
             * But every machine should implement this correctly.
             * The alternative is to divide by 1<<Shift
             */
            switch(ConvSize)
            {
            case 8:
                ShiftDst.u->i8 = NumBuf.i8 >> Pack.IntermediateShift;
                break;
            case 16:
                ShiftDst.u->i16 = NumBuf.i16 >> Pack.IntermediateShift;
                break;
            case 32:
                ShiftDst.u->i32 = NumBuf.i32 >> Pack.IntermediateShift;
                break;
            case 64:
                ShiftDst.u->i64 = NumBuf.i64 >> Pack.IntermediateShift;
                break;
            default:
                break;
            }
        }
        else switch(ConvSize)  /* anything else is unsigned shift */
        {
        case 8:
            ShiftDst.u->u8 = NumBuf.u8 >> Pack.IntermediateShift;
            break;
        case 16:
            ShiftDst.u->u16 = NumBuf.u16 >> Pack.IntermediateShift;
            break;
        case 32:
            ShiftDst.u->u32 = NumBuf.u32 >> Pack.IntermediateShift;
            break;
        case 64:
            ShiftDst.u->u64 = NumBuf.u64 >> Pack.IntermediateShift;
            break;
        default:
            break;
        }
    }

    /*
     * Do Final unpack stage, if indicated
     * This does the actual conversion of the packed value into native encoding,
     * for those cases where it did not match
     */
    if (Pack.IntermediateType != EDSLIB_BASICTYPE_NONE)
    {
        if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_SIGNED_INT)
        {
            EdsLib_Internal_DoSignedIntUnpack(DstPtr, DataDictPtr->SizeInfo.Bytes, &NumBuf,
                    DataDictPtr->Detail.Number.Encoding, DataDictPtr->SizeInfo.Bits);
        }
        else if (DataDictPtr->BasicType == EDSLIB_BASICTYPE_FLOAT)
        {
            EdsLib_Internal_DoFloatUnpack(DstPtr, DataDictPtr->SizeInfo.Bytes, &NumBuf,
                    DataDictPtr->Detail.Number.Encoding, DataDictPtr->SizeInfo.Bits);
        }
    }
}

static EdsLib_Iterator_Rc_t EdsLib_DataTypePackUnpack_Callback(const EdsLib_DatabaseObject_t *GD,
        EdsLib_Iterator_CbType_t CbType,
        const EdsLib_DataTypeIterator_StackEntry_t *CbInfo,
        void *OpaqueArg)
{
    EdsLib_DataTypePackUnpack_ControlBlock_t *Base = (EdsLib_DataTypePackUnpack_ControlBlock_t *)OpaqueArg;
    const uint8_t *SrcPtr;
    uint8_t *DstPtr;
    EdsLib_PackAction_t PackAction;
    uint32_t AlignBits;
    bool IsByteOrderMatch;
    bool IsPacked;

    /*
     * Generally we do not care about START/END callbacks -
     * however on the START callback it is a useful chance to
     * verify that we have enough buffer space to store the object,
     * and to clear the destination memory before writing into it.
     *
     * Note this is only done on the first top-level START callback where
     * the refobj matches the controlblock refobj.  This reflects the largest
     * object being handled, all other START callbacks will be for sub-objects
     * inside this.
     */
    if (CbType == EDSLIB_ITERATOR_CBTYPE_START &&
            Base->RefObj.AppIndex == CbInfo->Details.RefObj.AppIndex &&
            Base->RefObj.TypeIndex == CbInfo->Details.RefObj.TypeIndex)
    {
        if (Base->MaxSize.Bytes < CbInfo->DataDictPtr->SizeInfo.Bytes ||
                Base->MaxSize.Bits < CbInfo->DataDictPtr->SizeInfo.Bits)
        {
            Base->Status = EDSLIB_BUFFER_SIZE_ERROR;
            return EDSLIB_ITERATOR_RC_STOP;
        }

        /*
         * Clear out the target buffer before writing new data into it.
         *
         * In certain circumstances the data needs to be packed/unpacked in
         * multiple passes, for instance when the data needs to be identified externally
         * before continuing the operation.
         *
         * In these cases the clearing must only be done on the area that has not
         * already been processed, to avoid clobbering data that was already packed.
         *
         * When packing, use the "bits" value, and when unpacking use the "bytes" value
         * as the reference for where to begin clearing
         */
        uint32_t StartOffset;
        uint32_t EndOffset;
        switch(Base->OperMode)
        {
        case EDSLIB_BITPACK_OPERMODE_PACK:
        {
            StartOffset = (Base->ProcessedSize.Bits + 7) / 8;
            EndOffset = (CbInfo->EndOffset.Bits + 7) / 8;
            break;
        }
        case EDSLIB_BITPACK_OPERMODE_UNPACK:
        {
            StartOffset = Base->ProcessedSize.Bytes;
            EndOffset = CbInfo->EndOffset.Bytes;
            break;
        }
        default:
        {
            StartOffset = 0;
            EndOffset = 0;
            break;
        }
        }

        if (StartOffset < EndOffset)
        {
            DstPtr = Base->DestBasePtr;
            DstPtr += StartOffset;
            memset(DstPtr, 0, EndOffset - StartOffset);
        }

        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    /*
     * Any other callback types other than member, just continue on.
     * Also padding entries are irrelevant here, just skip them.
     */
    if (CbType != EDSLIB_ITERATOR_CBTYPE_MEMBER ||
            CbInfo->Details.EntryType == EDSLIB_ENTRYTYPE_CONTAINER_PADDING_ENTRY)
    {
        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    /*
     * Special fields cannot be fully handled right now --
     * the value within the user-supplied buffer cannot be used directly and
     * the correct value is not known at the moment
     *
     * When unpacking, the value of these fields can be validated/verified by the fixup code,
     * and in some cases these could serve as additional error control fields.  Therefore
     * in the case of unpacking the value _will_ be copied out even if is overwritten/fixed later.
     *
     * Conversely when packing the value could actually interfere with the correct calculation,
     * for instance in the case of error control fields the data should be set to all zero as
     * a prerequisite to calculating the value.
     */
    if (Base->OperMode == EDSLIB_BITPACK_OPERMODE_PACK &&
            (CbInfo->Details.EntryType == EDSLIB_ENTRYTYPE_CONTAINER_ERROR_CONTROL_ENTRY ||
             CbInfo->Details.EntryType == EDSLIB_ENTRYTYPE_CONTAINER_LENGTH_ENTRY ||
             CbInfo->Details.EntryType == EDSLIB_ENTRYTYPE_CONTAINER_FIXED_VALUE_ENTRY))
    {
        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    /*
     * In certain circumstances the data needs to be packed/unpacked in
     * multiple passes, for instance when the data needs to be identified externally
     * before continuing the operation.
     *
     * This means that the field has already been packed and it should be skipped entirely.
     */
    if (CbInfo->EndOffset.Bits <= Base->ProcessedSize.Bits ||
            CbInfo->EndOffset.Bytes <= Base->ProcessedSize.Bytes)
    {
        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    /*
     * Determine if this field is a candidate for optimized handling, i.e. direct copy.
     *
     * If the data type is exactly equivalent to a C type with no embedded padding
     * or unused bits, and (in the case of structures) has no special element types,
     * then this flag should be set.  This was determined by the EDS tool at compile time.
     */
    IsPacked = (CbInfo->DataDictPtr->Flags & EDSLIB_DATATYPE_FLAG_PACKED_MASK) != 0;

    /*
     * Determine if the native byte-ordering matches that of the message format (typically big-endian)
     * This determines if resulting bytes need to be inverted in memory or if they can be copied in-order
     */
    IsByteOrderMatch = (CbInfo->DataDictPtr->Flags & EDSLIB_DATATYPE_FLAG_PACKED_MASK) == EDSLIB_NATIVE_BYTE_PACK;

    /*
     * Determine if the packed representation starts on a byte boundary
     * This determines if a bitwise shift-register must be used to pack the data, or if it can be
     * copied as bytes (the latter obviously being more efficient)
     */
    AlignBits = CbInfo->StartOffset.Bits & 0x07;

    switch(CbInfo->DataDictPtr->BasicType)
    {
    case EDSLIB_BASICTYPE_CONTAINER:
    case EDSLIB_BASICTYPE_ARRAY:
    {
        /*
         * Optimization:
         * 1) if the machine encoding is the same as the packed encoding
         * 2) if the entire sub-structure does not contain any padding
         * 3) The packed bits start on a byte boundary (so no shifting is required)
         *
         * THEN -- this can be a simple byte-wise memory copy operation, which will
         * be much more efficient than iterating over all the sub-structure contents
         */
        if (IsByteOrderMatch && IsPacked && AlignBits == 0)
        {
            PackAction = EDSLIB_PACKACTION_BYTECOPY_STRAIGHT;
        }
        else
        {
            PackAction = EDSLIB_PACKACTION_SUBCOMPONENTS;
        }
        break;
    }
    case EDSLIB_BASICTYPE_BINARY:
    {
        if (AlignBits == 0)
        {
            PackAction = EDSLIB_PACKACTION_BYTECOPY_STRAIGHT;
        }
        else
        {
            PackAction = EDSLIB_PACKACTION_BITPACK;
        }
        break;
    }
    case EDSLIB_BASICTYPE_SIGNED_INT:
    case EDSLIB_BASICTYPE_UNSIGNED_INT:
    case EDSLIB_BASICTYPE_FLOAT:
    {
        if (IsPacked && AlignBits == 0)
        {
            if (IsByteOrderMatch)
            {
                PackAction = EDSLIB_PACKACTION_BYTECOPY_STRAIGHT;
            }
            else
            {
                PackAction = EDSLIB_PACKACTION_BYTECOPY_INVERT;
            }
        }
        else
        {
            PackAction = EDSLIB_PACKACTION_BITPACK;
        }
        break;
    }
    default:
    {
        PackAction = EDSLIB_PACKACTION_NONE;
        break;
    }
    }

    if (PackAction == EDSLIB_PACKACTION_NONE)
    {
        /*
         * nothing to do
         */
        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    if (PackAction == EDSLIB_PACKACTION_SUBCOMPONENTS)
    {
        /*
         * If the structure contains sub-components then the iterator
         * must dig down into the sub-components
         */
        return EDSLIB_ITERATOR_RC_DESCEND;
    }


    SrcPtr = Base->SourceBasePtr;
    DstPtr = Base->DestBasePtr;

    if (Base->OperMode == EDSLIB_BITPACK_OPERMODE_PACK)
    {
        SrcPtr += CbInfo->StartOffset.Bytes;
        DstPtr += CbInfo->StartOffset.Bits / 8;
    }
    else if (Base->OperMode == EDSLIB_BITPACK_OPERMODE_UNPACK)
    {
        SrcPtr += CbInfo->StartOffset.Bits / 8;
        DstPtr += CbInfo->StartOffset.Bytes;
    }
    else
    {
        return EDSLIB_ITERATOR_RC_STOP;
    }

    switch(PackAction)
    {
    case EDSLIB_PACKACTION_BYTECOPY_STRAIGHT:
    {
        /* simplest case: just use the standard library memcpy routine */
        memcpy(DstPtr, SrcPtr, CbInfo->DataDictPtr->SizeInfo.Bytes);
        break;
    }
    case EDSLIB_PACKACTION_BYTECOPY_INVERT:
    {
        /* need to invert byte order while copying, so use custom routine */
        uintptr_t Size = CbInfo->DataDictPtr->SizeInfo.Bytes;
        DstPtr += Size;
        while(Size > 0)
        {
            --DstPtr;
            *DstPtr = *SrcPtr;
            ++SrcPtr;
            --Size;
        }
        break;
    }
    case EDSLIB_PACKACTION_BITPACK:
    {
        /* This depends on whether packing or unpacking */
        if (Base->OperMode == EDSLIB_BITPACK_OPERMODE_PACK)
        {
            EdsLib_Internal_DoBitwisePack(DstPtr, SrcPtr, CbInfo->DataDictPtr, AlignBits);
        }
        else
        {
            EdsLib_Internal_DoBitwiseUnpack(DstPtr, SrcPtr, CbInfo->DataDictPtr, AlignBits);
        }
        break;
    }
    default:
    {
        break;
    }
    }

    return EDSLIB_ITERATOR_RC_CONTINUE;
}

void EdsLib_DataTypePackUnpack_Impl(const EdsLib_DatabaseObject_t *GD, EdsLib_DataTypePackUnpack_ControlBlock_t *PackState)
{
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
    const void *NativeBuffer;
    EdsLib_DatabaseRef_t NextBaseObj;
    int32_t Status;

    EDSLIB_DECLARE_ITERATOR_CB(IteratorState,
            EDSLIB_ITERATOR_MAX_DEEP_DEPTH,
            EdsLib_DataTypePackUnpack_Callback,
            PackState);

    Status = EDSLIB_SUCCESS;

    if (PackState->OperMode == EDSLIB_BITPACK_OPERMODE_PACK)
    {
        NativeBuffer = PackState->SourceBasePtr;
    }
    else
    {
        NativeBuffer = PackState->DestBasePtr;
    }

    NextBaseObj = PackState->RefObj;
    while(1)
    {
        DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &NextBaseObj);
        if (DataDictPtr == NULL)
        {
            break;
        }

        if (PackState->MaxSize.Bytes < DataDictPtr->SizeInfo.Bytes ||
                PackState->MaxSize.Bits < DataDictPtr->SizeInfo.Bits)
        {
            PackState->Status = EDSLIB_BUFFER_SIZE_ERROR;
            break;
        }

        EDSLIB_RESET_ITERATOR_FROM_REFOBJ(IteratorState, NextBaseObj);
        Status = EdsLib_DataTypeIterator_Impl(GD, &IteratorState.Cb);
        if (PackState->Status != EDSLIB_SUCCESS)
        {
            break;
        }

        if (Status != EDSLIB_SUCCESS)
        {
            PackState->Status = Status;
            break;
        }

        /* Record the depth that has been successfully packed */
        PackState->ProcessedSize = DataDictPtr->SizeInfo;
        PackState->RefObj = NextBaseObj;

        /*
         * Find the next level of encapsulation
         * Note this must use the "native" representation, so the buffer which contains
         * native data will be the source when packing or destination when unpacking
         */
        Status = EdsLib_DataTypeIdentifyBuffer_Impl(GD, DataDictPtr, NativeBuffer, NULL, &NextBaseObj);
        if (Status != EDSLIB_SUCCESS)
        {
            /* Not further derived; normal stop condition */
            break;
        }


    }

    if (PackState->ProcessedSize.Bits == 0 && PackState->Status == EDSLIB_SUCCESS)
    {
        PackState->Status = EDSLIB_INCOMPLETE_DB_OBJECT;
    }
}

EdsLib_Iterator_Rc_t EdsLib_PackedObject_PostProc_Callback(const EdsLib_DatabaseObject_t *GD,
        EdsLib_Iterator_CbType_t CbType,
        const EdsLib_DataTypeIterator_StackEntry_t *CbInfo,
        void *OpaqueArg)
{
    EdsLib_PackedPostProc_ControlBlock_t *Base = (EdsLib_PackedPostProc_ControlBlock_t *)OpaqueArg;
    EdsLib_GenericValueBuffer_t ScratchBuf;

    /*
     * Any other callback types other than member, just continue on.
     * Also padding entries are irrelevant here, just skip them.
     */
    if (CbType != EDSLIB_ITERATOR_CBTYPE_MEMBER)
    {
        /* save off the top-level data dictionary for future reference */
        if (CbType == EDSLIB_ITERATOR_CBTYPE_START && Base->BaseDictPtr == NULL)
        {
            Base->BaseDictPtr = CbInfo->DataDictPtr;
        }
        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    if(CbInfo->DataDictPtr->NumSubElements > 0)
    {
        return EDSLIB_ITERATOR_RC_DESCEND;
    }

    if (Base->BaseDictPtr == NULL)
    {
        Base->Status = EDSLIB_INCOMPLETE_DB_OBJECT;
        return EDSLIB_ITERATOR_RC_STOP;
    }

    ScratchBuf.ValueType = EDSLIB_BASICTYPE_NONE;
    switch (CbInfo->Details.EntryType)
    {
    case EDSLIB_ENTRYTYPE_CONTAINER_LENGTH_ENTRY:
    {
        ScratchBuf.Value.SignedInteger = (Base->BaseDictPtr->SizeInfo.Bits + 7) / 8;
        ScratchBuf.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
        if (CbInfo->Details.HandlerArg.IntegerCalibrator.Reverse)
        {
            ScratchBuf.Value.SignedInteger =
                    CbInfo->Details.HandlerArg.IntegerCalibrator.Reverse(ScratchBuf.Value.SignedInteger);
        }
        break;
    }
    case EDSLIB_ENTRYTYPE_CONTAINER_ERROR_CONTROL_ENTRY:
    {
        /* error control MUST be calculated last, since
         * other fields like fixed value/lengths can affect the result.
         * position and metadata info is saved for later handling.
         *
         * note this means there can be only _one_ such error control field, and
         * if multiple fields are defined then only the last one will be valid.
         *
         * this actually has to be the case since the contents of one would affect
         * the validity of the other.
         */
        Base->ErrorCtlType = CbInfo->Details.HandlerArg.ErrorControl;
        Base->ErrorCtlDictPtr = CbInfo->DataDictPtr;
        Base->ErrorCtlOffsetBits = CbInfo->StartOffset.Bits;
        break;
    }
    case EDSLIB_ENTRYTYPE_CONTAINER_FIXED_VALUE_ENTRY:
    {
        if (CbInfo->DataDictPtr->BasicType == EDSLIB_BASICTYPE_BINARY)
        {
            strncpy(ScratchBuf.Value.StringData, CbInfo->Details.HandlerArg.FixedString, sizeof(ScratchBuf.Value.StringData));
            ScratchBuf.ValueType = EDSLIB_BASICTYPE_BINARY;
        }
        else if (CbInfo->DataDictPtr->BasicType == EDSLIB_BASICTYPE_SIGNED_INT)
        {
            ScratchBuf.Value.SignedInteger = CbInfo->Details.HandlerArg.FixedInteger;
            ScratchBuf.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
        }
        else if (CbInfo->DataDictPtr->BasicType == EDSLIB_BASICTYPE_UNSIGNED_INT)
        {
            ScratchBuf.Value.UnsignedInteger = CbInfo->Details.HandlerArg.FixedUnsigned;
            ScratchBuf.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
        }
        break;
    }
    default:
        /* do nothing to the field */
        break;
    }

    if (ScratchBuf.ValueType != EDSLIB_BASICTYPE_NONE)
    {
        EdsLib_ConstPtr_t TempSrc;
        EdsLib_Ptr_t TempDst;

        if (CbInfo->DataDictPtr->BasicType != EDSLIB_BASICTYPE_BINARY)
        {
            /* Value must be stored to intermediate location before packing */
            TempDst.u = &ScratchBuf.Value;
            EdsLib_DataTypeStore_Impl(TempDst, &ScratchBuf, CbInfo->DataDictPtr);
        }

        TempSrc.u = &ScratchBuf.Value;
        TempDst.Ptr = Base->BasePtr;
        TempDst.Addr += CbInfo->StartOffset.Bits / 8;
        EdsLib_Internal_DoBitwisePack(TempDst.Addr, TempSrc.Addr, CbInfo->DataDictPtr, CbInfo->StartOffset.Bits & 0x07);
    }

    return EDSLIB_ITERATOR_RC_CONTINUE;
}

EdsLib_Iterator_Rc_t EdsLib_NativeObject_PostProc_Callback(const EdsLib_DatabaseObject_t *GD,
        EdsLib_Iterator_CbType_t CbType,
        const EdsLib_DataTypeIterator_StackEntry_t *CbInfo,
        void *OpaqueArg)
{
    EdsLib_NativePostProc_ControlBlock_t *Base = (EdsLib_NativePostProc_ControlBlock_t *)OpaqueArg;
    EdsLib_GenericValueBuffer_t ScratchBuf;
    EdsLib_GenericValueBuffer_t ExpectedValue;
    EdsLib_GenericValueBuffer_t InitValue;
    EdsLib_ConstPtr_t TempSrc;
    EdsLib_Ptr_t TempDst;

    /*
     * Any other callback types other than member, just continue on.
     * Also padding entries are irrelevant here, just skip them.
     */
    if (CbType != EDSLIB_ITERATOR_CBTYPE_MEMBER)
    {
        /* save off the top-level data dictionary for future reference */
        if (CbType == EDSLIB_ITERATOR_CBTYPE_START && Base->BaseDictPtr == NULL)
        {
            Base->BaseDictPtr = CbInfo->DataDictPtr;
        }
        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    if(CbInfo->DataDictPtr->NumSubElements > 0)
    {
        return EDSLIB_ITERATOR_RC_DESCEND;
    }

    if (Base->BaseDictPtr == NULL)
    {
        return EDSLIB_ITERATOR_RC_STOP;
    }

    memset(&ExpectedValue, 0, sizeof(ExpectedValue));
    memset(&InitValue, 0, sizeof(InitValue));

    switch (CbInfo->Details.EntryType)
    {
    case EDSLIB_ENTRYTYPE_CONTAINER_LENGTH_ENTRY:
    {
        /* If PackedPtr is supplied, will verify value in existing field */
        if (Base->PackedPtr != NULL)
        {
            ExpectedValue.Value.SignedInteger = (Base->BaseDictPtr->SizeInfo.Bits + 7) / 8;
            ExpectedValue.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
            if (CbInfo->Details.HandlerArg.IntegerCalibrator.Reverse)
            {
                ExpectedValue.Value.SignedInteger =
                        CbInfo->Details.HandlerArg.IntegerCalibrator.Reverse(ExpectedValue.Value.SignedInteger);
            }
        }

        /*
         * Initialization value for native object:
         *
         * Many historical applications assume that the length field
         * in the native struct will be based on sizeof(struct).
         *
         * But In the EDS world the encoded and decoded sizes may
         * be different.
         *
         * So in this case the InitValue for the native object is different than
         * the ExpectedValue in a packed object.
         */

        if (CbInfo->Details.HandlerArg.IntegerCalibrator.Reverse &&
                (Base->RecomputeFields & EDSLIB_DATATYPEDB_RECOMPUTE_LENGTH))
        {
            InitValue.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
            InitValue.Value.SignedInteger =
                    CbInfo->Details.HandlerArg.IntegerCalibrator.Reverse(Base->BaseDictPtr->SizeInfo.Bytes);
        }
        break;
    }
    case EDSLIB_ENTRYTYPE_CONTAINER_ERROR_CONTROL_ENTRY:
    {
        /* If PackedPtr is supplied, will verify value in existing field */
        if (Base->PackedPtr != NULL)
        {
            ExpectedValue.Value.UnsignedInteger = EdsLib_ErrorControlCompute(
                    CbInfo->Details.HandlerArg.ErrorControl, Base->PackedPtr,
                    Base->BaseDictPtr->SizeInfo.Bits, CbInfo->StartOffset.Bits);
            ExpectedValue.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
        }

        if (Base->RecomputeFields & EDSLIB_DATATYPEDB_RECOMPUTE_ERRORCONTROL)
        {
            InitValue.Value.UnsignedInteger = EdsLib_ErrorControlCompute(
                    CbInfo->Details.HandlerArg.ErrorControl, Base->NativePtr,
                    Base->BaseDictPtr->SizeInfo.Bytes * 8, CbInfo->StartOffset.Bytes * 8);
            InitValue.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
        }
        break;
    }
    case EDSLIB_ENTRYTYPE_CONTAINER_FIXED_VALUE_ENTRY:
    {
        /* For fixed values these do not change between the packed/encoded
         * value and the native/decoded value.   */
        if (Base->PackedPtr != NULL ||
                (Base->RecomputeFields & EDSLIB_DATATYPEDB_RECOMPUTE_ERRORCONTROL))
        {
            if (CbInfo->DataDictPtr->BasicType == EDSLIB_BASICTYPE_BINARY)
            {
                strncpy(InitValue.Value.StringData, CbInfo->Details.HandlerArg.FixedString,
                        sizeof(InitValue.Value.StringData));
                InitValue.ValueType = EDSLIB_BASICTYPE_BINARY;
            }
            else if (CbInfo->DataDictPtr->BasicType == EDSLIB_BASICTYPE_SIGNED_INT)
            {
                InitValue.Value.SignedInteger = CbInfo->Details.HandlerArg.FixedInteger;
                InitValue.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
            }
            else if (CbInfo->DataDictPtr->BasicType == EDSLIB_BASICTYPE_UNSIGNED_INT)
            {
                InitValue.Value.UnsignedInteger = CbInfo->Details.HandlerArg.FixedUnsigned;
                InitValue.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
            }
            else
            {
                Base->Status = EDSLIB_FIELD_MISMATCH;
            }

            /* ExpectedValue is the same as InitValue if the PackedPtr is set. */
            if (Base->PackedPtr != NULL)
            {
                ExpectedValue = InitValue;
            }

            /* Do _not_ do initialize (clear InitValue) unless the flag was set */
            if ((Base->RecomputeFields & EDSLIB_DATATYPEDB_RECOMPUTE_ERRORCONTROL) == 0)
            {
                InitValue.ValueType = EDSLIB_BASICTYPE_NONE;
            }
        }
        break;
    }
    default:
        /* do nothing to the field */
        break;
    }

    if (ExpectedValue.ValueType != EDSLIB_BASICTYPE_NONE)
    {
        EdsLib_DataTypeConvert(&ExpectedValue, CbInfo->DataDictPtr->BasicType);

        /*
         * Load the value that was actually in the unpacked object
         */
        TempSrc.Ptr = Base->NativePtr;
        TempSrc.Addr += CbInfo->StartOffset.Bytes;
        EdsLib_DataTypeLoad_Impl(&ScratchBuf, TempSrc, CbInfo->DataDictPtr);

        if (ScratchBuf.ValueType != ExpectedValue.ValueType)
        {
            ExpectedValue.Value.Boolean = false;
        }
        else switch(ExpectedValue.ValueType)
        {
        case EDSLIB_BASICTYPE_SIGNED_INT:
            ExpectedValue.Value.Boolean = (ScratchBuf.Value.SignedInteger == ExpectedValue.Value.SignedInteger);
            break;
        case EDSLIB_BASICTYPE_UNSIGNED_INT:
            ExpectedValue.Value.Boolean = (ScratchBuf.Value.UnsignedInteger == ExpectedValue.Value.UnsignedInteger);
            break;
        case EDSLIB_BASICTYPE_FLOAT:
            ExpectedValue.Value.Boolean = (ScratchBuf.Value.FloatingPoint == ExpectedValue.Value.FloatingPoint);
            break;
        case EDSLIB_BASICTYPE_BINARY:
            ExpectedValue.Value.Boolean = memcmp(ScratchBuf.Value.BinaryData,
                    ExpectedValue.Value.BinaryData, CbInfo->DataDictPtr->SizeInfo.Bytes) == 0;
            break;
        default:
            ExpectedValue.Value.Boolean = false;
            break;
        }

        if (!ExpectedValue.Value.Boolean)
        {
            if (CbInfo->Details.EntryType == EDSLIB_ENTRYTYPE_CONTAINER_ERROR_CONTROL_ENTRY)
            {
                Base->Status = EDSLIB_ERROR_CONTROL_MISMATCH;
            }
            else
            {
                Base->Status = EDSLIB_FIELD_MISMATCH;
            }
        }
    }

    if (InitValue.ValueType != EDSLIB_BASICTYPE_NONE)
    {
        /* Corrected Value can be stored directly to the native buffer */
        TempDst.Ptr = Base->NativePtr;
        TempDst.Addr += CbInfo->StartOffset.Bytes;
        EdsLib_DataTypeStore_Impl(TempDst, &InitValue, CbInfo->DataDictPtr);
    }

    return EDSLIB_ITERATOR_RC_CONTINUE;
}

void EdsLib_UpdateErrorControlField(const EdsLib_DataTypeDB_Entry_t *ErrorCtlDictPtr, void *PackedObject,
        uint32_t TotalBitSize, EdsLib_ErrorControlType_t ErrorCtlType, uint32_t ErrorCtlOffsetBits)
{
    EdsLib_ConstPtr_t TempSrc;
    EdsLib_Ptr_t TempDst;
    EdsLib_GenericValueBuffer_t ValBuf;

    ValBuf.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
    ValBuf.Value.UnsignedInteger = EdsLib_ErrorControlCompute(ErrorCtlType,
            PackedObject, TotalBitSize, ErrorCtlOffsetBits);

    /* Value must be stored to intermediate location before packing */
    TempDst.u = &ValBuf.Value;
    EdsLib_DataTypeStore_Impl(TempDst, &ValBuf, ErrorCtlDictPtr);

    TempSrc.Ptr = TempDst.Ptr;
    TempDst.Ptr = PackedObject;
    TempDst.Addr += ErrorCtlOffsetBits / 8;
    EdsLib_Internal_DoBitwisePack(TempDst.Addr, TempSrc.Addr, ErrorCtlDictPtr, ErrorCtlOffsetBits & 0x07);
}

