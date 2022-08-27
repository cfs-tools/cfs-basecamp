/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 * LEW-20211-1, Python Bindings for the Core Flight Executive Mission Library
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
 * \file     edslib_displaydb_api.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of public EDS API functions to locate members within EDS structure objects.
 *
 * Linked as part of the "full" EDS runtime library
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "edslib_displaydb.h"
#include "edslib_internal.h"

/**
 * In order to allow simpler and safer usage within printf()-like statements,
 * functions that return const char * strings will return this value rather
 * than NULL when the result is undefined.
 */
static const char UNDEF_STRING[] = "UNDEFINED";

/*
 * Initialization routine placeholder.
 *
 * Currently a no-op, as there are no global data
 * structures needing initialization at this time.
 *
 * For now, this is a placeholder in case that changes,
 * but this also serves an important role for static linking
 * to ensure that this code is linked into the target.
 */
void EdsLib_DisplayDB_Initialize(void)
{
}


const char *EdsLib_DisplayDB_GetEdsName(const EdsLib_DatabaseObject_t *GD, uint16_t AppId)
{
    EdsLib_DisplayDB_t DisplayDBPtr;
    const char *Result;

    DisplayDBPtr = EdsLib_DisplayDB_GetTopLevel(GD, AppId);
    if (DisplayDBPtr != NULL)
    {
        Result = DisplayDBPtr->EdsName;
    }
    else
    {
        Result = NULL;
    }

    /* Make sure we never return NULL, allows safe use in printf() statements */
    if (Result == NULL)
    {
        Result = UNDEF_STRING;
    }

    return Result;
}

const char *EdsLib_DisplayDB_GetBaseName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId)
{
    const EdsLib_DisplayDB_Entry_t *DispInfo;
    EdsLib_DatabaseRef_t TempRef;
    const char *Result;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DispInfo = EdsLib_DisplayDB_GetEntry(GD, &TempRef);

    if (DispInfo != NULL)
    {
        Result = DispInfo->Name;
    }
    else
    {
        Result = NULL;
    }

    /* Make sure we never return NULL, allows safe use in printf() statements */
    if (Result == NULL)
    {
        Result = UNDEF_STRING;
    }

    return Result;
}

const char *EdsLib_DisplayDB_GetTypeName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *Buffer, uint32_t BufferSize)
{
    const EdsLib_DisplayDB_Entry_t *DispInfo;
    EdsLib_DatabaseRef_t TempRef;
    const char *Result;

    /* Should not return NULL, which allows safe use in printf() statements */
    if (Buffer == NULL || BufferSize == 0)
    {
        return UNDEF_STRING;
    }

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DispInfo = EdsLib_DisplayDB_GetEntry(GD, &TempRef);
    Buffer[0] = 0;

    if (DispInfo != NULL)
    {
        if (DispInfo->Namespace != NULL && DispInfo->Name != NULL)
        {
            snprintf(Buffer,BufferSize,"%s/%s",DispInfo->Namespace, DispInfo->Name);
        }
        else if (DispInfo->Name != NULL)
        {
            snprintf(Buffer,BufferSize,"%s", DispInfo->Name);
        }
    }

    if (Buffer[0] != 0)
    {
        Result = Buffer;
    }
    else
    {
        Result = UNDEF_STRING;
    }

    return Result;
}

const char *EdsLib_DisplayDB_GetNamespace(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId)
{
    const EdsLib_DisplayDB_Entry_t *DispInfo;
    EdsLib_DatabaseRef_t TempRef;
    const char *Result;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DispInfo = EdsLib_DisplayDB_GetEntry(GD, &TempRef);

    if (DispInfo == NULL)
    {
        Result = NULL;
    }
    else
    {
        Result = DispInfo->Namespace;
    }

    /*
     * Allow direct use in printf()-style calls by never returning NULL
     */
    if (Result == NULL)
    {
        Result = UNDEF_STRING;
    }

    return Result;
}
EdsLib_DisplayHint_t EdsLib_DisplayDB_GetDisplayHint(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId)
{
    const EdsLib_DisplayDB_Entry_t *DisplayInf;
    EdsLib_DatabaseRef_t TempRef;
    EdsLib_DisplayHint_t HintType;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &TempRef);
    while (DisplayInf != NULL && DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_REFERENCE_TYPE)
    {
        DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &DisplayInf->DisplayArg.RefObj);
    }
    if (DisplayInf != NULL)
    {
        HintType = DisplayInf->DisplayHint;
    }
    else
    {
        HintType = EDSLIB_DISPLAYHINT_NONE;
    }

    return HintType;
}

