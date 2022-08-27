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
 * \file     edslib_database_types.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Data Types and constants used internally to the EDS library.
 *
 * Objects of these types are defined by the generated EDS database files and
 * these types are also used throughout the runtime library.
 *
 * HOWEVER - These typedefs are internal to the EDS runtime library and are
 * not be exposed directly via public API.  All access into these database objects
 * must be done via one of the many accessor functions in the public API, but
 * no direct references or pointers to these objects should ever be returned
 * to user application code.
 */

#ifndef _EDSLIB_DATABASE_TYPES_H_
#define _EDSLIB_DATABASE_TYPES_H_


#ifndef _EDSLIB_BUILD_
#error "Do not include edslib_database_types.h from application code; use the edslib API instead"
#endif

#ifdef __cplusplus
#error "This file is not intended to be compiled with a C++ compiler"
#endif


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "edslib_api_types.h"


/*******************************************************
 * BASIC DATA TYPE DATABASE COMPONENTS
 *******************************************************/

struct EdsLib_DatabaseRef
{
    uint16_t AppIndex;
    uint16_t TypeIndex;
};

typedef struct EdsLib_DatabaseRef EdsLib_DatabaseRef_t;

typedef intmax_t (*EdsLib_IntegerCalibratorFunc_t)(intmax_t x);
typedef double (*EdsLib_FloatingPointCalibratorFunc_t)(double x);

typedef enum
{
    EdsLib_ErrorControlType_INVALID = 0,
    EdsLib_ErrorControlType_CHECKSUM,
    EdsLib_ErrorControlType_CHECKSUM_LONGITUDINAL,
    EdsLib_ErrorControlType_CRC8,
    EdsLib_ErrorControlType_CRC16_CCITT,
    EdsLib_ErrorControlType_CRC32,
    EdsLib_ErrorControlType_MAX
} EdsLib_ErrorControlType_t;

struct EdsLib_IntegerCalPair
{
    EdsLib_IntegerCalibratorFunc_t Forward;
    EdsLib_IntegerCalibratorFunc_t Reverse;
};

typedef struct EdsLib_IntegerCalPair EdsLib_IntegerCalPair_t;

struct EdsLib_FloatCalPair
{
    EdsLib_IntegerCalibratorFunc_t Forward;
    EdsLib_IntegerCalibratorFunc_t Reverse;
};

typedef struct EdsLib_FloatCalPair EdsLib_FloatCalPair_t;

union EdsLib_HandlerArgument
{
    EdsLib_ErrorControlType_t ErrorControl;
    EdsLib_FloatCalPair_t FloatCalibrator;
    EdsLib_IntegerCalPair_t IntegerCalibrator;
    const char *FixedString;
    double FixedFloat;
    intmax_t FixedInteger;
    uintmax_t FixedUnsigned;
};

typedef union EdsLib_HandlerArgument EdsLib_HandlerArgument_t;

typedef enum
{
    EDSLIB_ENTRYTYPE_DEFAULT = 0,
    EDSLIB_ENTRYTYPE_BASE_TYPE,
    EDSLIB_ENTRYTYPE_ARRAY_ELEMENT,
    EDSLIB_ENTRYTYPE_CONTAINER_ENTRY,
    EDSLIB_ENTRYTYPE_CONTAINER_PADDING_ENTRY,
    EDSLIB_ENTRYTYPE_CONTAINER_LIST_ENTRY,
    EDSLIB_ENTRYTYPE_CONTAINER_FIXED_VALUE_ENTRY,
    EDSLIB_ENTRYTYPE_CONTAINER_LENGTH_ENTRY,
    EDSLIB_ENTRYTYPE_CONTAINER_ERROR_CONTROL_ENTRY,
    EDSLIB_ENTRYTYPE_PROVIDED_INTERFACE,
    EDSLIB_ENTRYTYPE_REQUIRED_INTERFACE,
    EDSLIB_ENTRYTYPE_PARAMETER
} EdsLib_EntryType_t;

