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
 * \file     edslib_pack_test.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
 * The intent of this test sequence is to validate the pack logic
 * This uses the same test vectors as the CCSDS SOIS interoperability tests
 * Native structs are passed in, and encoded output is verified against expected values
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "utassert.h"
#include "uttest.h"

#include "edslib_test_common.h"
#include "edslib_init.h"
#include "edslib_datatypedb.h"
#include "edslib_test_vectors.h"

void UtAssert_Eds_Contents(const void *ref, const void *actual, size_t sz)
{
    const uint8_t *actual_p = actual;
    const uint8_t *ref_p    = ref;
    char           hexdump_line[64];
    char           chardump_line[20];
    int            c;
    size_t         i;
    size_t         j;

    /* Start with a hexdump */
    i = 0;
    while (i < sz)
    {
        for (j = 0; j < 16 && i < sz; ++j)
        {
            c = actual_p[i];

            snprintf(&hexdump_line[j * 3], 4, " %02x", c);
            if (!isprint(c))
            {
                c = '.';
            }
            snprintf(&chardump_line[j], 2, "%c", c);

            ++i;
        }
        UtPrintf("OUTPUT@%3u:%-50s%-20s\n", (unsigned int)(i - j), hexdump_line, chardump_line);
    }

    /* Now report mismatches */
    for (i = 0; i < sz; ++i)
    {
        if (actual_p[i] != ref_p[i])
        {
            UtAssert_Failed("@%u: actual=0x%02x expected=0x%02x\n", (unsigned int)i, actual_p[i], ref_p[i]);
        }
    }
}

