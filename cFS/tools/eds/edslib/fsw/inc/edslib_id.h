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
 * \file     edslib_id.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * The Global Message Identifier attempts to boil down all the different structure formats into
 * a single 32-bit code that can consistently indicate how to interpret the data.
 *
 * It should be treated as an opaque object to the outside world, but the internal bit arrangement
 * is described here (subject to change).
 *
 * A number of basic accessor routines are implemented here as inline functions.
 * It is highly recommended to use these rather than interpreting the bits directly
 * in case things change (which they will).
 *
 * NOTE: there are a number of unused bits, these are reserved for future use.  The sub-fields
 * are byte-justified to make hex codes easier to interpret.  This need not be the case, i.e.
 * the spare bits could be consolidated to make room for a larger field if needed.
 *
 * It is currently arranged as follows:
 *   31                23                15                 7
 *  +-----------------+-----------------+-----------------+-----------------+
 *  |   G G C C C C C |   A A A A A A A |             F F | F F F F F F F F |
 *  +-----------------+-----------------+-----------------+-----------------+
 *  |      | CpuNum   |  AppId          |            |  Format Identifier   |
 *
 * G = Genre Identifier (2 bit) => If (0), this refers to an EDS-defined data type and
 *     should be used with the EdsLib_DataTypeDB API.  If (1) this refers to a interface and
 *     should be used with the EdsLib_IntfDB API.  Values 2 and 3 are reserved for use in
 *     additional user-defined database(s).
 * C = Cpu Number (5 bits) => Instance number in the range of 1-31.  (0 = any)
 *     This field is ignored for data types, as data types are never instance-specific.  The
 *     field should be set 0 on all data type references.  For interface/component references, 0 may be
 *     used as a placeholder value to indicate "any".  Valid instance numbers are in the
 *     range of 1-31.
 * A = App Number (7 bits) => EDS APP number in the range of 1-127 (0 = reserved/invalid)
 *     Note that the app number is the EDS_INDEX(XXX) enumeration, statically allocated
 *     by the EDS toolchain, and NOT the run-time app number that CFE might use.  This is so
 *     app numbers are always consistent from run-to-run, even if an app is not loaded or
 *     loaded multiple times.
 * F = Format Identifier (10 bits) => This value is specific to the AppID and suitable
 *     for looking up in the DataTypeTable field of the application's data dictionary.
 *     The value 0 is always invalid, toolchain allocates formats in the range of 1-1023.
 */

#ifndef EDSLIB_ID_H
#define EDSLIB_ID_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Type abstraction that is used for all message identifiers
 */
typedef uint32_t EdsLib_Id_t;

/**
 * An EdsLib_Id_t value that is never valid.
 * Because the appid of 0 is reserved, a valid EdsLib_Id_t can
 * never be all zero.
 */
#define EDSLIB_ID_INVALID ((EdsLib_Id_t)0)

/**
 * Some constants for interpreting the uint32_t.  User applications
 * should _not_ use these directly but they are defined here so the
 * inline accessor functions (below) can use them.
 */
enum
{
    EDSLIB_ID_SHIFT_INDEX  = 0,
    EDSLIB_ID_SHIFT_APP    = 16,
    EDSLIB_ID_SHIFT_CPUNUM = 24,
    EDSLIB_ID_SHIFT_GENRE  = 29,

    EDSLIB_ID_MASK_INDEX  = 0x3FF,
    EDSLIB_ID_MASK_APP    = 0x7F,
    EDSLIB_ID_MASK_CPUNUM = 0x1F,
    EDSLIB_ID_MASK_GENRE  = 0x3,

    EDSLIB_ID_INDEX_BITS  = EDSLIB_ID_MASK_INDEX << EDSLIB_ID_SHIFT_INDEX,
    EDSLIB_ID_APP_BITS    = EDSLIB_ID_MASK_APP << EDSLIB_ID_SHIFT_APP,
    EDSLIB_ID_CPUNUM_BITS = EDSLIB_ID_MASK_CPUNUM << EDSLIB_ID_SHIFT_CPUNUM,
    EDSLIB_ID_GENRE_BITS  = EDSLIB_ID_MASK_GENRE << EDSLIB_ID_SHIFT_GENRE,
};

