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
 * \file     edslib_displaydb_stringconv.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Utility functions to convert EDS data structure members to/from strings
 * according to the display information in the EDS.
 *
 * Linked as part of the "full" EDS runtime library
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>

#include <stdint.h>
#include "edslib_internal.h"


const EdsLib_SymbolTableEntry_t *EdsLib_DisplaySymbolLookup_GetByName(const EdsLib_SymbolTableEntry_t *SymbolDict, uint16_t TableSize, const char *String, uint32_t StringLen)
{
    const EdsLib_SymbolTableEntry_t *Sym;
    int Compare;
    uint32_t LowIndex;
    uint32_t HighIndex;
    uint32_t SearchIndex;

    LowIndex = 0;
    HighIndex = TableSize;
    while (true)
    {
        if (LowIndex >= HighIndex)
        {
            Sym = NULL;
            break;
        }

        SearchIndex = (LowIndex + HighIndex) / 2;
        Sym = &SymbolDict[SearchIndex];

        Compare = strncmp(Sym->SymName, String, StringLen);
        if (Compare == 0 && Sym->SymName[StringLen] == 0)
        {
            break;
        }

        if (Compare < 0)
        {
            LowIndex = SearchIndex + 1;
        }
        else
        {
            HighIndex = SearchIndex;
        }
    }

    return Sym;
}

const EdsLib_SymbolTableEntry_t *EdsLib_DisplaySymbolLookup_GetByValue(const EdsLib_SymbolTableEntry_t *SymbolDict, uint16_t TableSize, intmax_t Value)
{
    const EdsLib_SymbolTableEntry_t *Result;

    Result = SymbolDict;

    /*
     * The list is ordered by name, not by value, so we need to
     * do a less efficient sequential search to find the value.
     */
    while (1)
    {
        if (TableSize == 0)
        {
            Result = NULL;
            break;
        }
        if (Result->SymValue == Value)
        {
            break;
        }
        --TableSize;
        ++Result;
    }

    return Result;
}