struct EdsLib_FieldDetailEntry
{
    uint16_t EntryType;
    EdsLib_SizeInfo_t Offset;
    EdsLib_DatabaseRef_t RefObj;
    EdsLib_HandlerArgument_t HandlerArg;
};

typedef struct EdsLib_FieldDetailEntry EdsLib_FieldDetailEntry_t;

struct EdsLib_DerivativeEntry
{
    uint16_t IdentSeqIdx;
    EdsLib_DatabaseRef_t RefObj;
};

typedef struct EdsLib_DerivativeEntry EdsLib_DerivativeEntry_t;

union EdsLib_RefValue
{
    const char *String;
    intmax_t Integer;
    uintmax_t Unsigned;
};

typedef union EdsLib_RefValue EdsLib_RefValue_t;

struct EdsLib_ValueEntry
{
    EdsLib_RefValue_t RefValue;
};

typedef struct EdsLib_ValueEntry EdsLib_ValueEntry_t;

struct EdsLib_ConstraintEntity
{
    EdsLib_SizeInfo_t Offset;
    EdsLib_DatabaseRef_t RefObj;
};

typedef struct EdsLib_ConstraintEntity EdsLib_ConstraintEntity_t;

struct EdsLib_IdentSequenceEntry
{
    uint16_t EntryType;
    uint16_t NextOperationLess;
    uint16_t NextOperationGreater;
    uint16_t ParentOperation;
    uint16_t RefIdx;
};

typedef struct EdsLib_IdentSequenceEntry EdsLib_IdentSequenceEntry_t;

struct EdsLib_ContainerDescriptor
{
    EdsLib_SizeInfo_t MaxSize;
    uint16_t IdentSequenceBase;
    uint16_t DerivativeListSize;
    uint16_t ConstraintEntityListSize;
    uint16_t ValueListSize;
    const EdsLib_FieldDetailEntry_t *EntryList;
    const EdsLib_FieldDetailEntry_t *TrailerEntryList;
    const EdsLib_DerivativeEntry_t *DerivativeList;
    const EdsLib_IdentSequenceEntry_t *IdentSequenceList;
    const EdsLib_ConstraintEntity_t *ConstraintEntityList;
    const EdsLib_ValueEntry_t *ValueList;
};

typedef struct EdsLib_ContainerDescriptor EdsLib_ContainerDescriptor_t;

struct EdsLib_ArrayDescriptor
{
    EdsLib_DatabaseRef_t ElementRefObj;
};

typedef struct EdsLib_ArrayDescriptor EdsLib_ArrayDescriptor_t;

typedef enum
{
    EDSLIB_NUMBERBYTEORDER_UNDEFINED = 0,
    EDSLIB_NUMBERBYTEORDER_BIG_ENDIAN,
    EDSLIB_NUMBERBYTEORDER_LITTLE_ENDIAN
} EdsLib_NumberByteOrder_Enum_t;

typedef enum
{
    EDSLIB_NUMBERENCODING_UNDEFINED = 0,
    EDSLIB_NUMBERENCODING_UNSIGNED_INTEGER,
    EDSLIB_NUMBERENCODING_SIGN_MAGNITUDE,
    EDSLIB_NUMBERENCODING_ONES_COMPLEMENT,
    EDSLIB_NUMBERENCODING_TWOS_COMPLEMENT,
    EDSLIB_NUMBERENCODING_BCD_OCTET,
    EDSLIB_NUMBERENCODING_BCD_PACKED,
    EDSLIB_NUMBERENCODING_IEEE_754,
    EDSLIB_NUMBERENCODING_MILSTD_1750A
} EdsLib_NumberEncoding_Enum_t;

struct EdsLib_NumberDescriptor
{
    uint8_t Encoding;
    uint8_t ByteOrder;
    uint8_t BitInvertFlag;
    uint8_t LsbFirstFlag;
};

typedef struct EdsLib_NumberDescriptor EdsLib_NumberDescriptor_t;

typedef enum
{
    EDSLIB_STRINGENCODING_UNDEFINED = 0,
    EDSLIB_STRINGENCODING_ASCII,
    EDSLIB_STRINGENCODING_UTF8
} EdsLib_StringEncoding_Enum_t;

