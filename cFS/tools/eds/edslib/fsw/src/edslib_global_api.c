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
 * \file     edslib_intfdb_api.c
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

#include "edslib_global.h"
#include "edslib_internal.h"

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
uint16_t EdsLib_GetNumPackages(const EdsLib_DatabaseObject_t *GD)
{
    uint16_t NumPackages = 0;

    if (GD != NULL)
    {
        NumPackages = GD->AppTableSize;
    }

    return NumPackages;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_FindPackageIdxBySubstring(const EdsLib_DatabaseObject_t *GD,
                                         const char                    *MatchString,
                                         size_t                         MatchLen,
                                         uint8_t                       *IdxOut)
{
    const char *NamePtr;
    int32_t     Status;
    uint16_t    i;

    Status = EDSLIB_NAME_NOT_FOUND;

    if (GD->AppName_Table != NULL)
    {
        for (i = 0; i < GD->AppTableSize; ++i)
        {
            NamePtr = GD->AppName_Table[i];

            if (NamePtr != NULL && strlen(NamePtr) == MatchLen && memcmp(NamePtr, MatchString, MatchLen) == 0)
            {
                if (IdxOut != NULL)
                {
                    /* This already accounts for the first entry (0) being unused */
                    *IdxOut = i;
                }
                Status = EDSLIB_SUCCESS;
                break;
            }
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_FindPackageIdxByName(const EdsLib_DatabaseObject_t *GD, const char *PkgName, uint8_t *IdxOut)
{
    /* This just matches against the complete null-terminated string */
    return EdsLib_FindPackageIdxBySubstring(GD, PkgName, strlen(PkgName), IdxOut);
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const char *EdsLib_GetPackageNameOrNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx)
{
    const char *Result;

    if (GD->AppName_Table != NULL && PkgIdx < GD->AppTableSize)
    {
        Result = GD->AppName_Table[PkgIdx];
    }
    else
    {
        Result = NULL;
    }

    return Result;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const char *EdsLib_GetPackageNameNonNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx)
{
    const char       *Result;
    static const char UNDEFINED_RESULT[] = "[undef]";

    Result = EdsLib_GetPackageNameOrNull(GD, PkgIdx);
    if (Result == NULL)
    {
        /* avoid returning NULL, which allows safe use within printf() style calls */
        Result = UNDEFINED_RESULT;
    }

    return Result;
}