int32_t EdsLib_DisplayDB_GetIndexByName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name, uint16_t *SubIndex)
{
    const EdsLib_DisplayDB_Entry_t *DisplayInf;
    const EdsLib_DataTypeDB_Entry_t *DataTypeInf;
    EdsLib_DatabaseRef_t TempRef;
    int32_t Status;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataTypeInf = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    /*
     * Do not waste effort trying to convert something that is
     * not directly representable by a string
     */
    if (DataTypeInf == NULL || DataTypeInf->NumSubElements == 0)
    {
        Status = EDSLIB_INVALID_SIZE_OR_TYPE;
    }
    else
    {
        DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &TempRef);
        while (DisplayInf != NULL && DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_REFERENCE_TYPE)
        {
            DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &DisplayInf->DisplayArg.RefObj);
        }

        if (DisplayInf == NULL)
        {
            Status = EDSLIB_INCOMPLETE_DB_OBJECT;
        }
        else
        {
            Status = EDSLIB_NAME_NOT_FOUND;
            switch(DisplayInf->DisplayHint)
            {
            case EDSLIB_DISPLAYHINT_ENUM_SYMTABLE:
            {
                const EdsLib_SymbolTableEntry_t *EnumSym =
                        EdsLib_DisplaySymbolLookup_GetByName(DisplayInf->DisplayArg.SymTable,
                                DisplayInf->DisplayArgTableSize, Name, strlen(Name));
                if (EnumSym != NULL)
                {
                    *SubIndex = EnumSym->SymValue;
                    Status = EDSLIB_SUCCESS;
                }
                break;
            }
            case EDSLIB_DISPLAYHINT_MEMBER_NAMETABLE:
            {
                const char * const *NameTable = DisplayInf->DisplayArg.NameTable;
                uint16_t Idx;
                for (Idx = 0; Idx < DisplayInf->DisplayArgTableSize; ++Idx)
                {
                    if (*NameTable != NULL)
                    {
                        if(strcmp(*NameTable, Name) == 0)
                        {
                            *SubIndex = Idx;
                            Status = EDSLIB_SUCCESS;
                            break;
                        }
                    }
                    ++NameTable;
                }
                break;
            }
            default:
                break;
            }
        }
    }

    return Status;
}

const char *EdsLib_DisplayDB_GetNameByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t SubIndex)
{
    const EdsLib_DisplayDB_Entry_t *DisplayInf;
    const EdsLib_DataTypeDB_Entry_t *DataTypeInf;
    EdsLib_DatabaseRef_t TempRef;
    const char *result;

    result = NULL;
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DataTypeInf = EdsLib_DataTypeDB_GetEntry(GD, &TempRef);

    if (DataTypeInf != NULL && SubIndex < DataTypeInf->NumSubElements)
    {
        DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &TempRef);
        while (DisplayInf != NULL && DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_REFERENCE_TYPE)
        {
            DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &DisplayInf->DisplayArg.RefObj);
        }

        if (DisplayInf != NULL)
        {
            switch(DisplayInf->DisplayHint)
            {
            case EDSLIB_DISPLAYHINT_ENUM_SYMTABLE:
            {
                const EdsLib_SymbolTableEntry_t *EnumSym =
                        EdsLib_DisplaySymbolLookup_GetByValue(DisplayInf->DisplayArg.SymTable,
                                DisplayInf->DisplayArgTableSize, SubIndex);
                if (EnumSym != NULL)
                {
                    result = EnumSym->SymName;
                }
                break;
            }
            case EDSLIB_DISPLAYHINT_MEMBER_NAMETABLE:
            {
                /* direct lookup */
                if (SubIndex < DisplayInf->DisplayArgTableSize)
                {
                    result = DisplayInf->DisplayArg.NameTable[SubIndex];
                }
                break;
            }
            default:
                break;
            }
        }
    }

    return result;
}

