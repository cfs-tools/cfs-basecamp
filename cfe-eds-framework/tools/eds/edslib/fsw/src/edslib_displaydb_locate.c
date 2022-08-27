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
 * \file     edslib_displaydb_locate.c
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

static EdsLib_Iterator_Rc_t EdsLib_DisplayLocateMember_GetContainerPosition_Callback(
        const EdsLib_DatabaseObject_t *GD,
        EdsLib_Iterator_CbType_t CbType,
        const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
        const char *EntityName,
        void *OpaqueArg)
{
    EdsLib_DisplayLocateMember_ControlBlock_t *CtrlBlock = OpaqueArg;

    if (CbType == EDSLIB_ITERATOR_CBTYPE_MEMBER &&
            EntityName != NULL &&
            CtrlBlock->MatchQuality != EDSLIB_MATCHQUALITY_EXACT &&
            strncmp(EntityName, CtrlBlock->ContentPos, CtrlBlock->ContentLength) == 0 &&
            EntityName[CtrlBlock->ContentLength] == 0)
    {
        CtrlBlock->MatchQuality = EDSLIB_MATCHQUALITY_EXACT;
        CtrlBlock->RefObj = EntityInfo->Details.RefObj;
        CtrlBlock->MaxSize.Bytes = EntityInfo->EndOffset.Bytes - EntityInfo->StartOffset.Bytes;
        CtrlBlock->MaxSize.Bits = EntityInfo->EndOffset.Bits - EntityInfo->StartOffset.Bits;
        CtrlBlock->StartOffset.Bytes += EntityInfo->StartOffset.Bytes;
        CtrlBlock->StartOffset.Bits += EntityInfo->StartOffset.Bits;
    }

    if (CtrlBlock->MatchQuality != EDSLIB_MATCHQUALITY_EXACT)
    {
        return EDSLIB_ITERATOR_RC_CONTINUE;
    }

    return EDSLIB_ITERATOR_RC_STOP;
}

static void EdsLib_DisplayLocateMember_GetContainerPosition(const EdsLib_DatabaseObject_t *GD, EdsLib_DisplayLocateMember_ControlBlock_t *CtrlBlock)
{
    EDSLIB_DECLARE_DISPLAY_ITERATOR_CB(IteratorState,
            EDSLIB_ITERATOR_MAX_BASETYPE_DEPTH,
            EdsLib_DisplayLocateMember_GetContainerPosition_Callback,
            CtrlBlock);

    EDSLIB_RESET_DISPLAY_ITERATOR_FROM_REFOBJ(IteratorState, CtrlBlock->RefObj);

    EdsLib_DataTypeIterator_Impl(GD, &IteratorState.BaseIter.Cb);
}

static void EdsLib_DisplayLocateMember_GetArrayPosition(const EdsLib_DatabaseObject_t *GD, EdsLib_DisplayLocateMember_ControlBlock_t *CtrlBlock)
{
    const EdsLib_DisplayDB_Entry_t *DisplayInf;
    const EdsLib_SymbolTableEntry_t *Sym = NULL;
    const char *ConvEnd;
    int32_t LocalIdx;

    DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &CtrlBlock->RefObj);
    while (DisplayInf != NULL && DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_REFERENCE_TYPE)
    {
        DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &DisplayInf->DisplayArg.RefObj);
    }

    if (DisplayInf != NULL &&
            DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
    {
        Sym = EdsLib_DisplaySymbolLookup_GetByName(DisplayInf->DisplayArg.SymTable, DisplayInf->DisplayArgTableSize,
                CtrlBlock->ContentPos, CtrlBlock->ContentLength);
    }
    if (Sym != NULL)
    {
        LocalIdx = Sym->SymValue;
        CtrlBlock->MatchQuality = EDSLIB_MATCHQUALITY_EXACT;
    }
    else
    {
        /* Try a converting to a number and see what happens */
        LocalIdx = strtol(CtrlBlock->ContentPos, (char **)&ConvEnd, 0);
        if (ConvEnd != CtrlBlock->ContentPos)
        {
            CtrlBlock->MatchQuality = EDSLIB_MATCHQUALITY_EXACT;
        }
    }
    if (CtrlBlock->MatchQuality != EDSLIB_MATCHQUALITY_NONE)
    {
        CtrlBlock->RefObj = CtrlBlock->DataDict->Detail.Array->ElementRefObj;
        CtrlBlock->MaxSize.Bytes = (CtrlBlock->DataDict->SizeInfo.Bytes / CtrlBlock->DataDict->NumSubElements);
        CtrlBlock->MaxSize.Bits = (CtrlBlock->DataDict->SizeInfo.Bits / CtrlBlock->DataDict->NumSubElements);
        CtrlBlock->StartOffset.Bytes += LocalIdx * CtrlBlock->MaxSize.Bytes;
        CtrlBlock->StartOffset.Bits += LocalIdx * CtrlBlock->MaxSize.Bits;
    }
}

void EdsLib_DisplayLocateMember_Impl(const EdsLib_DatabaseObject_t *GD, EdsLib_DisplayLocateMember_ControlBlock_t *CtrlBlock)
{
    const char *EndPos;

    while (isspace((int)*CtrlBlock->ContentPos))
    {
        ++CtrlBlock->ContentPos;
    }

    CtrlBlock->DataDict = EdsLib_DataTypeDB_GetEntry(GD, &CtrlBlock->RefObj);
    if (*CtrlBlock->ContentPos != 0 && CtrlBlock->DataDict != NULL &&
            CtrlBlock->DataDict->NumSubElements > 0)
    {
        switch (CtrlBlock->DataDict->BasicType)
        {
        case EDSLIB_BASICTYPE_ARRAY:
            if (*CtrlBlock->ContentPos == '[')
            {
                do
                {
                    ++CtrlBlock->ContentPos;
                }
                while (isspace((int)*CtrlBlock->ContentPos));

                EndPos = CtrlBlock->ContentPos;
                while (isalnum((int)*EndPos) || *EndPos == '_')
                {
                    ++CtrlBlock->ContentLength;
                    ++EndPos;
                }

                while (isspace((int)*EndPos))
                {
                    ++EndPos;
                }

                if (*EndPos == ']' && CtrlBlock->ContentLength > 0)
                {
                    CtrlBlock->NextTokenPos = EndPos + 1;
                    EdsLib_DisplayLocateMember_GetArrayPosition(GD, CtrlBlock);
                }
            }
            break;
        case EDSLIB_BASICTYPE_CONTAINER:
            if (*CtrlBlock->ContentPos == '.')
            {
                do
                {
                    ++CtrlBlock->ContentPos;
                }
                while (isspace((int)*CtrlBlock->ContentPos));
            }

            EndPos = CtrlBlock->ContentPos;
            while (isalnum((int)*EndPos) || *EndPos == '_')
            {
                ++CtrlBlock->ContentLength;
                ++EndPos;
            }

            if (CtrlBlock->ContentLength > 0)
            {
                CtrlBlock->NextTokenPos = EndPos;
                EdsLib_DisplayLocateMember_GetContainerPosition(GD, CtrlBlock);
            }
            break;
        default:
            break;
        }
    }
}


