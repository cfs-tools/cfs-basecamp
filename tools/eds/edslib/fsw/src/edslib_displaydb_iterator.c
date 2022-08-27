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
 * \file     edslib_displaydb_iterator.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of internal EDS library functions to walk through an EDS
 * structure.  This actually uses the basic walk routine and combines that
 * data with supplemental data from the name dictionary and provides a
 * single callback to the user that has the full set of information.
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

EdsLib_Iterator_Rc_t EdsLib_DisplayIterator_Wrapper(const EdsLib_DatabaseObject_t *GD,
        EdsLib_Iterator_CbType_t CbType,
        const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
        void *OpaqueArg)
{
   EdsLib_DisplayDB_InternalIterator_ControlBlock_t *Iterator = (EdsLib_DisplayDB_InternalIterator_ControlBlock_t *)OpaqueArg;
   EdsLib_DisplayIterator_StackEntry_t *TopEnt;
   const char *EntityName;
   EdsLib_Iterator_Rc_t RetCode;

   RetCode = EDSLIB_ITERATOR_RC_DEFAULT;
   EntityName = NULL;

   switch(CbType)
   {
   case EDSLIB_ITERATOR_CBTYPE_START:
   {
       TopEnt = Iterator->NextStackEntry;
       ++Iterator->NextStackEntry;

       /*
        * note it is possible to get a NULL display info here, as the display
        * info is loaded separately and therefore can be missing.
        */
       TopEnt->DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &EntityInfo->Details.RefObj);
       while (TopEnt->DisplayInf != NULL &&
               TopEnt->DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_REFERENCE_TYPE)
       {
           TopEnt->DisplayInf = EdsLib_DisplayDB_GetEntry(GD, &TopEnt->DisplayInf->DisplayArg.RefObj);
       }
       break;
   }
   case EDSLIB_ITERATOR_CBTYPE_MEMBER:
   {
       TopEnt = Iterator->NextStackEntry - 1;
       if (TopEnt->DisplayInf)
       {
           if (TopEnt->DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_MEMBER_NAMETABLE)
           {
               EntityName = TopEnt->DisplayInf->DisplayArg.NameTable[EntityInfo->CurrIndex];
           }
           else if (TopEnt->DisplayInf->DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE)
           {
               const EdsLib_SymbolTableEntry_t *TableEnt =
                       EdsLib_DisplaySymbolLookup_GetByValue(TopEnt->DisplayInf->DisplayArg.SymTable,
                               TopEnt->DisplayInf->DisplayArgTableSize, EntityInfo->CurrIndex);
               if (TableEnt != NULL)
               {
                   EntityName = TableEnt->SymName;
               }
           }
       }
       break;
   }
   case EDSLIB_ITERATOR_CBTYPE_END:
   {
       --Iterator->NextStackEntry;
       break;
   }
   default:
       break;
   }

   if (EntityInfo->Details.EntryType == EDSLIB_ENTRYTYPE_BASE_TYPE)
   {
       /*
        * This is a "hidden base type" structure.
        * This means that the members of this structure will be iterated as if
        * they are members of the parent structure, just like an inherited type
        * in the typical OO paradigm.  In this case we descend _without_ giving
        * the typical user callback.
        */
       if (CbType == EDSLIB_ITERATOR_CBTYPE_MEMBER)
       {
           RetCode = EDSLIB_ITERATOR_RC_DESCEND;
       }
       else
       {
           RetCode = EDSLIB_ITERATOR_RC_CONTINUE;
       }
   }
   else
   {
       RetCode = Iterator->NextCallback(GD, CbType, EntityInfo, EntityName, Iterator->NextCallbackArg);
   }

   return RetCode;
}

EdsLib_Iterator_Rc_t EdsLib_DisplayUserIterator_BaseName_Callback(const EdsLib_DatabaseObject_t *GD,
        EdsLib_Iterator_CbType_t CbType,
        const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
        const char *EntityName,
        void *OpaqueArg)
{
    EdsLib_DisplayUserIterator_BaseName_ControlBlock_t *CtrlBlock = OpaqueArg;
    EdsLib_EntityDescriptor_t ParamBuff;

    if (CbType == EDSLIB_ITERATOR_CBTYPE_MEMBER)
    {
        memset(&ParamBuff, 0, sizeof(ParamBuff));
        ParamBuff.EntityInfo.Offset = EntityInfo->StartOffset;
        ParamBuff.EntityInfo.MaxSize.Bits = EntityInfo->EndOffset.Bits - EntityInfo->StartOffset.Bits;
        ParamBuff.EntityInfo.MaxSize.Bytes = EntityInfo->EndOffset.Bytes - EntityInfo->StartOffset.Bytes;
        ParamBuff.EntityInfo.EdsId = EdsLib_Encode_StructId(&EntityInfo->Details.RefObj);
        ParamBuff.FullName = EntityName;
        ParamBuff.SeqNum = EntityInfo->CurrIndex;

        CtrlBlock->UserCallback(CtrlBlock->UserArg, &ParamBuff);
    }

    return EDSLIB_ITERATOR_RC_CONTINUE;
}

