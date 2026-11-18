#ifndef EDSLIB_TEST_VECTORS_H
#define EDSLIB_TEST_VECTORS_H

#include "ccsds_sois_sampletypes_eds_datatypes.h"

extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_BOOLEANS_t                 UtRef_ALL_BOOLEANS;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_FLOATS_t                   UtRef_ALL_FLOATS;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_SIGNED_INTS_t              UtRef_ALL_SIGNED_INTS;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ALL_UNSIGNED_INTS_t            UtRef_ALL_UNSIGNED_INTS;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_COMPOUND_TYPES_t               UtRef_COMPOUND_TYPES;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_F64_t                    UtRef_DERIV_F64;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_U16_t                    UtRef_DERIV_U16;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_DERIV_U32_t                    UtRef_DERIV_U32;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t                      UtRef_ER_EVEN_FOUR;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t                      UtRef_ER_EVEN_SIX;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_EVEN_t                      UtRef_ER_EVEN_TWO;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t                       UtRef_ER_ODD_ONE;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t                       UtRef_ER_ODD_SEVEN;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ER_ODD_t                       UtRef_ER_ODD_THREE;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_t              UtRef_ERRCTL_CHECKSUM;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CHECKSUM_LONGITUDINAL_t UtRef_ERRCTL_CHECKSUM_LONGITUDINAL;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC16EL_t               UtRef_ERRCTL_CRC16EL;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_ERRCTL_CRC8_t                  UtRef_ERRCTL_CRC8;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_MFLOAT32EBxEU3_t               UtRef_MFLOAT32EBxEU3;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t              UtRef_OPTIONAL_DIRECT_350;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_DIRECT_t              UtRef_OPTIONAL_DIRECT_450;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t               UtRef_OPTIONAL_FIRST_4;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_FIRST_t               UtRef_OPTIONAL_FIRST_5;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t                UtRef_OPTIONAL_LAST_8;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_LAST_t                UtRef_OPTIONAL_LAST_9;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t                 UtRef_OPTIONAL_MID_6;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MID_t                 UtRef_OPTIONAL_MID_7;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t               UtRef_OPTIONAL_MULTI_10;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t               UtRef_OPTIONAL_MULTI_11;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t               UtRef_OPTIONAL_MULTI_12;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OPTIONAL_MULTI_t               UtRef_OPTIONAL_MULTI_13;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_OVERRIDE_TYPES_t               UtRef_OVERRIDE_TYPES;
extern EdsDataType_CCSDS_SOIS_SAMPLETYPES_S6EBx4_t                       UtRef_S6EBx4;

/* Packed objects */
/* These are generated from the SOIS test case3 bin files */

