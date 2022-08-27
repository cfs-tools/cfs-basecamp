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
 * \file     edslib_datatypedb_lookup.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Internal API functions for dealing with the EDS-defined type identifiers
 * that are used in the external API.  These are safe indices into the internal database,
 * which can be converted into a pointer into the DB.
 *
 * Since the index value can be checked and verified before actually dereferencing the
 * data, it is safer to use than a direct memory pointer.  However all internal API calls
 * generally pass around direct DB object pointers for efficiency.
 *
 * This is linked as part of the "basic" EDS library
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "edslib_internal.h"


void EdsLib_Decode_StructId(EdsLib_DatabaseRef_t *RefObj, EdsLib_Id_t EdsId)
{
    RefObj->AppIndex = EdsLib_Get_AppIdx(EdsId);
    RefObj->TypeIndex = EdsLib_Get_FormatIdx(EdsId);
}

EdsLib_Id_t EdsLib_Encode_StructId(const EdsLib_DatabaseRef_t *RefObj)
{
    if (RefObj == NULL)
    {
        return EDSLIB_ID_INVALID;
    }
    return EDSLIB_MAKE_ID(RefObj->AppIndex, RefObj->TypeIndex);
}

EdsLib_DataTypeDB_t EdsLib_DataTypeDB_GetTopLevel(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
{
    if (GD == NULL || GD->DataTypeDB_Table == NULL)
    {
       return NULL;
    }

    if (AppIdx >= GD->AppTableSize)
    {
       return NULL;
    }

    return GD->DataTypeDB_Table[AppIdx];
}

const EdsLib_DataTypeDB_Entry_t *EdsLib_DataTypeDB_GetEntry(const EdsLib_DatabaseObject_t *GD, const EdsLib_DatabaseRef_t *RefObj)
{
    EdsLib_DataTypeDB_t Dict;

    if (RefObj == NULL)
    {
        return NULL;
    }

    Dict = EdsLib_DataTypeDB_GetTopLevel(GD, RefObj->AppIndex);
    if (Dict == NULL)
    {
        return NULL;
    }

    if (RefObj->TypeIndex >= Dict->DataTypeTableSize)
    {
        return NULL;
    }

    return &Dict->DataTypeTable[RefObj->TypeIndex];
}


void EdsLib_DataTypeDB_CopyTypeInfo(const EdsLib_DataTypeDB_Entry_t *DataDictEntry, EdsLib_DataTypeDB_TypeInfo_t *TypeInfo)
{
    memset(TypeInfo, 0, sizeof(*TypeInfo));
    if (DataDictEntry != NULL)
    {
        TypeInfo->Size = DataDictEntry->SizeInfo;
        TypeInfo->ElemType = DataDictEntry->BasicType;
        TypeInfo->NumSubElements = DataDictEntry->NumSubElements;
    }
}