int32_t EdsLib_DisplayScalarConv_ToString_Impl(const EdsLib_DataTypeDB_Entry_t *DictEntryPtr, const EdsLib_DisplayDB_Entry_t *DisplayInfoPtr,
        char *OutputBuffer, uint32_t BufferSize, const void *SourcePtr)
{
    EdsLib_GenericValueBuffer_t NumberBuffer;
    EdsLib_ConstPtr_t Src;
    int32_t Status;

    Src.Ptr = SourcePtr;
    NumberBuffer.ValueType = EDSLIB_BASICTYPE_NONE;

    /*
     * Do not waste effort trying to convert something that is
     * not directly representable by a string
     */
    if (DictEntryPtr == NULL)
    {
        Status = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else if (DictEntryPtr->BasicType == EDSLIB_BASICTYPE_CONTAINER)
    {
        snprintf(OutputBuffer,BufferSize,"<ContainerDataType>");
        Status = EDSLIB_SUCCESS;
    }
    else if (DictEntryPtr->BasicType == EDSLIB_BASICTYPE_ARRAY)
    {
        snprintf(OutputBuffer,BufferSize,"<ArrayDataType>");
        Status = EDSLIB_SUCCESS;
    }
    else
    {
        Status = EDSLIB_NOT_IMPLEMENTED;
        EdsLib_DataTypeLoad_Impl(&NumberBuffer, Src, DictEntryPtr);

        /*
         * First preference:
         * Get the display hint from the DB, and use it if available
         */
        if (DisplayInfoPtr != NULL)
        {
            switch (DisplayInfoPtr->DisplayHint)
            {
            case EDSLIB_DISPLAYHINT_STRING:
            {
                uint16_t SrcSize = DictEntryPtr->SizeInfo.Bytes;

                /*
                 * binary blob contains character string data.
                 * copy only PRINTABLE chars to output buffer,
                 * as this is intended for UI display
                 */
                while (SrcSize > 0 && BufferSize > 1 && Src.u->u8 != 0)
                {
                    if (isprint((int)Src.u->u8))
                    {
                        *OutputBuffer = Src.u->u8;
                    }
                    else
                    {
                        *OutputBuffer = '.';
                    }

                    ++OutputBuffer;
                    --BufferSize;
                    ++Src.Addr;
                    --SrcSize;
                }
                if (BufferSize > 0)
                {
                    *OutputBuffer = 0;
                }
                Status = EDSLIB_SUCCESS;
                break;
            }
            case EDSLIB_DISPLAYHINT_BASE64:
            {
                /*
                 * binary blob contains binary data, which cannot be
                 * put into a normal string.  Use Base64 encoding.
                 */
                EdsLib_DisplayDB_Base64Encode(OutputBuffer, BufferSize, Src.Addr, DictEntryPtr->SizeInfo.Bits);
                Status = EDSLIB_SUCCESS;
                break;
            }
            case EDSLIB_DISPLAYHINT_ENUM_SYMTABLE:
            {
                const EdsLib_SymbolTableEntry_t *Symbol;

                if (NumberBuffer.ValueType == EDSLIB_BASICTYPE_SIGNED_INT)
                {
                    Symbol = EdsLib_DisplaySymbolLookup_GetByValue(DisplayInfoPtr->DisplayArg.SymTable,
                            DisplayInfoPtr->DisplayArgTableSize, NumberBuffer.Value.SignedInteger);
                }
                else if (NumberBuffer.ValueType == EDSLIB_BASICTYPE_UNSIGNED_INT)
                {
                    Symbol = EdsLib_DisplaySymbolLookup_GetByValue(DisplayInfoPtr->DisplayArg.SymTable,
                            DisplayInfoPtr->DisplayArgTableSize, NumberBuffer.Value.UnsignedInteger);
                }
                else
                {
                    Symbol = NULL;
                }

                if (Symbol != NULL)
                {
                    snprintf(OutputBuffer, BufferSize, "%s", Symbol->SymName);
                    Status = EDSLIB_SUCCESS;
                }
                break;
            }
            case EDSLIB_DISPLAYHINT_ADDRESS:
            {
                /* memory addresses are always hex values */
                if (NumberBuffer.ValueType == EDSLIB_BASICTYPE_SIGNED_INT ||
                        NumberBuffer.ValueType == EDSLIB_BASICTYPE_UNSIGNED_INT)
                {
                    snprintf(OutputBuffer,BufferSize,"0x%0*llx", (int)(DictEntryPtr->SizeInfo.Bytes * 2),
                            NumberBuffer.Value.UnsignedInteger);
                    Status = EDSLIB_SUCCESS;
                }
                break;
            }
            case EDSLIB_DISPLAYHINT_BOOLEAN:
            {
                /* memory addresses are always hex values */
                if ((NumberBuffer.ValueType == EDSLIB_BASICTYPE_UNSIGNED_INT && NumberBuffer.Value.UnsignedInteger != 0) ||
                        (NumberBuffer.ValueType == EDSLIB_BASICTYPE_SIGNED_INT && NumberBuffer.Value.SignedInteger != 0))
                {
                    snprintf(OutputBuffer, BufferSize, "true");
                }
                else
                {
                    snprintf(OutputBuffer, BufferSize, "false");
                }
                Status = EDSLIB_SUCCESS;
                break;
            }
            default:
                break;
            }
        }
    }

    /*
     * If not converted, then use a fallback/default conversion
     */
    if (Status != EDSLIB_SUCCESS)
    {
        switch(NumberBuffer.ValueType)
        {
        case EDSLIB_BASICTYPE_UNSIGNED_INT:
            snprintf(OutputBuffer,BufferSize,"%llu",NumberBuffer.Value.UnsignedInteger);
            Status = EDSLIB_SUCCESS;
            break;
        case EDSLIB_BASICTYPE_SIGNED_INT:
            snprintf(OutputBuffer,BufferSize,"%lld",NumberBuffer.Value.SignedInteger);
            Status = EDSLIB_SUCCESS;
            break;
        case EDSLIB_BASICTYPE_FLOAT:
            snprintf(OutputBuffer,BufferSize,"%.4Lg",NumberBuffer.Value.FloatingPoint);
            Status = EDSLIB_SUCCESS;
            break;
        default:
            /*
             * catch-all case, to ensure something is written to the buffer.
             * If this is seen in the output it likely means a DB is missing.
             */
            snprintf(OutputBuffer,BufferSize,"<\?\?\?>");
            break;
        }
    }

    return Status;
}

int32_t EdsLib_DisplayScalarConv_FromString_Impl(const EdsLib_DataTypeDB_Entry_t *DictEntryPtr, const EdsLib_DisplayDB_Entry_t *DisplayInfoPtr,
        void *DestPtr, const char *SrcString)
{
    EdsLib_GenericValueBuffer_t NumberBuffer;
    int32_t Status;
    EdsLib_Ptr_t Dst;

    Dst.Ptr = DestPtr;
    NumberBuffer.ValueType = EDSLIB_BASICTYPE_NONE;

    if (DictEntryPtr == NULL)
    {
        Status = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else if (DictEntryPtr->BasicType == EDSLIB_BASICTYPE_CONTAINER ||
            DictEntryPtr->BasicType == EDSLIB_BASICTYPE_ARRAY)
    {
        Status = EDSLIB_NOT_IMPLEMENTED;
    }
    else
    {
        Status = EDSLIB_FAILURE;

        /*
         * Get the display hint from the DB, and use it if available
         */
        if (DisplayInfoPtr != NULL)
        {
            switch (DisplayInfoPtr->DisplayHint)
            {
            case EDSLIB_DISPLAYHINT_STRING:
            {
                uint16_t DstSize = DictEntryPtr->SizeInfo.Bytes;
                while (DstSize > 0)
                {
                    Dst.u->u8 = *SrcString;
                    if (*SrcString != 0)
                    {
                        ++SrcString;
                    }
                    ++Dst.Addr;
                    --DstSize;
                }
                Status = EDSLIB_SUCCESS;
                break;
            }
            case EDSLIB_DISPLAYHINT_BASE64:
            {
                /*
                 * binary blob contains binary data, which cannot be
                 * put into a normal string.  Use Base64 encoding.
                 */
                EdsLib_DisplayDB_Base64Decode(Dst.Addr, DictEntryPtr->SizeInfo.Bits, SrcString);
                Status = EDSLIB_SUCCESS;
                break;
            }
            case EDSLIB_DISPLAYHINT_ENUM_SYMTABLE:
            {
                const EdsLib_SymbolTableEntry_t *Symbol;

                Symbol = EdsLib_DisplaySymbolLookup_GetByName(DisplayInfoPtr->DisplayArg.SymTable,
                        DisplayInfoPtr->DisplayArgTableSize, SrcString, strlen(SrcString));

                if (Symbol != NULL)
                {
                    NumberBuffer.Value.SignedInteger = Symbol->SymValue;
                    NumberBuffer.ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
                }
                break;
            }
            case EDSLIB_DISPLAYHINT_ADDRESS:
            {
                /* memory addresses are always hex values */
                const char *EndPtr;
                NumberBuffer.Value.UnsignedInteger = strtoull(SrcString, (char**)&EndPtr, 16);
                if (EndPtr != SrcString && *EndPtr == 0)
                {
                    NumberBuffer.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
                }
                break;
            }
            case EDSLIB_DISPLAYHINT_BOOLEAN:
            {
                /* typically TRUE/FALSE, also accept YES/NO or 0/1 */
                const char *EndPtr = SrcString;
                if (strcasecmp(SrcString, "true") == 0)
                {
                    EndPtr += 4;
                    NumberBuffer.Value.UnsignedInteger = 1;
                }
                else if (strcasecmp(SrcString, "false") == 0)
                {
                    EndPtr += 4;
                    NumberBuffer.Value.UnsignedInteger = 0;
                }
                else if (strcasecmp(SrcString, "yes") == 0)
                {
                    EndPtr += 3;
                    NumberBuffer.Value.UnsignedInteger = 1;
                }
                else if (strcasecmp(SrcString, "no") == 0)
                {
                    EndPtr += 2;
                    NumberBuffer.Value.UnsignedInteger = 0;
                }
                else
                {
                    NumberBuffer.Value.UnsignedInteger = strtol(SrcString, (char**)&EndPtr, 0);
                }
                if (EndPtr != SrcString && *EndPtr == 0)
                {
                    NumberBuffer.ValueType = EDSLIB_BASICTYPE_UNSIGNED_INT;
                }
                break;
            }
            default:
                break;
            }
        }

        if (Status != EDSLIB_SUCCESS &&
                NumberBuffer.ValueType == EDSLIB_BASICTYPE_NONE)
        {
            const char *EndPtr;
            switch(DictEntryPtr->BasicType)
            {
            case EDSLIB_BASICTYPE_UNSIGNED_INT:
            case EDSLIB_BASICTYPE_SIGNED_INT:
            case EDSLIB_BASICTYPE_FLOAT:
            {
                /*
                 * NOTE: conversion is done into the system long double type --
                 * This is because we do not have control over how the input data is
                 * formatted.  For instance, if the target type is a signed integer and
                 * supplied string is "-5.0" or "1e3", these should still work.
                 * Using a long double here should provide for a 64-bit mantissa, so
                 * even 64 bit integer values can be stored without precision loss
                 */
                NumberBuffer.Value.FloatingPoint = strtold(SrcString, (char**)&EndPtr);
                if (EndPtr != SrcString && *EndPtr == 0)
                {
                    NumberBuffer.ValueType = EDSLIB_BASICTYPE_FLOAT;
                }
                break;
            }
            case EDSLIB_BASICTYPE_BINARY:
            {
                uint16_t DstSize = DictEntryPtr->SizeInfo.Bytes;
                size_t SrcSize = strlen(SrcString);
                Status = EDSLIB_SUCCESS;

                while (DstSize > 0 && SrcSize >= 2)
                {
                    NumberBuffer.Value.StringData[0] = *SrcString;
                    ++SrcString;
                    NumberBuffer.Value.StringData[1] = *SrcString;
                    ++SrcString;
                    NumberBuffer.Value.StringData[2] = 0;
                    SrcSize -= 2;

                    Dst.u->u8 = strtoul(NumberBuffer.Value.StringData, (char **)&EndPtr, 16);
                    if (*EndPtr != 0)
                    {
                        Status = EDSLIB_FAILURE;
                        break;
                    }
                    ++Dst.Addr;
                    --DstSize;
                }
                break;
            }
            default:
            {
                break;
            }
            }
        }

        if (Status != EDSLIB_SUCCESS)
        {
            EdsLib_DataTypeStore_Impl(Dst, &NumberBuffer, DictEntryPtr);
            if (NumberBuffer.ValueType == DictEntryPtr->BasicType)
            {
                Status = EDSLIB_SUCCESS;
            }
        }
    }

    return Status;
}