void Test_ALL_BOOLEANS(void)
{
    unsigned char *vector                                              = TESTDATA_ALL_BOOLEANS_bin;
    size_t         len                                                 = TESTDATA_ALL_BOOLEANS_bin_len;
    EdsLib_Id_t    EdsId_Expect                                        = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_BOOLEANS_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Base                                          = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_BOOLEANS_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_BOOLEANS_t *NativeObj = &UtRef_ALL_BOOLEANS;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_BOOLEANS_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_BOOLEANS_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_ALL_FLOATS(void)
{
    unsigned char *vector = TESTDATA_ALL_FLOATS_bin;
    size_t         len    = TESTDATA_ALL_FLOATS_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_FLOATS_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_FLOATS_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_FLOATS_t *NativeObj = &UtRef_ALL_FLOATS;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_FLOATS_t    PackObj;

    memset(&PackObj, 0xEE, sizeof(PackObj));

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_FLOATS_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_ALL_SIGNED_INTS(void)
{
    unsigned char *vector       = TESTDATA_ALL_SIGNED_INTS_bin;
    size_t         len          = TESTDATA_ALL_SIGNED_INTS_bin_len;
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_SIGNED_INTS_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_SIGNED_INTS_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_SIGNED_INTS_t *NativeObj = &UtRef_ALL_SIGNED_INTS;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_SIGNED_INTS_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_SIGNED_INTS_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_ALL_UNSIGNED_INTS(void)
{
    unsigned char *vector       = TESTDATA_ALL_UNSIGNED_INTS_bin;
    size_t         len          = TESTDATA_ALL_UNSIGNED_INTS_bin_len;
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_UNSIGNED_INTS_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_ALL_UNSIGNED_INTS_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_UNSIGNED_INTS_t *NativeObj = &UtRef_ALL_UNSIGNED_INTS;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_UNSIGNED_INTS_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ALL_UNSIGNED_INTS_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_COMPOUND_TYPES(void)
{
    unsigned char *vector       = TESTDATA_COMPOUND_TYPES_bin;
    size_t         len          = TESTDATA_COMPOUND_TYPES_bin_len;
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_COMPOUND_TYPES_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_COMPOUND_TYPES_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_COMPOUND_TYPES_t *NativeObj = &UtRef_COMPOUND_TYPES;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_COMPOUND_TYPES_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_COMPOUND_TYPES_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_DERIV_F64(void)
{
    unsigned char *vector = TESTDATA_DERIV_F64_bin;
    size_t         len    = TESTDATA_DERIV_F64_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_DERIV_F64_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_F64_t *NativeObj = &UtRef_DERIV_F64;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_DERIV_F64_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_DERIV_F64_t));
    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_BASE_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_DERIV_U16(void)
{
    unsigned char *vector = TESTDATA_DERIV_U16_bin;
    size_t         len    = TESTDATA_DERIV_U16_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_DERIV_U16_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_U16_t *NativeObj = &UtRef_DERIV_U16;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_DERIV_U16_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_DERIV_U16_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_DERIV_U32(void)
{
    unsigned char *vector = TESTDATA_DERIV_U32_bin;
    size_t         len    = TESTDATA_DERIV_U32_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_DERIV_U32_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_U32_t *NativeObj = &UtRef_DERIV_U32;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_DERIV_U32_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_DERIV_U32_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_ER_EVEN_FOUR(void)
{
    unsigned char *vector = TESTDATA_ER_EVEN_FOUR_bin;
    size_t         len    = TESTDATA_ER_EVEN_FOUR_bin_len;
    EdsLib_Id_t    EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE2_DATADICTIONARY);
    EdsLib_Id_t EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t *NativeObj = &UtRef_ER_EVEN_FOUR;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t    PackObj;
    EdsLib_SizeInfo_t                                   SizeInfo;

    /* Include validation that the var size API must be used for this */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_NOT_FIXED_SIZE);

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t));

    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_ER_EVEN_SIX(void)
{
    unsigned char *vector = TESTDATA_ER_EVEN_SIX_bin;
    size_t         len    = TESTDATA_ER_EVEN_SIX_bin_len;
    EdsLib_Id_t    EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE2_DATADICTIONARY);
    EdsLib_Id_t EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t *NativeObj = &UtRef_ER_EVEN_SIX;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t    PackObj;
    EdsLib_SizeInfo_t                                   SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_ER_EVEN_TWO(void)
{
    unsigned char *vector = TESTDATA_ER_EVEN_TWO_bin;
    size_t         len    = TESTDATA_ER_EVEN_TWO_bin_len;
    EdsLib_Id_t    EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE2_DATADICTIONARY);
    EdsLib_Id_t EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t *NativeObj = &UtRef_ER_EVEN_TWO;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t    PackObj;
    EdsLib_SizeInfo_t                                   SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_ER_ODD_ONE(void)
{
    unsigned char *vector = TESTDATA_ER_ODD_ONE_bin;
    size_t         len    = TESTDATA_ER_ODD_ONE_bin_len;
    EdsLib_Id_t    EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE2_DATADICTIONARY);
    EdsLib_Id_t EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t *NativeObj = &UtRef_ER_ODD_ONE;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t    PackObj;
    EdsLib_SizeInfo_t                                  SizeInfo;

    /* Include validation that the var size API must be used for this */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_NOT_FIXED_SIZE);

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_ER_ODD_SEVEN(void)
{
    unsigned char *vector = TESTDATA_ER_ODD_SEVEN_bin;
    size_t         len    = TESTDATA_ER_ODD_SEVEN_bin_len;
    EdsLib_Id_t    EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE2_DATADICTIONARY);
    EdsLib_Id_t EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t *NativeObj = &UtRef_ER_ODD_SEVEN;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t    PackObj;
    EdsLib_SizeInfo_t                                  SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_ER_ODD_THREE(void)
{
    unsigned char *vector = TESTDATA_ER_ODD_THREE_bin;
    size_t         len    = TESTDATA_ER_ODD_THREE_bin_len;
    EdsLib_Id_t    EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_BASE2_DATADICTIONARY);
    EdsLib_Id_t EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t *NativeObj = &UtRef_ER_ODD_THREE;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t    PackObj;
    EdsLib_SizeInfo_t                                  SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_ERRCTL_CHECKSUM(void)
{
    unsigned char *vector       = TESTDATA_ERRCTL_CHECKSUM_bin;
    size_t         len          = TESTDATA_ERRCTL_CHECKSUM_bin_len;
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_t *NativeObj = &UtRef_ERRCTL_CHECKSUM;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_ERRCTL_CHECKSUM_LONGITUDINAL(void)
{
    unsigned char *vector = TESTDATA_ERRCTL_CHECKSUM_LONGITUDINAL_bin;
    size_t         len    = TESTDATA_ERRCTL_CHECKSUM_LONGITUDINAL_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                       EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_LONGITUDINAL_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                       EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_LONGITUDINAL_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_LONGITUDINAL_t *NativeObj =
        &UtRef_ERRCTL_CHECKSUM_LONGITUDINAL;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_LONGITUDINAL_t PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_LONGITUDINAL_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_ERRCTL_CRC16EL(void)
{
    unsigned char *vector       = TESTDATA_ERRCTL_CRC16EL_bin;
    size_t         len          = TESTDATA_ERRCTL_CRC16EL_bin_len;
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC16EL_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC16EL_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC16EL_t *NativeObj = &UtRef_ERRCTL_CRC16EL;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC16EL_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC16EL_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_ERRCTL_CRC8(void)
{
    unsigned char *vector = TESTDATA_ERRCTL_CRC8_bin;
    size_t         len    = TESTDATA_ERRCTL_CRC8_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC8_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsContainer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC8_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC8_t *NativeObj = &UtRef_ERRCTL_CRC8;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC8_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC8_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_MFLOAT32EBxEU3(void)
{
    unsigned char *vector = TESTDATA_MFLOAT32EBxEU3_bin;
    size_t         len    = TESTDATA_MFLOAT32EBxEU3_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsArray_CCSDS_SOIS_SAMPLETYPES_MFLOAT32EBxEU3_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsArray_CCSDS_SOIS_SAMPLETYPES_MFLOAT32EBxEU3_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_MFLOAT32EB_t  *NativeObj = UtRef_MFLOAT32EBxEU3;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_MFLOAT32EBxEU3_t PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_MFLOAT32EBxEU3_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(UtRef_MFLOAT32EBxEU3)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_OPTIONAL_DIRECT_350(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_DIRECT_350_bin;
    size_t         len          = TESTDATA_OPTIONAL_DIRECT_350_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t *NativeObj = &UtRef_OPTIONAL_DIRECT_350;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t    PackObj;
    EdsLib_SizeInfo_t                                           SizeInfo;

    /* Include validation that the var size API must be used for this */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_NOT_FIXED_SIZE);

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_DIRECT_450(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_DIRECT_450_bin;
    size_t         len          = TESTDATA_OPTIONAL_DIRECT_450_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t *NativeObj = &UtRef_OPTIONAL_DIRECT_450;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t    PackObj;
    EdsLib_SizeInfo_t                                           SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_OPTIONAL_FIRST_4(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_FIRST_4_bin;
    size_t         len          = TESTDATA_OPTIONAL_FIRST_4_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t *NativeObj = &UtRef_OPTIONAL_FIRST_4;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t    PackObj;
    EdsLib_SizeInfo_t                                          SizeInfo;

    /* Include validation that the var size API must be used for this */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_NOT_FIXED_SIZE);

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_FIRST_5(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_FIRST_5_bin;
    size_t         len          = TESTDATA_OPTIONAL_FIRST_5_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t *NativeObj = &UtRef_OPTIONAL_FIRST_5;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t    PackObj;
    EdsLib_SizeInfo_t                                          SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_LAST_8(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_LAST_8_bin;
    size_t         len          = TESTDATA_OPTIONAL_LAST_8_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t *NativeObj = &UtRef_OPTIONAL_LAST_8;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t    PackObj;
    EdsLib_SizeInfo_t                                         SizeInfo;

    /* Include validation that the var size API must be used for this */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_NOT_FIXED_SIZE);

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_LAST_9(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_LAST_9_bin;
    size_t         len          = TESTDATA_OPTIONAL_LAST_9_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t *NativeObj = &UtRef_OPTIONAL_LAST_9;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t    PackObj;
    EdsLib_SizeInfo_t                                         SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_MID_6(void)
{
    unsigned char *vector                                              = TESTDATA_OPTIONAL_MID_6_bin;
    size_t         len                                                 = TESTDATA_OPTIONAL_MID_6_bin_len;
    EdsLib_Id_t    EdsId_Base                                          = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect                                        = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t *NativeObj = &UtRef_OPTIONAL_MID_6;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t    PackObj;
    EdsLib_SizeInfo_t                                        SizeInfo;

    /* Include validation that the var size API must be used for this */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_NOT_FIXED_SIZE);

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_MID_7(void)
{
    unsigned char *vector                                              = TESTDATA_OPTIONAL_MID_7_bin;
    size_t         len                                                 = TESTDATA_OPTIONAL_MID_7_bin_len;
    EdsLib_Id_t    EdsId_Base                                          = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect                                        = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t *NativeObj = &UtRef_OPTIONAL_MID_7;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t    PackObj;
    EdsLib_SizeInfo_t                                        SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_MULTI_10(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_MULTI_10_bin;
    size_t         len          = TESTDATA_OPTIONAL_MULTI_10_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t *NativeObj = &UtRef_OPTIONAL_MULTI_10;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t    PackObj;
    EdsLib_SizeInfo_t                                          SizeInfo;

    /* Include validation that the var size API must be used for this */
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_NOT_FIXED_SIZE);

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_MULTI_11(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_MULTI_11_bin;
    size_t         len          = TESTDATA_OPTIONAL_MULTI_11_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t *NativeObj = &UtRef_OPTIONAL_MULTI_11;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t    PackObj;
    EdsLib_SizeInfo_t                                          SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_MULTI_12(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_MULTI_12_bin;
    size_t         len          = TESTDATA_OPTIONAL_MULTI_12_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t *NativeObj = &UtRef_OPTIONAL_MULTI_12;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t    PackObj;
    EdsLib_SizeInfo_t                                          SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OPTIONAL_MULTI_13(void)
{
    unsigned char *vector       = TESTDATA_OPTIONAL_MULTI_13_bin;
    size_t         len          = TESTDATA_OPTIONAL_MULTI_13_bin_len;
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t *NativeObj = &UtRef_OPTIONAL_MULTI_13;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t    PackObj;
    EdsLib_SizeInfo_t                                          SizeInfo;

    UtAssert_LTEQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t));
    SizeInfo.Bits  = sizeof(PackObj) * 8;
    SizeInfo.Bytes = sizeof(*NativeObj);
    UtAssert_INT32_EQ(
        EdsLib_DataTypeDB_PackCompleteObjectVarSize(&UT_DATABASE, &EdsId_Base, PackObj, NativeObj, &SizeInfo),
        EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_UINT32_EQ(SizeInfo.Bits, len * 8);
    UtAssert_UINT32_EQ(SizeInfo.Bytes, sizeof(*NativeObj));
    UtAssert_Eds_Contents(vector, PackObj, len);
}
void Test_OVERRIDE_TYPES(void)
{
    unsigned char *vector       = TESTDATA_OVERRIDE_TYPES_bin;
    size_t         len          = TESTDATA_OVERRIDE_TYPES_bin_len;
    EdsLib_Id_t    EdsId_Expect = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                              EdsContainer_CCSDS_SOIS_SAMPLETYPES_OVERRIDE_TYPES_DATADICTIONARY);
    EdsLib_Id_t    EdsId_Base   = EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES,
                                            EdsContainer_CCSDS_SOIS_SAMPLETYPES_OVERRIDE_TYPES_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_OVERRIDE_TYPES_t *NativeObj = &UtRef_OVERRIDE_TYPES;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OVERRIDE_TYPES_t    PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_OVERRIDE_TYPES_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(*NativeObj)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void Test_S6EBx4(void)
{
    unsigned char *vector = TESTDATA_S6EBx4_bin;
    size_t         len    = TESTDATA_S6EBx4_bin_len;
    EdsLib_Id_t    EdsId_Expect =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsArray_CCSDS_SOIS_SAMPLETYPES_S6EBx4_DATADICTIONARY);
    EdsLib_Id_t EdsId_Base =
        EDSLIB_MAKE_ID(UT_INDEX_CCSDS_SOIS_SAMPLETYPES, EdsArray_CCSDS_SOIS_SAMPLETYPES_S6EBx4_DATADICTIONARY);
    const EdsDataType_CCSDS_SOIS_SAMPLETYPES_S6EB_t *NativeObj = UtRef_S6EBx4;
    EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_S6EBx4_t  PackObj;

    UtAssert_EQ(size_t, len, sizeof(EdsPackedBuffer_CCSDS_SOIS_SAMPLETYPES_S6EBx4_t));
    UtAssert_INT32_EQ(EdsLib_DataTypeDB_PackCompleteObject(&UT_DATABASE,
                                                           &EdsId_Base,
                                                           PackObj,
                                                           NativeObj,
                                                           sizeof(PackObj) * 8,
                                                           sizeof(UtRef_S6EBx4)),
                      EDSLIB_SUCCESS);
    UtAssert_INT32_EQ(EdsId_Base, EdsId_Expect);
    UtAssert_Eds_Contents(vector, PackObj, len);
}

void UtTest_Setup(void)
{
    EdsLib_Initialize();

    UtTest_Add(Test_ALL_BOOLEANS, NULL, NULL, "ALL_BOOLEANS");
    UtTest_Add(Test_ALL_FLOATS, NULL, NULL, "ALL_FLOATS");
    UtTest_Add(Test_ALL_SIGNED_INTS, NULL, NULL, "ALL_SIGNED_INTS");
    UtTest_Add(Test_ALL_UNSIGNED_INTS, NULL, NULL, "ALL_UNSIGNED_INTS");
    UtTest_Add(Test_COMPOUND_TYPES, NULL, NULL, "COMPOUND_TYPES");
    UtTest_Add(Test_DERIV_F64, NULL, NULL, "DERIV_F64");
    UtTest_Add(Test_DERIV_U16, NULL, NULL, "DERIV_U16");
    UtTest_Add(Test_DERIV_U32, NULL, NULL, "DERIV_U32");
    UtTest_Add(Test_ER_EVEN_FOUR, NULL, NULL, "ER_EVEN_FOUR");
    UtTest_Add(Test_ER_EVEN_SIX, NULL, NULL, "ER_EVEN_SIX");
    UtTest_Add(Test_ER_EVEN_TWO, NULL, NULL, "ER_EVEN_TWO");
    UtTest_Add(Test_ER_ODD_ONE, NULL, NULL, "ER_ODD_ONE");
    UtTest_Add(Test_ER_ODD_SEVEN, NULL, NULL, "ER_ODD_SEVEN");
    UtTest_Add(Test_ER_ODD_THREE, NULL, NULL, "ER_ODD_THREE");
    UtTest_Add(Test_ERRCTL_CHECKSUM, NULL, NULL, "ERRCTL_CHECKSUM");
    UtTest_Add(Test_ERRCTL_CHECKSUM_LONGITUDINAL, NULL, NULL, "ERRCTL_CHECKSUM_LONGITUDINAL");
    UtTest_Add(Test_ERRCTL_CRC16EL, NULL, NULL, "ERRCTL_CRC16EL");
    UtTest_Add(Test_ERRCTL_CRC8, NULL, NULL, "ERRCTL_CRC8");
    UtTest_Add(Test_MFLOAT32EBxEU3, NULL, NULL, "MFLOAT32EBxEU3");
    UtTest_Add(Test_OPTIONAL_DIRECT_350, NULL, NULL, "OPTIONAL_DIRECT_350");
    UtTest_Add(Test_OPTIONAL_DIRECT_450, NULL, NULL, "OPTIONAL_DIRECT_450");
    UtTest_Add(Test_OPTIONAL_FIRST_4, NULL, NULL, "OPTIONAL_FIRST_4");
    UtTest_Add(Test_OPTIONAL_FIRST_5, NULL, NULL, "OPTIONAL_FIRST_5");
    UtTest_Add(Test_OPTIONAL_LAST_8, NULL, NULL, "OPTIONAL_LAST_8");
    UtTest_Add(Test_OPTIONAL_LAST_9, NULL, NULL, "OPTIONAL_LAST_9");
    UtTest_Add(Test_OPTIONAL_MID_6, NULL, NULL, "OPTIONAL_MID_6");
    UtTest_Add(Test_OPTIONAL_MID_7, NULL, NULL, "OPTIONAL_MID_7");
    UtTest_Add(Test_OPTIONAL_MULTI_10, NULL, NULL, "OPTIONAL_MULTI_10");
    UtTest_Add(Test_OPTIONAL_MULTI_11, NULL, NULL, "OPTIONAL_MULTI_11");
    UtTest_Add(Test_OPTIONAL_MULTI_12, NULL, NULL, "OPTIONAL_MULTI_12");
    UtTest_Add(Test_OPTIONAL_MULTI_13, NULL, NULL, "OPTIONAL_MULTI_13");
    UtTest_Add(Test_OVERRIDE_TYPES, NULL, NULL, "OVERRIDE_TYPES");
    UtTest_Add(Test_S6EBx4, NULL, NULL, "S6EBx4");
}