/* Note, the bits for datatype are chosen so it will be the default if no genre is set */
/* The other possible values for Genre are reserved for use in derivative products (user-defined) */
#define EDSLIB_ID_GENRE_DATATYPE_BITS 0x0
#define EDSLIB_ID_GENRE_INTF_BITS     0x1
#define EDSLIB_ID_GENRE_USERDEF1_BITS 0x2
#define EDSLIB_ID_GENRE_USERDEF2_BITS 0x3

/***************************************************************************************
 * INITIALIZERS
 *
 * These macros may be used to statically initialize an EdsLib_Id_t value.  They are done
 * as macros rather than inline functions to allow use in compile-time initializers.
 ***************************************************************************************/

/**
 * Initialize an EdsLib_Id referring to a data type.
 *
 * The CPU number will be set to 0 (any) and the format type will be set
 * to indicate a generic data structure.
 *
 * @param AppIdx  An enumeration value from the main eds_index.h file
 * @param FormatIdx A value from the app-specific data dictionary enumeration.  These
 *        are the values from the APP_eds_defines.h file.
 */
#define EDSLIB_MAKE_ID(AppIdx, FormatIdx)                                 \
    ((EDSLIB_ID_GENRE_DATATYPE_BITS << EDSLIB_ID_SHIFT_GENRE)             \
     | ((uint32_t)((AppIdx) & EDSLIB_ID_MASK_APP) << EDSLIB_ID_SHIFT_APP) \
     | ((uint32_t)((FormatIdx) & EDSLIB_ID_MASK_INDEX) << EDSLIB_ID_SHIFT_INDEX))

/**
 * Initialize an EdsLib_Id referring to an interface.
 *
 * The CPU number will be set to 0 (any) and the format type will be set
 * to the specified interface
 *
 * @param AppIdx  An enumeration value from the main eds_index.h file
 * @param IntfIdx A value from the app-specific interface enumeration.  These
 *        are the values from the APP_eds_defines.h file.
 */
#define EDSLIB_INTF_ID(AppIdx, IntfIdx)                                   \
    ((EDSLIB_ID_GENRE_INTF_BITS << EDSLIB_ID_SHIFT_GENRE)                 \
     | ((uint32_t)((AppIdx) & EDSLIB_ID_MASK_APP) << EDSLIB_ID_SHIFT_APP) \
     | ((uint32_t)((IntfIdx) & EDSLIB_ID_MASK_INDEX) << EDSLIB_ID_SHIFT_INDEX))

/***************************************************************************************
 * BASIC ACCESSORS
 *
 * The functions below here are simple accessors that do not involve any external lookup.
 * They are implemented as inline functions.
 ***************************************************************************************/

/**
 * Gets the genre field in the EdsLib_Id_t
 */
static inline uint8_t EdsLib_Get_Genre(EdsLib_Id_t EdsId)
{
    return (EdsId >> EDSLIB_ID_SHIFT_GENRE) & EDSLIB_ID_MASK_GENRE;
}

/**
 * Sets the interface flag in the EdsLib_Id_t
 */
static inline void EdsLib_Set_Genre(EdsLib_Id_t *EdsId, uint8_t Genre)
{
    *EdsId &= ~EDSLIB_ID_GENRE_BITS;
    *EdsId |= (Genre & EDSLIB_ID_MASK_GENRE) << EDSLIB_ID_SHIFT_GENRE;
}

/**
 * Checks if the EdsLib_Id_t refers to a DataType
 */
static inline bool EdsLib_Is_DataType(EdsLib_Id_t EdsId)
{
    return EdsLib_Get_Genre(EdsId) == EDSLIB_ID_GENRE_DATATYPE_BITS;
}

/**
 * Checks if the EdsLib_Id_t refers to an Interface
 */