void EdsLib_DisplayDB_IterateAllEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_EntityCallback_t Callback, void *Arg)
{
    EdsLib_DisplayUserIterator_FullName_StackEntry_t NameStack[EDSLIB_ITERATOR_MAX_DEEP_DEPTH];
    EdsLib_DisplayUserIterator_FullName_ControlBlock_t CtrlBlock;

    EDSLIB_DECLARE_DISPLAY_ITERATOR_CB(IteratorState,
            EDSLIB_ITERATOR_MAX_DEEP_DEPTH,
            EdsLib_DisplayUserIterator_FullName_Callback,
            &CtrlBlock);

    EDSLIB_RESET_DISPLAY_ITERATOR_FROM_EDSID(IteratorState, EdsId);

    memset(&CtrlBlock, 0, sizeof(CtrlBlock));
    CtrlBlock.NextEntry = NameStack;
    CtrlBlock.ScratchNameBuffer[0] = 0;
    CtrlBlock.Base.UserCallback = Callback;
    CtrlBlock.Base.UserArg = Arg;

    EdsLib_DataTypeIterator_Impl(GD, &IteratorState.BaseIter.Cb);
}

void EdsLib_DisplayDB_IterateBaseEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_EntityCallback_t Callback, void *Arg)
{
    EdsLib_DisplayUserIterator_BaseName_ControlBlock_t CtrlBlock;

    EDSLIB_DECLARE_DISPLAY_ITERATOR_CB(IteratorState,
            EDSLIB_ITERATOR_MAX_BASETYPE_DEPTH,
            EdsLib_DisplayUserIterator_BaseName_Callback,
            &CtrlBlock);

    EDSLIB_RESET_DISPLAY_ITERATOR_FROM_EDSID(IteratorState, EdsId);

    memset(&CtrlBlock, 0, sizeof(CtrlBlock));
    CtrlBlock.UserCallback = Callback;
    CtrlBlock.UserArg = Arg;
    EdsLib_DataTypeIterator_Impl(GD, &IteratorState.BaseIter.Cb);
}

int32_t EdsLib_DisplayDB_LocateSubEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name, EdsLib_DataTypeDB_EntityInfo_t *CompInfo)
{
    EdsLib_DisplayLocateMember_ControlBlock_t CtrlBlock;
    const char *CurrTokenPos;
    int32_t Status;

    memset(CompInfo, 0, sizeof(*CompInfo));
    memset(&CtrlBlock, 0, sizeof(CtrlBlock));
    EdsLib_Decode_StructId(&CtrlBlock.RefObj, EdsId);
    CurrTokenPos = Name;
    Status = EDSLIB_FAILURE;

    while (1)
    {
        CtrlBlock.ContentPos = CurrTokenPos;
        CtrlBlock.NextTokenPos = NULL;
        CtrlBlock.MatchQuality = EDSLIB_MATCHQUALITY_NONE;
        CtrlBlock.ContentLength = 0;

        EdsLib_DisplayLocateMember_Impl(GD, &CtrlBlock);
        if (CtrlBlock.MatchQuality != EDSLIB_MATCHQUALITY_EXACT)
        {
            Status = EDSLIB_NAME_NOT_FOUND;
            break;
        }

        CurrTokenPos = CtrlBlock.NextTokenPos;
        while (isspace((int)*CurrTokenPos))
        {
            ++CurrTokenPos;
        }

        if (*CurrTokenPos == 0)
        {
            Status = EDSLIB_SUCCESS;
            CompInfo->Offset = CtrlBlock.StartOffset;
            CompInfo->MaxSize = CtrlBlock.MaxSize;
            CompInfo->EdsId = EdsLib_Encode_StructId(&CtrlBlock.RefObj);
            break;
        }
    }

    return Status;
}

