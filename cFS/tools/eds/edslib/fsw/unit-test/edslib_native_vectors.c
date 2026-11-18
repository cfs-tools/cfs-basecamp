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
 * \file     edslib_native_vectors.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
 * Test vectors from the SOIS interoperability test
 * These are the native struct forms of the vectors
 */

#include "edslib_test_vectors.h"

/* clang-format off */
EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_BOOLEANS_t UtRef_ALL_BOOLEANS = {
    .normal = true,
    .invert = true,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_UNSIGNED_INTS_t UtRef_ALL_UNSIGNED_INTS = {
    .u8el       = 1,
    .u8eb       = 2,
    .u16el      = 3,
    .u16eb      = 4,
    .u32el      = 5,
    .u32eb      = 6,
    .u7         = 7,
    .u11        = 8,
    .u21        = 9,
    .u36        = 10,
    .fixed5_u21 = 5,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_SIGNED_INTS_t UtRef_ALL_SIGNED_INTS = {
    .s8el        = -1,
    .s16el       = -2,
    .s32el       = -3,
    .s6eb        = -4,
    .s15eb       = -5,
    .s27eb       = -6,
    .oc20eb      = -7,
    .sm40el      = -8,
    .bcd40eb     = 123,
    .bcd56el     = 456,
    .fixedminus4 = -4,
    .fixedplus10 = 10,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_FLOATS_t UtRef_ALL_FLOATS = {
    .mfloat32el  = -0.5,
    .mfloat32eb  = 20,
    .mfloat48el  = -40.5,
    .mfloat48eb  = 63.9995,
    .ifloat128eb = 4.5e-08,
    .ifloat128el = -10593194283.2,
    .ifloat64eb  = 594555.2,
    .ifloat64el  = -0.00032,
    .ifloat32eb  = -4053.1,
    .ifloat32el  = 0.74,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_COMPOUND_TYPES_t UtRef_COMPOUND_TYPES = {
    .s50ascii = "ascii_string",
    .s60utf8  = "utf8_string",
    .eu3      = EdsLabel_CCSDS_SOIS_SAMPLETYPES_EU3_FIVE,
    .es16     = EdsLabel_CCSDS_SOIS_SAMPLETYPES_ES16_MINUS10,
    .es24     = EdsLabel_CCSDS_SOIS_SAMPLETYPES_ES24_PLUS1000,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OVERRIDE_TYPES_t UtRef_OVERRIDE_TYPES = {
    .s32el  = -4954,
    .oc8    = 23,
    .oc24el = -13,
    .s15eb  = 6829,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC16EL_t UtRef_ERRCTL_CRC16EL = {
    .string = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy",
    .crc    = 10891,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC8_t UtRef_ERRCTL_CRC8 = {
    .string = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy",
    .crc    = 173,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_t UtRef_ERRCTL_CHECKSUM = {
    .string = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy",
    .csum   = 3016186466,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_LONGITUDINAL_t UtRef_ERRCTL_CHECKSUM_LONGITUDINAL = {
    .testdata1 = 555,
    .testdata2 = 1194593,
    .csum      = 147,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_S6EBx4_t UtRef_S6EBx4 = {
    5, -5, 12, -12,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_MFLOAT32EBxEU3_t UtRef_MFLOAT32EBxEU3 = {
    5, -5, 12, -12, 0, -2000, 42, 4000,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_U16_t UtRef_DERIV_U16 = {
    .BASE.fixed   = 21930,
    .BASE.ident   = 1,
    .BASE.length  = 112,
    .u16testdata1 = 111,
    .u16testdata2 = 222,
    .u16testdata3 = 333,
    .u16testdata4 = 444,
    .crc          = 7531,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_U32_t UtRef_DERIV_U32 = {
    .BASE.fixed   = 21930,
    .BASE.ident   = 2,
    .BASE.length  = 176,
    .u32testdata1 = 5555,
    .u32testdata2 = 6666,
    .u32testdata3 = 7777,
    .u32testdata4 = 8888,
    .crc          = 64165,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_F64_t UtRef_DERIV_F64 = {
    .BASE.fixed   = 21930,
    .BASE.ident   = 3,
    .BASE.length  = 304,
    .f64testdata1 = 0.1,
    .f64testdata2 = 2.2,
    .f64testdata3 = 33.3,
    .f64testdata4 = 444.4,
    .crc          = 18830,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t UtRef_OPTIONAL_FIRST_4 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 4,
    .BASE.length = 136,
    .optional1   = 92,
    .mandatory1  = 16640,
    .mandatory2  = 3606461230,
    .check       = 12,
    .crc         = 56108,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t UtRef_OPTIONAL_FIRST_5 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 5,
    .BASE.length = 128,
    .optional1   = 0,
    .mandatory1  = 7902,
    .mandatory2  = 3025745324,
    .check       = 12,
    .crc         = 36546,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t UtRef_OPTIONAL_MID_6 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 6,
    .BASE.length = 136,
    .mandatory1  = 2210,
    .optional1   = 204,
    .mandatory2  = 667934528,
    .check       = 123,
    .crc         = 48639,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t UtRef_OPTIONAL_MID_7 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 7,
    .BASE.length = 128,
    .mandatory1  = 17034,
    .mandatory2  = 1753013946,
    .check       = 123,
    .crc         = 60695,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t UtRef_OPTIONAL_LAST_8 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 8,
    .BASE.length = 104,
    .mandatory1  = 13556,
    .mandatory2  = 1408160266,
    .optional1   = 220,
    .crc         = 31764,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t UtRef_OPTIONAL_LAST_9 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 9,
    .BASE.length = 96,
    .mandatory1  = 12124,
    .mandatory2  = 560719646,
    .crc         = 11463,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t UtRef_OPTIONAL_MULTI_10 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 10,
    .BASE.length = 136,
    .mandatory1  = 3312,
    .optional1   = 0,
    .mandatory2  = 4053935192,
    .optional2   = 86,
    .check       = 1234,
    .crc         = 13458,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t UtRef_OPTIONAL_MULTI_11 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 11,
    .BASE.length = 152,
    .mandatory1  = 10036,
    .optional1   = 56380,
    .mandatory2  = 1116029528,
    .optional2   = 100,
    .check       = 1234,
    .crc         = 14337,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t UtRef_OPTIONAL_MULTI_12 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 12,
    .BASE.length = 152,
    .mandatory1  = 50624,
    .optional1   = 2204,
    .mandatory2  = 297133374,
    .optional2   = 196,
    .check       = 1234,
    .crc         = 62618,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t UtRef_OPTIONAL_MULTI_13 = {
    .BASE.fixed  = 21930,
    .BASE.ident  = 13,
    .BASE.length = 144,
    .mandatory1  = 62234,
    .optional1   = 53146,
    .mandatory2  = 2869396676,
    .optional2   = 0,
    .check       = 1234,
    .crc         = 50118,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t UtRef_OPTIONAL_DIRECT_350 = {
    .mandatory1 = 350,
    .optional1  = 0,
    .check      = 12345,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t UtRef_OPTIONAL_DIRECT_450 = {
    .mandatory1 = 450,
    .optional1  = 154,
    .check      = 12345,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t UtRef_ER_EVEN_FOUR = {
    .BASE2.fixed  = 21930,
    .BASE2.ident  = EdsLabel_CCSDS_SOIS_SAMPLETYPES_EU3_FOUR,
    .BASE2.length = 88,
    .optional1    = 0,
    .optional2    = 0,
    .mandatory1   = 57790,
    .crc          = 35195,
    .fixedCheck   = 1234567,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t UtRef_ER_EVEN_SIX = {
    .BASE2.fixed  = 21930,
    .BASE2.ident  = EdsLabel_CCSDS_SOIS_SAMPLETYPES_EU3_SIX,
    .BASE2.length = 104,
    .optional1    = 0,
    .optional2    = 24440,
    .mandatory1   = 9240,
    .crc          = 54782,
    .fixedCheck   = 1234567,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t UtRef_ER_EVEN_TWO = {
    .BASE2.fixed  = 21930,
    .BASE2.ident  = EdsLabel_CCSDS_SOIS_SAMPLETYPES_EU3_TWO,
    .BASE2.length = 112,
    .optional1    = 76,
    .optional2    = 24290,
    .mandatory1   = 4968,
    .crc          = 7363,
    .fixedCheck   = 1234567,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t UtRef_ER_ODD_ONE = {
    .BASE2.fixed  = 21930,
    .BASE2.ident  = EdsLabel_CCSDS_SOIS_SAMPLETYPES_EU3_ONE,
    .BASE2.length = 96,
    .optional1    = 112,
    .optional2    = 0,
    .mandatory1   = 41510,
    .crc          = 3498,
    .fixedCheck   = 1234567,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t UtRef_ER_ODD_SEVEN = {
    .BASE2.fixed  = 21930,
    .BASE2.ident  = EdsLabel_CCSDS_SOIS_SAMPLETYPES_EU3_SEVEN,
    .BASE2.length = 88,
    .optional1    = 0,
    .optional2    = 0,
    .mandatory1   = 1654,
    .crc          = 33645,
    .fixedCheck   = 1234567,
};

EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t UtRef_ER_ODD_THREE = {
    .BASE2.fixed  = 21930,
    .BASE2.ident  = EdsLabel_CCSDS_SOIS_SAMPLETYPES_EU3_THREE,
    .BASE2.length = 104,
    .optional1    = 0,
    .optional2    = 8688,
    .mandatory1   = 55066,
    .crc          = 39078,
    .fixedCheck   = 1234567,
};