extern unsigned char TESTDATA_ALL_BOOLEANS_bin[];
extern unsigned int  TESTDATA_ALL_BOOLEANS_bin_len;
extern unsigned char TESTDATA_ALL_FLOATS_bin[];
extern unsigned int  TESTDATA_ALL_FLOATS_bin_len;
extern unsigned char TESTDATA_ALL_SIGNED_INTS_bin[];
extern unsigned int  TESTDATA_ALL_SIGNED_INTS_bin_len;
extern unsigned char TESTDATA_ALL_UNSIGNED_INTS_bin[];
extern unsigned int  TESTDATA_ALL_UNSIGNED_INTS_bin_len;
extern unsigned char TESTDATA_COMPOUND_TYPES_bin[];
extern unsigned int  TESTDATA_COMPOUND_TYPES_bin_len;
extern unsigned char TESTDATA_DERIV_F64_bin[];
extern unsigned int  TESTDATA_DERIV_F64_bin_len;
extern unsigned char TESTDATA_DERIV_U16_bin[];
extern unsigned int  TESTDATA_DERIV_U16_bin_len;
extern unsigned char TESTDATA_DERIV_U32_bin[];
extern unsigned int  TESTDATA_DERIV_U32_bin_len;
extern unsigned char TESTDATA_ER_EVEN_FOUR_bin[];
extern unsigned int  TESTDATA_ER_EVEN_FOUR_bin_len;
extern unsigned char TESTDATA_ER_EVEN_SIX_bin[];
extern unsigned int  TESTDATA_ER_EVEN_SIX_bin_len;
extern unsigned char TESTDATA_ER_EVEN_TWO_bin[];
extern unsigned int  TESTDATA_ER_EVEN_TWO_bin_len;
extern unsigned char TESTDATA_ER_ODD_ONE_bin[];
extern unsigned int  TESTDATA_ER_ODD_ONE_bin_len;
extern unsigned char TESTDATA_ER_ODD_SEVEN_bin[];
extern unsigned int  TESTDATA_ER_ODD_SEVEN_bin_len;
extern unsigned char TESTDATA_ER_ODD_THREE_bin[];
extern unsigned int  TESTDATA_ER_ODD_THREE_bin_len;
extern unsigned char TESTDATA_ERRCTL_CHECKSUM_bin[];
extern unsigned int  TESTDATA_ERRCTL_CHECKSUM_bin_len;
extern unsigned char TESTDATA_ERRCTL_CHECKSUM_LONGITUDINAL_bin[];
extern unsigned int  TESTDATA_ERRCTL_CHECKSUM_LONGITUDINAL_bin_len;
extern unsigned char TESTDATA_ERRCTL_CRC16EL_bin[];
extern unsigned int  TESTDATA_ERRCTL_CRC16EL_bin_len;
extern unsigned char TESTDATA_ERRCTL_CRC8_bin[];
extern unsigned int  TESTDATA_ERRCTL_CRC8_bin_len;
extern unsigned char TESTDATA_MFLOAT32EBxEU3_bin[];
extern unsigned int  TESTDATA_MFLOAT32EBxEU3_bin_len;
extern unsigned char TESTDATA_OPTIONAL_DIRECT_350_bin[];
extern unsigned int  TESTDATA_OPTIONAL_DIRECT_350_bin_len;
extern unsigned char TESTDATA_OPTIONAL_DIRECT_450_bin[];
extern unsigned int  TESTDATA_OPTIONAL_DIRECT_450_bin_len;
extern unsigned char TESTDATA_OPTIONAL_FIRST_4_bin[];
extern unsigned int  TESTDATA_OPTIONAL_FIRST_4_bin_len;
extern unsigned char TESTDATA_OPTIONAL_FIRST_5_bin[];
extern unsigned int  TESTDATA_OPTIONAL_FIRST_5_bin_len;
extern unsigned char TESTDATA_OPTIONAL_LAST_8_bin[];
extern unsigned int  TESTDATA_OPTIONAL_LAST_8_bin_len;
extern unsigned char TESTDATA_OPTIONAL_LAST_9_bin[];
extern unsigned int  TESTDATA_OPTIONAL_LAST_9_bin_len;
extern unsigned char TESTDATA_OPTIONAL_MID_6_bin[];
extern unsigned int  TESTDATA_OPTIONAL_MID_6_bin_len;
extern unsigned char TESTDATA_OPTIONAL_MID_7_bin[];
extern unsigned int  TESTDATA_OPTIONAL_MID_7_bin_len;
extern unsigned char TESTDATA_OPTIONAL_MULTI_10_bin[];
extern unsigned int  TESTDATA_OPTIONAL_MULTI_10_bin_len;
extern unsigned char TESTDATA_OPTIONAL_MULTI_11_bin[];
extern unsigned int  TESTDATA_OPTIONAL_MULTI_11_bin_len;
extern unsigned char TESTDATA_OPTIONAL_MULTI_12_bin[];
extern unsigned int  TESTDATA_OPTIONAL_MULTI_12_bin_len;
extern unsigned char TESTDATA_OPTIONAL_MULTI_13_bin[];
extern unsigned int  TESTDATA_OPTIONAL_MULTI_13_bin_len;
extern unsigned char TESTDATA_OVERRIDE_TYPES_bin[];
extern unsigned int  TESTDATA_OVERRIDE_TYPES_bin_len;
extern unsigned char TESTDATA_S6EBx4_bin[];
extern unsigned int  TESTDATA_S6EBx4_bin_len;

#endif