void EdsLib_Generate_Hexdump(void *output, const uint8_t *DataPtr, uint16_t DisplayOffset, uint16_t Count)
{
   uint16_t i;
   char display[20];
   FILE *fp = output;

   fprintf(fp, "Data Segment Length=%u:",Count);
   memset(display,0,sizeof(display));
   for (i = 0; i < Count; ++i)
   {
      if ((i & 0x0F) == 0)
      {
         fprintf(fp, "  %s\n  %03x:",display,DisplayOffset);
         memset(display,0,sizeof(display));
      }
      fprintf(fp, " %02x",*DataPtr);
      if (isprint(*DataPtr))
      {
         display[i & 0x0F] = *DataPtr;
      }
      else
      {
         display[i & 0x0F] = '.';
      }
      ++DataPtr;
      ++DisplayOffset;
   }
   if (i > 0)
   {
      while (i & 0xF)
      {
         fprintf(fp, "   ");
         ++i;
      }
   }
   fprintf(fp, "  %s\n", display);
}

int32_t EdsLib_Scalar_ToString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *OutputBuffer, uint32_t BufferSize, const void *SrcPtr)
{
    EdsLib_DatabaseRef_t TempRef;

    EdsLib_Decode_StructId(&TempRef, EdsId);

    return EdsLib_DisplayScalarConv_ToString_Impl(EdsLib_DataTypeDB_GetEntry(GD, &TempRef), EdsLib_DisplayDB_GetEntry(GD, &TempRef),
            OutputBuffer, BufferSize, SrcPtr);
}

int32_t EdsLib_Scalar_FromString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr, const char *SrcString)
{
    EdsLib_DatabaseRef_t TempRef;

    EdsLib_Decode_StructId(&TempRef, EdsId);

    return EdsLib_DisplayScalarConv_FromString_Impl(EdsLib_DataTypeDB_GetEntry(GD, &TempRef), EdsLib_DisplayDB_GetEntry(GD, &TempRef),
            DestPtr, SrcString);
}

