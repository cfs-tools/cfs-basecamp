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
 * @file
 *
 * Auto-Generated stub implementations for functions defined in edslib_global header
 */

#include "edslib_global.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_FindPackageIdxByName()
 * ----------------------------------------------------
 */
int32_t EdsLib_FindPackageIdxByName(const EdsLib_DatabaseObject_t *GD, const char *PkgName, uint8_t *IdxOut)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_FindPackageIdxByName, int32_t);

    UT_GenStub_AddParam(EdsLib_FindPackageIdxByName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_FindPackageIdxByName, const char *, PkgName);
    UT_GenStub_AddParam(EdsLib_FindPackageIdxByName, uint8_t *, IdxOut);

    UT_GenStub_Execute(EdsLib_FindPackageIdxByName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_FindPackageIdxByName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_FindPackageIdxBySubstring()
 * ----------------------------------------------------
 */
int32_t EdsLib_FindPackageIdxBySubstring(const EdsLib_DatabaseObject_t *GD,
                                         const char                    *MatchString,
                                         size_t                         MatchLen,
                                         uint8_t                       *IdxOut)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_FindPackageIdxBySubstring, int32_t);

    UT_GenStub_AddParam(EdsLib_FindPackageIdxBySubstring, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_FindPackageIdxBySubstring, const char *, MatchString);
    UT_GenStub_AddParam(EdsLib_FindPackageIdxBySubstring, size_t, MatchLen);
    UT_GenStub_AddParam(EdsLib_FindPackageIdxBySubstring, uint8_t *, IdxOut);

    UT_GenStub_Execute(EdsLib_FindPackageIdxBySubstring, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_FindPackageIdxBySubstring, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_GetNumPackages()
 * ----------------------------------------------------
 */
uint16_t EdsLib_GetNumPackages(const EdsLib_DatabaseObject_t *GD)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_GetNumPackages, uint16_t);

    UT_GenStub_AddParam(EdsLib_GetNumPackages, const EdsLib_DatabaseObject_t *, GD);

    UT_GenStub_Execute(EdsLib_GetNumPackages, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_GetNumPackages, uint16_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_GetPackageNameNonNull()
 * ----------------------------------------------------
 */
const char *EdsLib_GetPackageNameNonNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_GetPackageNameNonNull, const char *);

    UT_GenStub_AddParam(EdsLib_GetPackageNameNonNull, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_GetPackageNameNonNull, uint8_t, PkgIdx);

    UT_GenStub_Execute(EdsLib_GetPackageNameNonNull, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_GetPackageNameNonNull, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_GetPackageNameOrNull()
 * ----------------------------------------------------
 */
const char *EdsLib_GetPackageNameOrNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_GetPackageNameOrNull, const char *);

    UT_GenStub_AddParam(EdsLib_GetPackageNameOrNull, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_GetPackageNameOrNull, uint8_t, PkgIdx);

    UT_GenStub_Execute(EdsLib_GetPackageNameOrNull, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_GetPackageNameOrNull, const char *);
}