EdsLib_Iterator_Rc_t EdsLib_DisplayUserIterator_FullName_Callback(const EdsLib_DatabaseObject_t *GD,
        EdsLib_Iterator_CbType_t CbType,
        const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
        const char *EntityName,
        void *OpaqueArg)
{
    EdsLib_DisplayUserIterator_FullName_ControlBlock_t *CtrlBlock = OpaqueArg;
    EdsLib_DisplayUserIterator_FullName_StackEntry_t *TopEnt;
    EdsLib_Iterator_Rc_t RetCode;

    RetCode = EDSLIB_ITERATOR_RC_DEFAULT;

    switch(CbType)
    {
    case EDSLIB_ITERATOR_CBTYPE_START:
    {
        TopEnt = CtrlBlock->NextEntry;
        ++CtrlBlock->NextEntry;
        TopEnt->ScratchOffset = strlen(CtrlBlock->ScratchNameBuffer);
        TopEnt->NameStyle = EntityInfo->DataDictPtr->BasicType;
        if (TopEnt->NameStyle == EDSLIB_BASICTYPE_CONTAINER && TopEnt->ScratchOffset > 0 &&
                TopEnt->ScratchOffset < (sizeof(CtrlBlock->ScratchNameBuffer) - 1))
        {
            CtrlBlock->ScratchNameBuffer[TopEnt->ScratchOffset] = '.';
            ++TopEnt->ScratchOffset;
            CtrlBlock->ScratchNameBuffer[TopEnt->ScratchOffset] = 0;
        }
        break;
    }
    case EDSLIB_ITERATOR_CBTYPE_MEMBER:
    {
        TopEnt = CtrlBlock->NextEntry - 1;
        if (EntityName != NULL)
        {
            /*
             * For arrays, the name will be suffixed with a C-style array index i.e. parent[index]
             * For structures, append a single '.' character as a C-style field delineator i.e "parent.member"
             */
            if (EntityInfo->Details.EntryType == EDSLIB_ENTRYTYPE_ARRAY_ELEMENT)
            {
                snprintf(&CtrlBlock->ScratchNameBuffer[TopEnt->ScratchOffset],
                        sizeof(CtrlBlock->ScratchNameBuffer) - TopEnt->ScratchOffset,
                        "[%s]", EntityName);
            }
            else
            {
                snprintf(&CtrlBlock->ScratchNameBuffer[TopEnt->ScratchOffset],
                        sizeof(CtrlBlock->ScratchNameBuffer) - TopEnt->ScratchOffset,
                        "%s", EntityName);
            }
        }
        else if (EntityInfo->Details.EntryType == EDSLIB_ENTRYTYPE_ARRAY_ELEMENT)
        {
            snprintf(&CtrlBlock->ScratchNameBuffer[TopEnt->ScratchOffset],
                    sizeof(CtrlBlock->ScratchNameBuffer) - TopEnt->ScratchOffset,
                    "[%u]", (unsigned int)EntityInfo->CurrIndex);
        }
        else
        {
            /* No name */
            CtrlBlock->ScratchNameBuffer[TopEnt->ScratchOffset] = 0;
        }

        if (EntityInfo->DataDictPtr->NumSubElements > 0)
        {
            RetCode = EDSLIB_ITERATOR_RC_DESCEND;
        }
        else if (CtrlBlock->ScratchNameBuffer[TopEnt->ScratchOffset] != 0)
        {
            RetCode = EdsLib_DisplayUserIterator_BaseName_Callback(GD, CbType,
                    EntityInfo, CtrlBlock->ScratchNameBuffer, &CtrlBlock->Base);
        }
        break;
    }
    case EDSLIB_ITERATOR_CBTYPE_END:
    {
        --CtrlBlock->NextEntry;
        break;
    }
    default:
        break;
    }

    return RetCode;
}