const char *EdsLib_DisplayDB_GetEnumLabel(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const EdsLib_GenericValueBuffer_t *ValueBuffer)
{
    EdsLib_DatabaseRef_t TempRef;
    const EdsLib_DisplayDB_Entry_t *DisplayInfoPtr;
    const EdsLib_SymbolTableEntry_t *TableEnt;

    TableEnt = NULL;
    EdsLib_Decode_StructId(&TempRef, EdsId);
    DisplayInfoPtr = EdsLib_DisplayDB_GetEntry(GD, &TempRef);

    if (ValueBuffer != NULL && DisplayInfoPtr != NULL &&
            DisplayInfoPtr->DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
    {
        if (ValueBuffer->ValueType == EDSLIB_BASICTYPE_SIGNED_INT)
        {
            TableEnt = EdsLib_DisplaySymbolLookup_GetByValue(DisplayInfoPtr->DisplayArg.SymTable,
                    DisplayInfoPtr->DisplayArgTableSize, ValueBuffer->Value.SignedInteger);
        }
        else if (ValueBuffer->ValueType == EDSLIB_BASICTYPE_UNSIGNED_INT)
        {
            TableEnt = EdsLib_DisplaySymbolLookup_GetByValue(DisplayInfoPtr->DisplayArg.SymTable,
                    DisplayInfoPtr->DisplayArgTableSize, ValueBuffer->Value.UnsignedInteger);
        }
    }

    if (TableEnt == NULL)
    {
        return NULL;
    }

    return TableEnt->SymName;
}

void EdsLib_DisplayDB_GetEnumValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *String, EdsLib_GenericValueBuffer_t *ValueBuffer)
{
    EdsLib_DatabaseRef_t TempRef;
    const EdsLib_DisplayDB_Entry_t *DisplayInfoPtr;
    const EdsLib_SymbolTableEntry_t *Symbol;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    Symbol = NULL;
    DisplayInfoPtr = EdsLib_DisplayDB_GetEntry(GD, &TempRef);

    if (DisplayInfoPtr != NULL && DisplayInfoPtr->DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
    {
        Symbol = EdsLib_DisplaySymbolLookup_GetByName(DisplayInfoPtr->DisplayArg.SymTable,
                    DisplayInfoPtr->DisplayArgTableSize, String, strlen(String));
    }
    else
    {
        Symbol = NULL;
    }


    if (ValueBuffer != NULL)
    {
        if (Symbol == NULL)
        {
            ValueBuffer->ValueType = EDSLIB_BASICTYPE_NONE;
            ValueBuffer->Value.SignedInteger = 0;
        }
        else
        {
            ValueBuffer->ValueType = EDSLIB_BASICTYPE_SIGNED_INT;
            ValueBuffer->Value.SignedInteger = Symbol->SymValue;
        }
    }
}

void EdsLib_DisplayDB_IterateEnumValues(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_SymbolCallback_t Callback, void *Arg)
{
    EdsLib_DatabaseRef_t TempRef;
    const EdsLib_DisplayDB_Entry_t *DisplayInfoPtr;
    const EdsLib_SymbolTableEntry_t *Symbol;
    uint16_t SymbolCount;

    EdsLib_Decode_StructId(&TempRef, EdsId);
    DisplayInfoPtr = EdsLib_DisplayDB_GetEntry(GD, &TempRef);

    if (DisplayInfoPtr != NULL && DisplayInfoPtr->DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
    {
        Symbol = DisplayInfoPtr->DisplayArg.SymTable;
        SymbolCount = DisplayInfoPtr->DisplayArgTableSize;
    }
    else
    {
        Symbol = NULL;
        SymbolCount = 0;
    }

    while(Symbol != NULL && SymbolCount > 0)
    {
        Callback(Arg, Symbol->SymName, Symbol->SymValue);
        ++Symbol;
        --SymbolCount;
    }
}

const char * EdsLib_DisplayDB_GetEnumLabelByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t Index, char *Buffer, uint32_t BufferSize)
{
	EdsLib_DatabaseRef_t TempRef;
	const EdsLib_DisplayDB_Entry_t *DisplayInfoPtr;
	const EdsLib_SymbolTableEntry_t *Symbol = NULL;
	uint16_t SymbolCount;
	const char * Result;

	EdsLib_Decode_StructId(&TempRef, EdsId);
	DisplayInfoPtr = EdsLib_DisplayDB_GetEntry(GD, &TempRef);

	if (DisplayInfoPtr != NULL && DisplayInfoPtr->DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
	{
		Symbol = DisplayInfoPtr->DisplayArg.SymTable;
		SymbolCount = DisplayInfoPtr->DisplayArgTableSize;
	}

	if (Symbol != NULL && Index < SymbolCount)
	{
		Symbol = Symbol + Index;
        snprintf(Buffer, BufferSize,"%s", Symbol->SymName);
        Result = Buffer;
	}
	else
	{
		Result = UNDEF_STRING;
	}

	return Result;
}

intmax_t EdsLib_DisplayDB_GetEnumValueByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t Index)
{
	EdsLib_DatabaseRef_t TempRef;
	const EdsLib_DisplayDB_Entry_t *DisplayInfoPtr;
	const EdsLib_SymbolTableEntry_t *Symbol = NULL;
	uint16_t SymbolCount;
	intmax_t Result;

	EdsLib_Decode_StructId(&TempRef, EdsId);
	DisplayInfoPtr = EdsLib_DisplayDB_GetEntry(GD, &TempRef);

	if (DisplayInfoPtr != NULL && DisplayInfoPtr->DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
	{
		Symbol = DisplayInfoPtr->DisplayArg.SymTable;
		SymbolCount = DisplayInfoPtr->DisplayArgTableSize;
	}

	if (Symbol != NULL && Index < SymbolCount)
	{
		Symbol = Symbol + Index;
		Result = Symbol->SymValue;
	}
	else
	{
		Result = (intmax_t) EDSLIB_FAILURE;
	}

	return Result;
}