struct EdsLib_StringDescriptor
{
    uint8_t Encoding;
};

typedef struct EdsLib_StringDescriptor EdsLib_StringDescriptor_t;

union EdsLib_ObjectDetailDescriptor
{
    const void *Ptr;
    const EdsLib_ContainerDescriptor_t *Container;
    const EdsLib_ArrayDescriptor_t *Array;
    const EdsLib_StringDescriptor_t String;
    const EdsLib_NumberDescriptor_t Number;
};

typedef union EdsLib_ObjectDetailDescriptor EdsLib_ObjectDetailDescriptor_t;


/*
 * Note: The Packing Flags here correlate with the byte order check values
 * in edslib_datatypedb_pack_unpack.c
 */
#define EDSLIB_DATATYPE_FLAG_NONE           0x00
#define EDSLIB_DATATYPE_FLAG_PACKED_BE      0x01
#define EDSLIB_DATATYPE_FLAG_PACKED_LE      0x02
#define EDSLIB_DATATYPE_FLAG_PACKED_MASK    0x03

struct EdsLib_DataTypeDB_Entry
{
    uint64_t Checksum;
    uint8_t BasicType;
    uint8_t Flags;
    uint16_t NumSubElements;
    EdsLib_SizeInfo_t SizeInfo;
    EdsLib_ObjectDetailDescriptor_t Detail;
};

typedef struct EdsLib_DataTypeDB_Entry EdsLib_DataTypeDB_Entry_t;


typedef enum
{
    EDSLIB_IDENT_SEQUENCE_INVALID = 0,      /**< Reserved marker, processing should stop when encountered */
    EDSLIB_IDENT_SEQUENCE_ENTITY_LOCATION,  /**< Load/Save entity from location */
    EDSLIB_IDENT_SEQUENCE_VALUE_CONDITION,  /**< Set/Check current value using the value constraint table */
    EDSLIB_IDENT_SEQUENCE_RANGE_CONDITION,  /**< Set/Check current value using the range constraint table */
    EDSLIB_IDENT_SEQUENCE_TYPE_CONDITION,   /**< Set/Check current value using the type constraint table */
    EDSLIB_IDENT_SEQUENCE_RESULT            /**< A positive identification entry */
} EdsLib_IdentSequence_Enum_t;


/*******************************************************
 * DISPLAY DATABASE COMPONENTS (extends basic info above)
 *******************************************************/

struct EdsLib_SymbolTableEntry
{
   intmax_t SymValue;
   const char *SymName;
};

typedef struct EdsLib_SymbolTableEntry EdsLib_SymbolTableEntry_t;

union EdsLib_DisplayArg
{
    EdsLib_DatabaseRef_t RefObj;
    const char * const *NameTable;
    const EdsLib_SymbolTableEntry_t *SymTable;
    void *ArgValue;
};

typedef union EdsLib_DisplayArg EdsLib_DisplayArg_t;

struct EdsLib_DisplayDB_Entry
{
    uint16_t DisplayHint;           /**< Type of display logic to apply */
    uint16_t DisplayArgTableSize;
    EdsLib_DisplayArg_t DisplayArg; /**< Optional extra data - typically the name table for enums or containers */
    const char *Namespace;          /**< Namespace of entry */
    const char *Name;               /**< Friendly name of data type or component */
};

typedef struct EdsLib_DisplayDB_Entry EdsLib_DisplayDB_Entry_t;


/*******************************************************
 * TOP LEVEL OBJECTS
 *
 * These may be referenced by user application code outside
 * of this interpreter library as abstract objects, and
 * used as concrete objects inside the EdsLib API.
 *******************************************************/

struct EdsLib_App_DataTypeDB
{
   uint16_t MissionIdx;
   uint16_t DataTypeTableSize;
   const EdsLib_DataTypeDB_Entry_t *DataTypeTable;
};

struct EdsLib_App_DisplayDB
{
   const char *EdsName;
   const EdsLib_DisplayDB_Entry_t *DisplayInfoTable;
};


#endif  /* _EDSLIB_DATABASE_TYPES_H_ */

