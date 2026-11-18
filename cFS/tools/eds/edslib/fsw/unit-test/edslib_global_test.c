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
 * \file     edslib_test.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
 * Unit test entry point
 */

#include "utassert.h"
#include "uttest.h"

#include "edslib_test_common.h"
#include "edslib_global.h"
#include "edslib_datatypedb.h"

void Test_EdsLib_GetNumPackages(void)
{
    /* Test case for:
     * uint16_t EdsLib_GetNumPackages(const EdsLib_DatabaseObject_t *GD);
     */
    UtAssert_UINT16_EQ(EdsLib_GetNumPackages(&UT_DATABASE), 5);
}

void Test_EdsLib_FindPackageIdxBySubstring(void)
{
    /* Test case for:
     * int32_t EdsLib_FindPackageIdxBySubstring(const EdsLib_DatabaseObject_t *GD, const char *MatchString, size_t
     * MatchLen, uint8_t *IdxOut);
     */
    uint8_t Idx;

    UtAssert_INT32_EQ(EdsLib_FindPackageIdxBySubstring(&UT_DATABASE, "UtApp/Something", 5, &Idx), EDSLIB_SUCCESS);
}

void Test_EdsLib_FindPackageIdxByName(void)
{
    /* Test case for:
     * int32_t EdsLib_FindPackageIdxByName(const EdsLib_DatabaseObject_t *GD, const char *PkgName, uint8_t *IdxOut);
     */
    uint8_t Idx;

    UtAssert_INT32_EQ(EdsLib_FindPackageIdxByName(&UT_DATABASE, "UtApp", &Idx), EDSLIB_SUCCESS);
}

void Test_EdsLib_GetPackageNameOrNull(void)
{
    /* Test case for:
     * const char *EdsLib_GetPackageNameOrNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx);
     */
    const char *Name;

    UtAssert_NOT_NULL(Name = EdsLib_GetPackageNameOrNull(&UT_DATABASE, UT_INDEX_UTAPP));
    UtAssert_STRINGBUF_EQ(Name, -1, "UtApp", -1);
    UtAssert_NULL(Name = EdsLib_GetPackageNameOrNull(&UT_DATABASE, 0));
}

void Test_EdsLib_GetPackageNameNonNull(void)
{
    /* Test case for:
     * const char *EdsLib_GetPackageNameNonNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx);
     */
    const char *Name;

    UtAssert_NOT_NULL(Name = EdsLib_GetPackageNameNonNull(&UT_DATABASE, UT_INDEX_UTHDR));
    UtAssert_STRINGBUF_EQ(Name, -1, "UtHdr", -1);
    UtAssert_NOT_NULL(Name = EdsLib_GetPackageNameNonNull(&UT_DATABASE, 0));
    UtAssert_STRINGBUF_EQ(Name, -1, "[undef]", -1);
}

void UtTest_Setup(void)
{
    UtTest_Add(Test_EdsLib_GetNumPackages, NULL, NULL, "EdsLib_GetNumPackages");
    UtTest_Add(Test_EdsLib_FindPackageIdxBySubstring, NULL, NULL, "EdsLib_FindPackageIdxBySubstring");
    UtTest_Add(Test_EdsLib_FindPackageIdxByName, NULL, NULL, "EdsLib_FindPackageIdxByName");
    UtTest_Add(Test_EdsLib_GetPackageNameOrNull, NULL, NULL, "EdsLib_GetPackageNameOrNull");
    UtTest_Add(Test_EdsLib_GetPackageNameNonNull, NULL, NULL, "EdsLib_GetPackageNameNonNull");
}
