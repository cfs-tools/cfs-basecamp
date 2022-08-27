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
 * \file     edslib_datatypedb_iterator.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of generic functions to walk through EDS-defined data structures,
 * calling a user-defined callback function for each data member.
 *
 * Linked as part of the "basic" EDS runtime library
 */

#include <stdlib.h>
#include <string.h>

#include "edslib_internal.h"

int32_t EdsLib_DataTypeIterator_Impl(const EdsLib_DatabaseObject_t *GD,
        EdsLib_DataTypeIterator_ControlBlock_t *StateInfo)
{
    EdsLib_DataTypeIterator_StackEntry_t *CurrLev;
    EdsLib_DataTypeIterator_StackEntry_t *ParentLev;
    EdsLib_Iterator_Rc_t NextAction;
    int32_t Status;
    uint16_t TopDepth;

    TopDepth = 1;
    ParentLev = NULL;
    CurrLev = StateInfo->StackBase;
    Status = EDSLIB_SUCCESS;

    /*
     * Set up the first entry in the state stack
     * The "RefObj" value and "StartOffset" values should already be set,
     * but other items may not be.
     */
    CurrLev->DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &CurrLev->Details.RefObj);
    if (CurrLev->DataDictPtr == NULL)
    {
        return EDSLIB_INCOMPLETE_DB_OBJECT;
    }

    CurrLev->EndOffset = CurrLev->DataDictPtr->SizeInfo;
    CurrLev->EndOffset.Bits += CurrLev->StartOffset.Bits;
    CurrLev->EndOffset.Bytes += CurrLev->StartOffset.Bytes;
    if (CurrLev->DataDictPtr->BasicType != EDSLIB_BASICTYPE_CONTAINER &&
            CurrLev->DataDictPtr->BasicType != EDSLIB_BASICTYPE_ARRAY)
    {
        /*
         * This handles cases where the entity was a scalar.
         * Just give a single callback for the entity itself.
         */
        StateInfo->Callback(GD, EDSLIB_ITERATOR_CBTYPE_MEMBER, CurrLev, StateInfo->CallbackArg);
        NextAction = EDSLIB_ITERATOR_RC_STOP;
    }
    else
    {
        NextAction = EDSLIB_ITERATOR_RC_DESCEND;
    }

    while(NextAction != EDSLIB_ITERATOR_RC_STOP)
    {
        /*
         * Perform the user callback based on the current requested action.
         * This determines the next action (ascend/descend/etc).
         */
        switch (NextAction)
        {
        case EDSLIB_ITERATOR_RC_DESCEND:
            if (CurrLev->DataDictPtr == NULL)
            {
                Status = EDSLIB_INCOMPLETE_DB_OBJECT;
                NextAction = EDSLIB_ITERATOR_RC_ASCEND;
            }
            else if (TopDepth < StateInfo->StackSize)
            {
                /* Give indication to the user application that an object is being started */
                NextAction = StateInfo->Callback(GD, EDSLIB_ITERATOR_CBTYPE_START, CurrLev, StateInfo->CallbackArg);
                ParentLev = CurrLev;
                ++CurrLev;
                ++TopDepth;
                memset(CurrLev, 0, sizeof(*CurrLev));
            }

            /* It is never valid to descend twice (must "continue" at least once to get the 1st element) */
            if (NextAction == EDSLIB_ITERATOR_RC_DESCEND)
            {
                NextAction = EDSLIB_ITERATOR_RC_CONTINUE;
            }
            break;
        case EDSLIB_ITERATOR_RC_ASCEND:
            --CurrLev;
            --TopDepth;

            if (TopDepth > 1)
            {
                --ParentLev;

                /* Give indication to the user application that an object is finished */
                NextAction = StateInfo->Callback(GD, EDSLIB_ITERATOR_CBTYPE_END, CurrLev, StateInfo->CallbackArg);
            }
            else
            {
                ParentLev = NULL;
                NextAction = EDSLIB_ITERATOR_RC_STOP;
            }
            break;
        case EDSLIB_ITERATOR_RC_CONTINUE:
            if (ParentLev == NULL)
            {
                NextAction = EDSLIB_ITERATOR_RC_STOP;
            }
            else if (CurrLev->CurrIndex >= ParentLev->DataDictPtr->NumSubElements)
            {
                /*
                 * Advance to the next element in the parent container, if we have not gotten to the end.
                 */
                NextAction = EDSLIB_ITERATOR_RC_ASCEND;
            }
            else if (ParentLev->DataDictPtr->BasicType == EDSLIB_BASICTYPE_ARRAY)
            {
                /*
                 * Because structure elements might contain padding,
                 * we reset the "AbsOffset" every time through, recalculate
                 * by taking the parent AbsOffset and adding the start offset to the
                 * current element.
                 */
                if (CurrLev->CurrIndex == 0)
                {
                    /* Lookup the actual array type.  Unlike containers, this only needs to be
                     * done on the first time since all entries are of the same type.
                     *
                     * This calculates the END offset of this element based on the parent size.  Doing
                     * it this way accounts for (potential) extra padding between elements, such as
                     * cases where we have an array of base type containers.  This would not be accounted
                     * for when using the "SizeInfo" of the current level directly, as this would only be
                     * the size of the base type.
                     */
                    CurrLev->Details.RefObj = ParentLev->DataDictPtr->Detail.Array->ElementRefObj;
                    CurrLev->DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &CurrLev->Details.RefObj);
                    CurrLev->Details.EntryType = EDSLIB_ENTRYTYPE_ARRAY_ELEMENT;
                    CurrLev->StartOffset = ParentLev->StartOffset;
                    CurrLev->EndOffset.Bytes = (ParentLev->EndOffset.Bytes - ParentLev->StartOffset.Bytes);
                    CurrLev->EndOffset.Bits = (ParentLev->EndOffset.Bits - ParentLev->StartOffset.Bits);
                    CurrLev->EndOffset.Bytes /= ParentLev->DataDictPtr->NumSubElements;
                    CurrLev->EndOffset.Bits /= ParentLev->DataDictPtr->NumSubElements;
                    CurrLev->EndOffset.Bytes += CurrLev->StartOffset.Bytes;
                    CurrLev->EndOffset.Bits += CurrLev->StartOffset.Bits;
                }
                else
                {
                    /*
                     * Adjust the start/end offsets to the next element
                     */
                    EdsLib_SizeInfo_t ElementSize;
                    ElementSize.Bytes = CurrLev->EndOffset.Bytes - CurrLev->StartOffset.Bytes;
                    ElementSize.Bits = CurrLev->EndOffset.Bits - CurrLev->StartOffset.Bits;
                    CurrLev->StartOffset = CurrLev->EndOffset;
                    CurrLev->EndOffset.Bytes += ElementSize.Bytes;
                    CurrLev->EndOffset.Bits += ElementSize.Bits;
                }
            }
            else if (ParentLev->DataDictPtr->BasicType == EDSLIB_BASICTYPE_CONTAINER)
            {
                /* Copy the entry details directly from the DB */
                CurrLev->Details = ParentLev->DataDictPtr->Detail.Container->
                        EntryList[CurrLev->CurrIndex];

                /*
                 * Current level is a structure container, all sub-elements are a different type;
                 * so structure ID and type info must be reinitialized for each member
                 */
                if (CurrLev->CurrIndex == 0)
                {
                    /*
                     * Usually the offset of the first entry is zero, except in case
                     * where there is leading padding in a container.  So it must be
                     * added now.
                     */
                    CurrLev->StartOffset.Bytes = ParentLev->StartOffset.Bytes +
                            CurrLev->Details.Offset.Bytes;
                    CurrLev->StartOffset.Bits = ParentLev->StartOffset.Bits +
                            CurrLev->Details.Offset.Bits;
                }
                else
                {
                    /* The start of the current element is the end of the previous one */
                    CurrLev->StartOffset = CurrLev->EndOffset;
                }
                if (CurrLev->CurrIndex < (ParentLev->DataDictPtr->NumSubElements-1))
                {
                    /* peek ahead to the offset of the _next_ entry.  This is done
                     * for the same reason as the array entries - to include for padding/buffer
                     * space between elements in case an entry is actually a base type */
                    CurrLev->EndOffset = ParentLev->DataDictPtr->Detail.Container->
                            EntryList[1+CurrLev->CurrIndex].Offset;
                    CurrLev->EndOffset.Bytes += ParentLev->StartOffset.Bytes;
                    CurrLev->EndOffset.Bits += ParentLev->StartOffset.Bits;
                }
                else
                {
                    /* use the parent end offset */
                    CurrLev->EndOffset = ParentLev->EndOffset;
                }
                CurrLev->DataDictPtr = EdsLib_DataTypeDB_GetEntry(GD, &CurrLev->Details.RefObj);
            }
            else
            {
                /* Should not get here; bug in database? */
                NextAction = EDSLIB_ITERATOR_RC_ASCEND;
            }

            if (NextAction == EDSLIB_ITERATOR_RC_CONTINUE)
            {
                NextAction = StateInfo->Callback(GD, EDSLIB_ITERATOR_CBTYPE_MEMBER, CurrLev, StateInfo->CallbackArg);
                ++CurrLev->CurrIndex;
            }
            break;

        case EDSLIB_ITERATOR_RC_STOP:
            break;

        default:
            NextAction = EDSLIB_ITERATOR_RC_CONTINUE;
            break;
        }
    }

    return Status;
}