static inline bool EdsLib_Is_Interface(EdsLib_Id_t EdsId)
{
    return EdsLib_Get_Genre(EdsId) == EDSLIB_ID_GENRE_INTF_BITS;
}

/**
 * Gets the cpu number from the EdsLib_Id_t
 */
static inline uint16_t EdsLib_Get_CpuNumber(EdsLib_Id_t EdsId)
{
    return (EdsId >> EDSLIB_ID_SHIFT_CPUNUM) & EDSLIB_ID_MASK_CPUNUM;
}

/**
 * Sets the cpu number in the EdsLib_Id_t
 */
static inline void EdsLib_Set_CpuNumber(EdsLib_Id_t *EdsId, uint16_t CpuNum)
{
    *EdsId &= ~EDSLIB_ID_CPUNUM_BITS;
    *EdsId |= (CpuNum & EDSLIB_ID_MASK_CPUNUM) << EDSLIB_ID_SHIFT_CPUNUM;
}

/**
 * Gets the appid field from the EdsLib_Id_t
 */
static inline uint16_t EdsLib_Get_AppIdx(EdsLib_Id_t EdsId)
{
    return (EdsId >> EDSLIB_ID_SHIFT_APP) & EDSLIB_ID_MASK_APP;
}

/**
 * Sets the appid field in the EdsLib_Id_t
 */
static inline void EdsLib_Set_AppIdx(EdsLib_Id_t *EdsId, uint16_t AppIdx)
{
    *EdsId &= ~EDSLIB_ID_APP_BITS;
    *EdsId |= (AppIdx & EDSLIB_ID_MASK_APP) << EDSLIB_ID_SHIFT_APP;
}

/**
 * Gets the format index field from the EdsLib_Id_t
 */
static inline uint16_t EdsLib_Get_FormatIdx(EdsLib_Id_t EdsId)
{
    return ((EdsId >> EDSLIB_ID_SHIFT_INDEX) & EDSLIB_ID_MASK_INDEX);
}

/**
 * Sets the format index field in the EdsLib_Id_t
 */
static inline void EdsLib_Set_FormatIdx(EdsLib_Id_t *EdsId, uint16_t FormatIdx)
{
    *EdsId &= ~EDSLIB_ID_INDEX_BITS;
    *EdsId |= ((FormatIdx & EDSLIB_ID_MASK_INDEX) << EDSLIB_ID_SHIFT_INDEX);
}

/**
 * Boolean EdsLib_Id_t simple validity check function
 *
 * Note that this just checks if the EdsId has certain the bits set; it does not
 * do any actual data lookups to see if the data dictionary is actually present for it.
 *
 * @returns nonzero (true) if the EdsId passes basic validity checks
 */
static inline bool EdsLib_Is_Valid(EdsLib_Id_t EdsId)
{
    return ((EdsId & (EDSLIB_ID_INDEX_BITS | EDSLIB_ID_APP_BITS)) != 0);
}

/**
 * Boolean EdsLib_Id_t simple equality function
 *
 * Checks that the two msgindex values are equal, DISREGARDING the cpu number portion.
 *
 * @returns nonzero (true) if the EdsId refers to the same AppIdx and FormatIdx.
 */
static inline bool EdsLib_Is_Similar(EdsLib_Id_t EdsId1, EdsLib_Id_t EdsId2)
{
    return (((EdsId1 ^ EdsId2) & (EDSLIB_ID_GENRE_BITS | EDSLIB_ID_INDEX_BITS | EDSLIB_ID_APP_BITS)) == 0);
}

/**
 * Boolean EdsLib_Id_t simple equality function
 *
 * Checks that the two msgindex values are equal, INCLUDING the cpu number portion.
 *
 * @returns nonzero (true) if the EdsId values are exactly the same.
 */
static inline bool EdsLib_Is_Match(EdsLib_Id_t EdsId1, EdsLib_Id_t EdsId2)
{
    return (EdsId1 == EdsId2);
}

#endif /* _EDSLIB_ID_H_ */
