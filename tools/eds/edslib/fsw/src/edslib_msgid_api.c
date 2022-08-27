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
 * \file     edslib_msgid_api.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Public API functions for dealing with the EDS-defined "Mission-Unique"
 * message identifiers.  This includes:
 *  - looking up a descriptive name for a message id
 *  - converting a user-supplied string to a message id
 *
 * This is linked as part of the "full" EDS library
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "edslib_displaydb.h"
#include "edslib_internal.h"

EdsLib_Id_t EdsLib_DisplayDB_LookupTypeName(const EdsLib_DatabaseObject_t *GD, const char *String)
{
    EdsLib_DisplayDB_t NameDict;
    EdsLib_DataTypeDB_t DataDict;
    const EdsLib_DisplayDB_Entry_t *DisplayInfo;
    const char *EndPtr;
    size_t PartLength;
    uint16_t InstanceNum;
    uint16_t AppIdx;
    uint16_t StructId;
    EdsLib_Id_t Result;

    /*
     * A global structure ID is different than a message ID in two ways:
     *  - there is no cpu number
     *  - the format index goes directly into the map table
     */

    Result = EDSLIB_ID_INVALID;
    NameDict = NULL;
    InstanceNum = 0;

    if (GD->DisplayDB_Table != NULL && GD->DataTypeDB_Table != NULL)
    {
        /*
         * Next component is the EDS object name (required)
         * this is actually the fully-qualified name including the namespace parts
         * Note that the DB is organized by datasheets, NOT by namespace, so there is
         * no way to go directly to a namespace -- it could be scattered in multiple datasheets.
         * So this must do a sequential search
         */
        for (AppIdx = 0; AppIdx < GD->AppTableSize; ++AppIdx)
        {
            DataDict = GD->DataTypeDB_Table[AppIdx];
            NameDict = GD->DisplayDB_Table[AppIdx];
            if (NameDict == NULL || DataDict == NULL ||
                    NameDict->DisplayInfoTable == NULL)
            {
                continue;
            }

            DisplayInfo = NameDict->DisplayInfoTable;
            for (StructId = 0; StructId < DataDict->DataTypeTableSize; ++StructId, ++DisplayInfo)
            {
                EndPtr = String;
                if (DisplayInfo->Namespace != NULL)
                {
                    PartLength = strlen(DisplayInfo->Namespace);
                    if (strncmp(DisplayInfo->Namespace, String, PartLength) != 0 ||
                            String[PartLength] != '/')
                    {
                        continue;
                    }
                    EndPtr += PartLength + 1;
                }
                if (DisplayInfo->Name != NULL &&
                        strcmp(DisplayInfo->Name,EndPtr) == 0)
                {
                    /* Initialize the Global ID with the result */
                    Result = EDSLIB_MAKE_ID(AppIdx, StructId);
                    EdsLib_Set_CpuNumber(&Result, InstanceNum);
                    break;
                }
            }
        }
    }

    return Result;
}

