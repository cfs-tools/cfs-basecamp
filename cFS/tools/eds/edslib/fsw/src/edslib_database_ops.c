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
 * \file     edslib_database_ops.c
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

#include "edslib_database_ops.h"

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 * This converts between the EdsLib_Id_t used in the public API, and
 * the EdsLib_DatabaseRef_t used in the internal DB.
 *
 *-----------------------------------------------------------------*/
void EdsLib_Decode_StructId(EdsLib_DatabaseRef_t *RefObj, EdsLib_Id_t EdsId)
{
    RefObj->AppIndex = EdsLib_Get_AppIdx(EdsId);
    RefObj->SubIndex = EdsLib_Get_FormatIdx(EdsId);

    switch (EdsLib_Get_Genre(EdsId))
    {
        case EDSLIB_ID_GENRE_DATATYPE_BITS:
            RefObj->Qualifier = EdsLib_DbRef_Qualifier_DATATYPE;
            break;
        case EDSLIB_ID_GENRE_INTF_BITS:
            RefObj->Qualifier = EdsLib_DbRef_Qualifier_INTERFACE;
            break;
        case EDSLIB_ID_GENRE_USERDEF1_BITS:
            RefObj->Qualifier = EdsLib_DbRef_Qualifier_USERDEF1;
            break;
        case EDSLIB_ID_GENRE_USERDEF2_BITS:
            RefObj->Qualifier = EdsLib_DbRef_Qualifier_USERDEF2;
            break;
        default:
            /* Unspecified qualifier (this could be a problem) */
            RefObj->Qualifier = EdsLib_DbRef_Qualifier_UNSPECIFIED;
            break;
    }
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 * This converts between the EdsLib_Id_t used in the public API, and
 * the EdsLib_DatabaseRef_t used in the internal DB.
 *
 *-----------------------------------------------------------------*/
EdsLib_Id_t EdsLib_Encode_StructId(const EdsLib_DatabaseRef_t *RefObj)
{
    EdsLib_Id_t Result;

    Result = EDSLIB_ID_INVALID;
    if (RefObj != NULL && RefObj->Qualifier != EdsLib_DbRef_Qualifier_UNSPECIFIED)
    {
        EdsLib_Set_AppIdx(&Result, RefObj->AppIndex);
        EdsLib_Set_FormatIdx(&Result, RefObj->SubIndex);

        switch (RefObj->Qualifier)
        {
            case EdsLib_DbRef_Qualifier_DATATYPE:
                EdsLib_Set_Genre(&Result, EDSLIB_ID_GENRE_DATATYPE_BITS);
                break;
            case EdsLib_DbRef_Qualifier_INTERFACE:
                EdsLib_Set_Genre(&Result, EDSLIB_ID_GENRE_INTF_BITS);
                break;
            case EdsLib_DbRef_Qualifier_USERDEF1:
                EdsLib_Set_Genre(&Result, EDSLIB_ID_GENRE_USERDEF1_BITS);
                break;
            case EdsLib_DbRef_Qualifier_USERDEF2:
                EdsLib_Set_Genre(&Result, EDSLIB_ID_GENRE_USERDEF2_BITS);
                break;
            default:
                /* Unspecified qualifier (this could be a problem) */
                break;
        }
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
bool EdsLib_DatabaseRef_IsEqual(const EdsLib_DatabaseRef_t *RefObj1, const EdsLib_DatabaseRef_t *RefObj2)
{
    return (RefObj1->Qualifier == RefObj2->Qualifier && RefObj1->AppIndex == RefObj2->AppIndex
            && RefObj1->SubIndex == RefObj2->SubIndex);
}
