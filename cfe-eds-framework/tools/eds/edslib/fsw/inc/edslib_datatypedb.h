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
 * \file     edslib_datatypedb.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * BASIC MESSAGE DATA LOOKUP FUNCTIONS
 *
 * The API calls in this file require lookups from the data dictionary structures to
 * obtain the information.  They only deal with abstract types and offsets.  As such, they
 * do not use nor require a name dictionary to be loaded.
 *
 * Any API that deals with user-friendly member names must be put in the "full" API, not here.
 */

#ifndef _EDSLIB_DATATYPEDB_H_
#define _EDSLIB_DATATYPEDB_H_


/*
 * The EDS library uses the fixed-width types from the C99 stdint.h header file,
 * rather than the OSAL types.  This allows use in applications that are not
 * based on OSAL.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "edslib_api_types.h"
#include "edslib_id.h"

/******************************
 * MACROS
 ******************************/

/**
 * Maximum size of string / binary data members in EdsLib_GenericValueUnion_t objects
 *
 * In cases where a value constraint on an structure is a string type, this
 * defines the maximum length of the string.  Strings used as value constraints
 * in EDS container types are expected to be relatively short.
 *
 * This also represents the maximum number of digits for "BCD" value encoding types
 *
 */
#define EDSLIB_VALUEBUFFER_MAX_BINARY_SIZE      32



/******************************
 * TYPEDEFS
 ******************************/


/**
 * Return codes for Edslib functions, packed into a int32_t type.
 *
 * In keeping with common paradigms, negative values are failures, zero
 * represents complete/normal success, other non-negative values can be
 * used for special cases where some post-conditions are met.  Such
 * values should be documented in the error code and/or the function
 * as to what non-zero success codes mean.
 *
 * Note that over time, more return codes may be added in future versions.
 *
 * In general, user code should check if the returned value is less than
 * EDSLIB_SUCCESS (value < 0) rather than checking for specific error codes.
 * If specific error codes are checked, there MUST be a "catch-all" condition
 * if the code is to remain compatible with future library releases.
 */
enum
{
    EDSLIB_SUCCESS = 0,                 /**< Operation fully successful; all post-conditions are met */
    EDSLIB_FAILURE = -1,                /**< Unspecified failure, operation not successful; post-conditions are not met */
    EDSLIB_NOT_IMPLEMENTED = -2,        /**< Function is not implemented in this version of EdsLib */
    EDSLIB_INVALID_SIZE_OR_TYPE = -3,   /**< The user has specified an invalid size or type */
    EDSLIB_NAME_NOT_FOUND = -4,         /**< Element/Structure Name not found in database */
    EDSLIB_INCOMPLETE_DB_OBJECT = -5,   /**< A database element required for the requested operation is missing */
    EDSLIB_BUFFER_SIZE_ERROR = -6,      /**< A user-supplied buffer is insufficient for storing the requested object */
    EDSLIB_INVALID_INDEX = -7,          /**< A user-supplied index value is invalid */
    EDSLIB_NO_MATCHING_VALUE = -8,      /**< No matching values were found in the DB */
    EDSLIB_ERROR_CONTROL_MISMATCH = -9, /**< Error control field did not match */
    EDSLIB_FIELD_MISMATCH = -10,        /**< Length or Fixed Value field did not match */
    EDSLIB_INSUFFICIENT_MEMORY = -11    /**< Internal structure sizes were insufficient for operation */
};

/*
 * Typedefs representing the longest standard C99 type.
 * This is necessary if passing the value to a C library function, such as printf()
 * Intentionally using the native system type here, rather than fixed-width types,
 * to ensure that these will work correctly with specific printf() modifiers.
 */

/**
 * Type for holding generic signed integer values of the maximum width supported on the platform
 *
 * This is the largest width signed integer that the platform can handle.  It
 * may be passed to the C library printf() function using modifier %lld.
 */
typedef long long int           EdsLib_Generic_SignedInt_t;

/**
 * Type for holding generic unsigned integer values of the maximum width supported on the platform
 *
 * This is the largest width unsigned integer that the platform can handle.  It
 * may be passed to the C library printf() function using modifiers %llu or %llx.
 */
typedef unsigned long long int  EdsLib_Generic_UnsignedInt_t;

/**
 * Type for holding generic floating point values of the maximum precision supported on the platform
 *
 * This is the largest / highest precision floating point type that the platform can handle.  It
 * may be passed to the C library printf() function using modifiers %Lf or %Lg.
 */
typedef long double             EdsLib_Generic_FloatingPoint_t;

/**
 * General purpose union for storing a numeric data element of any standard width.
 *
 * Useful for allocating a buffer that is large enough and aligned for
 * storing any native numeric type, and conversions between them.  This is also
 * capable of holding a small string, in case of string value constraints.
 */
typedef union
{
    /* Fixed-width integer numbers */
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;

    /* Floating point numbers */
    float fpsgl;        /* single precision; probably 32 bits, but not guaranteed */
    double fpdbl;       /* double precision; probably 64 bits, but not guaranteed */
    long double fpmax;  /* extended precision, if supported.  Might alias to double. */

    /*
     * Buffer to hold a full IEEE-754 quad even if the
     * native machine FPU does not support quads (most do not).
     * This is fixed 128-bit size.  Implemented as four
     * 32-bit values for storage purposes.
     */
    uint32_t quad[4];

    /* String type (in case of e.g. a string value constraint) */
    char StringData[EDSLIB_VALUEBUFFER_MAX_BINARY_SIZE];

    /* member to allow data access as raw bytes */
    uint8_t BinaryData[EDSLIB_VALUEBUFFER_MAX_BINARY_SIZE];


    /*
     * Also include members representing the longest standard C99 type.
     * (these likely are aliases to other members already defined)
     * This is necessary if passing the value to a C library function, such as printf()
     */
    bool Boolean;
    EdsLib_Generic_SignedInt_t SignedInteger;
    EdsLib_Generic_UnsignedInt_t UnsignedInteger;
    EdsLib_Generic_FloatingPoint_t FloatingPoint;
} EdsLib_GenericValueUnion_t;

/**
 * Buffer type used for holding values of various types
 *
 * This is used in the external API to hold constraint values, to
 * provide a single type capable of holding several different types
 * of values for satisfying constraint entities.
 */
typedef struct
{
    /**
     * Indicates the type of data currently held in the "Value" element
     *
     * EDSLIB_BASICTYPE_SIGNED_INT indicates that the SignedInteger member is valid
     * EDSLIB_BASICTYPE_UNSIGNED_INT indicates that the UnsignedInteger member is valid
     * EDSLIB_BASICTYPE_IEEE_FLOAT indicates that the FloatingPoint member is valid
     * EDSLIB_BASICTYPE_BINARY indicates that the BinaryData and/or StringData member is valid
     *
     */
    EdsLib_BasicType_t ValueType;
    EdsLib_GenericValueUnion_t Value;
} EdsLib_GenericValueBuffer_t;

/**
 * Argument to the basic data dictionary walk callback function.
 * Indicates basic Type, Size and Offset information about a data element.
 */
struct EdsLib_DataTypeDB_TypeInfo
{
    EdsLib_BasicType_t ElemType;        /**< Type of element */
    uint16_t NumSubElements;            /**< Number of sub-elements below this (0 for atomic entities) */
    EdsLib_SizeInfo_t Size;             /**< Basic Size of object (base object only - not derived types) */
};

typedef struct EdsLib_DataTypeDB_TypeInfo EdsLib_DataTypeDB_TypeInfo_t;

struct EdsLib_DataTypeDB_DerivedTypeInfo
{
    uint16_t NumDerivatives;            /**< Number of defined derivative types for this type */
    uint16_t NumConstraints;            /**< Number of elements in the object that have constraints */
    EdsLib_SizeInfo_t MaxSize;          /**< Maximum Size of element (= size of largest derivative type) */
};

typedef struct EdsLib_DataTypeDB_DerivedTypeInfo EdsLib_DataTypeDB_DerivedTypeInfo_t;

struct EdsLib_DataTypeDB_DerivativeObjectInfo
{
    EdsLib_Id_t EdsId;
    uint16_t DerivativeTableIndex;      /**< Index into the list of derived types */
};

typedef struct EdsLib_DataTypeDB_DerivativeObjectInfo EdsLib_DataTypeDB_DerivativeObjectInfo_t;

/**
 * Structure to represent entities within EDS defined data types.
 *
 * This essentially just pairs up an EDS ID with an offset in bytes that indicates
 * the position of the entity within the parent.
 *
 * \note this only includes the byte offset for the native / unpacked representation.
 * This is intentional as it is not expected that user code will manipulate entities within
 * packed objects at the bit level.  User applications should unpack the object first and
 * then manipulate the native representation.
 */
struct EdsLib_DataTypeDB_EntityInfo
{
    EdsLib_Id_t EdsId;         /**< The EDS ID value representing the specific type of the data */
    EdsLib_SizeInfo_t Offset;  /**< Absolute Offset of member within the top-level parent structure */
    EdsLib_SizeInfo_t MaxSize; /**< Total allocated space for this object within parent structure */
};

typedef struct EdsLib_DataTypeDB_EntityInfo EdsLib_DataTypeDB_EntityInfo_t;

/**
 * The callback function assocated with EdsLib_DataTypeDB_ConstraintIterator()
 *
 * @param GD the runtime database object
 * @param MemberInfo the type and offset location of the constrained field
 * @param ConstraintValue the value that should be in the field
 * @param Arg opaque user argument
 */
typedef void (*EdsLib_ConstraintCallback_t)(const EdsLib_DatabaseObject_t *GD,
        const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
        EdsLib_GenericValueBuffer_t *ConstraintValue,
        void *Arg);



/******************************
 * API CALLS
 ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initialize the EdsLib DataTypeDB internal state
 *
 * This should be called once during initialization by
 * programs wishing to use the the EDS DataTypeDB API calls.
 *
 * This function initializes all shared/global state information
 * internal to EdsLib.  In particular this will generate lookup
 * tables that may be necessary for error control algorithms to
 * work correctly (e.g. CRC).
 */
void EdsLib_DataTypeDB_Initialize(void);

/**
 * Extract the AppIdx value from a dictionary structure
 *
 * The "DataTypeDB_t" type is abstract, so application code can only treat it as an opaque
 * object pointer and cannot directly access the fields inside.
 *
 * This function queries the "AppIdx" value from inside the dictionary object
 *
 * @param AppDict the application dictionary object
 * @return The associated index
 */
uint16_t EdsLib_DataTypeDB_GetAppIdx(const EdsLib_DataTypeDB_t AppDict);

/**
 * Register an application in a dynamic runtime EdsLib runtime database.
 *
 * NOTE: Multiple registrations of the same application have no effect, as only a single entry
 * ever exists for an individual application.
 *
 * @param GD the active EdsLib runtime database object
 * @param AppDict the application dictionary object
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 */
int32_t EdsLib_DataTypeDB_Register(EdsLib_DatabaseObject_t *GD, EdsLib_DataTypeDB_t AppDict);

/**
 * Unregister an application in a dynamic runtime EdsLib runtime database.
 *
 * @param GD the active EdsLib runtime database object
 * @param AppIdx Application identifier as returned by EdsLib_DataTypeDB_Register()
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 */
int32_t EdsLib_DataTypeDB_Unregister(EdsLib_DatabaseObject_t *GD, uint16_t AppIdx);

/**
 * Given any global message identifier index, retrieve the basic details of that data type
 * The details are in reference to the local (native) representation not necessarily the packed representation
 *
 * @param GD the runtime database object
 * @param EdsId the message identifier word
 * @param TypeInfo Buffer for storing the member info
 * @return EDS_SUCCESS if successful
 */
int32_t EdsLib_DataTypeDB_GetTypeInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_DataTypeDB_TypeInfo_t *TypeInfo);

/**
 * Given a type with sub-members (container, array, interface, etc), look up the identification and offset for
 * a given child index within that parent type
 *
 * SubMsgId will be set to an invalid identifier if the index is out of range, or the parent does not have any members
 *
 * @param GD the runtime database object
 * @param EdsId the parent message identifier word
 * @param SubIndex the sub-element number within the parent
 * @param MemberInfo Buffer to store the Component information
 * @return EDS_SUCCESS if successful
 */
int32_t EdsLib_DataTypeDB_GetMemberByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t SubIndex, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo);

/**
 * Given a type with sub-members (container, array, interface, etc), look up the identification and offset for
 * the immediate sub-member containing the item at the given offset.  This only locates the _immediate_ subelement
 * containing the offset, which could be another container.  This function may have to be called multiple times
 * to locate a single item.
 *
 * @param GD the runtime database object
 * @param EdsId the parent message identifier word
 * @param ByteOffset the sub-element number within the parent
 * @param MemberInfo Buffer to store the Component information
 * @return EDS_SUCCESS if successful
 */
int32_t EdsLib_DataTypeDB_GetMemberByNativeOffset(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint32_t ByteOffset, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo);

/**
 * Look up the derived type of the message identified by EdsId,
 * given a constraint entity and constraint value.
 *
 * @param GD the runtime database object
 * @param BaseId the message identifier word
 * @param ConstraintEntityIdx The entity index that the ConstraintValue refers to
 * @param ConstraintValue the actual constraint value to look up
 * @return The derived interface ID, or an invalid ID if an error occurs.
 */
int32_t EdsLib_DataTypeDB_GetDerivedTypeById(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t DerivId, EdsLib_Id_t *DerivedEdsId);

/**
 * Look up the constraint entity corresponding to the given index
 *
 * @param GD the runtime database object
 * @param EdsId the message identifier word
 * @param ConstraintEntityIdx The constraint entity number
 * @param MemberInfo Buffer to store result (output)
 * @return EDS_SUCCESS if successful
 */
int32_t EdsLib_DataTypeDB_GetConstraintEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t ConstraintIdx, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo);

/**
 * Look up the details regarding types that are derived from the given type
 *
 * These details include:
 *  - the maximum size of the largest derivative (for e.g. allocating a buffer)
 *  - the number of member entities in the base type with constraints
 *
 * @param GD the runtime database object
 * @param EdsId the interface identifier word
 * @param DerivInfo buffer to hold the derivation information
 * @return EDS_SUCCESS if successful
 */
int32_t EdsLib_DataTypeDB_GetDerivedInfo(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_DataTypeDB_DerivedTypeInfo_t *DerivInfo);

/**
 * Find all required constraint value(s) to produce a specified derived message
 * from the given base type.
 *
 * As there may be an arbitrary number of constraints on fields on the base type, this
 * is implemented as an iterator with a user-supplied callback.  The user callback will
 * be invoked for each constraint, indicating the required value and its location/offset
 * within the container.
 *
 * @note
 * If the indicated base type is not actually a base type for the indicated derived type,
 * the function will return an ERROR response.  However, the user callback may be invoked
 * before the error is detected, so the caller is responsible for discarding any constraints
 * collected prior to the error.
 *
 * @param GD the runtime database object
 * @param BaseId the message identifier word of the base type
 * @param DerivedId the message identifier word of the derived/final type
 * @param Callback user-supplied function to invoke for each constraint
 * @param CbArg user-supplied argument to pass to the callback
 * @return EDS_SUCCESS if successful
 */
int32_t EdsLib_DataTypeDB_ConstraintIterator(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t DerivedId, EdsLib_ConstraintCallback_t Callback, void *CbArg);

/**
 * Determine if a base type relationship exists between the two types
 *
 * Returns a successful response if the passed-in base ID is a base type for
 * the passed-in derived type ID.
 *
 * @param GD the runtime database object
 * @param BaseId the message identifier word of the base type
 * @param DerivedId the message identifier word of the derived/final type
 * @return EDS_SUCCESS if successful
 */
int32_t EdsLib_DataTypeDB_BaseCheck(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t DerivedId);


/**
 * Perform conversion from a native/unpacked object to an EDS/packed bitstream
 *
 * This packs/encodes a single object type in one step.  The object will be encoded
 * according to the object type supplied via the EdsId parameter.  After encoding
 * the database will be checked for derived types matching the constraints, and
 * if a matching derived type is found, encoding continues for the derived type.
 * This process repeats until no further derivation is found.
 *
 * After encoding, all special fields in the object will be updated to the correct
 * values.  Fixed value fields will be set, and Error control fields and Length fields
 * will be computed and set to the correct values per EDS specifications.
 *
 * @param GD the runtime database object
 * @param EdsId Buffer containing identifier for the top level encapsulation.
 *      After encoding, this is set to the final encoded object type.
 * @param DestBuffer Pointer to the destination buffer
 * @param SourceBuffer Pointer to the source buffer (not modified by this call)
 * @param MaxPackedBitSize Maximum size of destination buffer, in bits
 * @param SourceByteSize Maximum size of source buffer, in bytes
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 *
 * \sa EdsLib_DataTypeDB_PackPartialObject()
 */
int32_t EdsLib_DataTypeDB_PackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize);

/**
 * Perform conversion from a native/unpacked object to an EDS/packed bitstream in stages
 *
 * This packs the output buffer in stages.  The object will be encoded similar to the
 * EdsLib_DataTypeDB_PackCompleteObject(), with respect to derived object identification
 * and encoding.  However, the object is not completed in that the values for special fields
 * (i.e. ErrorControl, Length, Fixed values) are not computed or set in the encoded object.
 *
 * This function should be used by applications that need to further process the content
 * or otherwise customize the procedure during the encoding process.
 *
 * To facilitate this, an additional parameter "StartingBit" is included, which allows
 * the application to specify the starting bit position for encoding.  This allows
 * encoding to be resumed from a previous call.  Bits prior to this starting bit will
 * not be re-encoded.  On the first invocation, the StartingBit should be set 0.  On
 * subsequent invocations, the StartingBit should be set to the end of the object
 * (in bits) from the previous invocation.
 *
 * The EdsLib_DataTypeDB_FinalizePackedObject() API call can be used at a later time
 * to compute the values for special fields within the encoded binary.
 *
 * @param GD the runtime database object
 * @param EdsId The identifier for the top level encapsulation
 * @param DestBuffer Pointer to the destination buffer
 * @param SourceBuffer Pointer to the source buffer (not modified by this call)
 * @param MaxPackedBitSize Maximum size of destination buffer, in bits
 * @param SourceByteSize Maximum size of source buffer, in bytes
 * @param StartingBit Number of bits that are already encoded (i.e. from previous call)
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 *
 * \sa EdsLib_DataTypeDB_PackCompleteObject()
 * \sa EdsLib_DataTypeDB_FinalizePackedObject()
 */
int32_t EdsLib_DataTypeDB_PackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize, uint32_t StartingBit);

/**
 * Perform conversion from an EDS/packed bitstream to a native/unpacked object
 *
 * This unpacks/decodes a single object type in one step.  The object will be decoded
 * according to the object type supplied via the EdsId parameter.  After decoding
 * the database will be checked for derived types matching the constraints, and
 * if a matching derived type is found, decoding continues for the derived type.
 * This process repeats until no further derivation is found.
 *
 * After decoding completes, any special fields within the object are verified
 * against the EDS specifications.  Length entries, FixedValue entries, and ErrorControl
 * entries are checked against locally computed according to EDS specifications and
 * values within the decoded object are compared against the locally computed value.
 * Any mismatch will generate an error.
 *
 * @param GD the runtime database object
 * @param EdsId The identifier for the packed object.
 *      After decoding, this is set to the final decoded object type.
 * @param DestBuffer Pointer to the destination buffer
 * @param SourceBuffer Pointer to the source buffer (not modified by this call)
 * @param MaxNativeByteSize Maximum size of destination buffer, in bytes
 * @param SourceBitSize Maximum size of source buffer, in bits
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 *
 * \sa EdsLib_DataTypeDB_UnpackPartialObject()
 */
int32_t EdsLib_DataTypeDB_UnpackCompleteObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize);

/**
 * Perform conversion from an EDS/packed bitstream to a native/unpacked object in stages.
 *
 * This unpacks the output buffer in stages.  The object will be decoded similar to the
 * EdsLib_DataTypeDB_UnpackCompleteObject(), with respect to derived object identification
 * and decoding.  However, the object is not verified in that the values for special fields
 * (i.e. ErrorControl, Length, Fixed values) are passed through and not checked.
 *
 * This function should be used by applications that need to further process the content
 * or otherwise customize the procedure during the decoding process.
 *
 * To facilitate this, an additional parameter "StartingByte" is included, which allows
 * the application to specify the starting byte position for decoding.  This allows
 * decoding to be resumed from a previous call.  Bytes prior to this starting byte will
 * not be re-decoded.  On the first invocation, the StartingByte should be set 0.  On
 * subsequent invocations, the StartingByte should be set to the end of the object
 * (in bytes) from the previous invocation.
 *
 * The EdsLib_DataTypeDB_VerifyUnpackedObject() API call can be used at a later time
 * to check the values for special fields within the decoded structure.
 *
 * @param GD the runtime database object
 * @param EdsId The identifier for the top level encapsulation
 * @param DestBuffer Pointer to the destination buffer
 * @param SourceBuffer Pointer to the source buffer (not modified by this call)
 * @param MaxNativeByteSize Maximum size of destination buffer, in bytes
 * @param SourceBitSize Maximum size of source buffer, in bits
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 *
 * \sa EdsLib_DataTypeDB_UnpackCompleteObject()
 * \sa EdsLib_DataTypeDB_VerifyUnpackedObject()
 */
int32_t EdsLib_DataTypeDB_UnpackPartialObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
        void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize, uint32_t StartingByte);

/**
 * Compute values for special fields within a packed object.
 *
 * Special fields such as Error Control, Length Entries, and Fixed Values need
 * to be computed after encoding is complete.  These cannot be computed
 * on-the-fly during encoding because they require the complete object to known.
 * For instance, a Length would reflect the complete length of the packet, and
 * an Error Control such as CRC-16 requires the Length to be set correctly prior
 * to calculation.  Due to these interdependencies, the computation needs to be
 * deferred until the entire content is known.
 *
 * Note that the EdsLib_DataTypeDB_PackCompleteObject() API invokes this
 * function automatically.  This only needs to be invoked by the application
 * when using the EdsLib_DataTypeDB_PackPartialObject() API.
 *
 * @param GD the runtime database object
 * @param EdsId The identifier for the top level encapsulation
 * @param PackedData Pointer to the encoded/packed buffer
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 *
 * \sa EdsLib_DataTypeDB_PackPartialObject()
 */
int32_t EdsLib_DataTypeDB_FinalizePackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *PackedData);

/**
 * Constants for EdsLib_DataTypeDB_VerifyUnpackedObject() "RecomputeFields" parameter.
 *
 * These are bitmasks that can be OR'ed together to indicate which aspects to recompute
 * during the verification of a decoded object.
 */
enum
{
    EDSLIB_DATATYPEDB_RECOMPUTE_NONE         = 0,    /**< Do not recompute any fields (default) */
    EDSLIB_DATATYPEDB_RECOMPUTE_LENGTH       = 0x01, /**< Recompute Length field of the native object */
    EDSLIB_DATATYPEDB_RECOMPUTE_ERRORCONTROL = 0x02, /**< Recompute Error Control field of the native object */
    EDSLIB_DATATYPEDB_RECOMPUTE_FIXEDVALUE   = 0x04, /**< Recompute Fixed Value fields of the native object */
    EDSLIB_DATATYPEDB_RECOMPUTE_ALL          = 0xFF  /**< Recompute all relevant fields */
};

/**
 * Verify values for special fields within an unpacked (native) object.
 *
 * Special fields such as Error Control, Length Entries, and Fixed Values need
 * to be verified after decoding is complete.  These cannot be computed
 * on-the-fly during encoding because they require the complete object to known.
 * For instance, a Length would reflect the complete length of the packet, and
 * an Error Control such as CRC-16 requires the entire content to be known
 * prior to verification.  Due to these interdependencies, the computation needs
 * to be deferred until the entire content is known.
 *
 * Note that the EdsLib_DataTypeDB_UnpackCompleteObject() API invokes this
 * function automatically.  This only needs to be invoked by the application
 * when using the EdsLib_DataTypeDB_UnpackPartialObject() API.
 *
 * Note that in order to calculate an error control field such as a CRC, this
 * function needs a pointer to the raw (encoded) data in addition to the decoded
 * data.  If this is set NULL, then the field(s) cannot be checked.
 *
 * In addition to verification of the original values from the encoded object,
 * this function can also update the values to match the decoded object if desired.
 * For instance, some historical code might assume that an embedded LengthEntry
 * field will be related to the "sizeof()" operator in C.  However, the size of
 * an EDS object will not necessarily match the size of the corresponding C structure
 * due to native representation of values and alignment padding.  To appease
 * older applications, the value of the Length field can be adjusted to match
 * the native structure instead of encoded object.
 *
 *
 * @param GD the runtime database object
 * @param EdsId The identifier for the top level encapsulation
 * @param UnpackedObj Pointer to the decoded/unpacked buffer which is to be checked.
 * @param PackedData Pointer to the encoded/packed buffer, for reference when verifying
 *          fields.  May be set NULL to skip verification (i.e. if recomputing fields).
 * @param RecomputeFields A bitmask indicating the fields to recompute.  Should be
 *          a bitwise OR of the flags defined in the respective enumeration.
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 *
 * \sa EDSLIB_DATATYPEDB_RECOMPUTE_NONE
 * \sa EdsLib_DataTypeDB_UnpackPartialObject()
 */
int32_t EdsLib_DataTypeDB_VerifyUnpackedObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
        void *UnpackedObj, const void *PackedObj, uint32_t RecomputeFields);

/**
 * Initialize fixed values within a newly-created unpacked (native) object.
 *
 * On derived containers, the EDS specifies Value constraints on fields within
 * the base container that are subsequently used for identification.  This
 * function initializes those fields which are specified in the EDS for the
 * given container type.
 *
 * This function should be called whenever a new object is instantiated in
 * memory.
 *
 * @param GD the runtime database object
 * @param EdsId The identifier for the top level encapsulation
 * @param UnpackedObj Pointer to the decoded/unpacked buffer which is to be initialized.
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 */
int32_t EdsLib_DataTypeDB_InitializeNativeObject(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *UnpackedObj);

/**
 * Given an unpacked/native buffer, extract a value using EDS-specified semantics.
 *
 * @param GD the runtime database object
 * @param SourceBuffer The message buffer (read-only, not modified by this call)
 * @param ComponentInfo The ID of the message and field (offset) within the message to extract
 * @param DestBuffer Buffer to store result information.
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 */
int32_t EdsLib_DataTypeDB_LoadValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_GenericValueBuffer_t *DestBuffer, const void *SrcPtr);

/**
 * Given an unpacked (native byte order) message buffer, store a value using EDS-specified semantics.
 *
 * @param GD the runtime database object
 * @param DestBuffer The message buffer (modified by this call)
 * @param ComponentInfo The ID of the message and field (offset) within the message to write
 * @param SourceValue Value to store in the message.
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 */
int32_t EdsLib_DataTypeDB_StoreValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr, EdsLib_GenericValueBuffer_t *SrcBuffer);

/**
 * Given an unpacked (native byte order) container, identify the derived contents using EDS-specified constraint values
 *
 * Reads the necessary field(s) from the encapsulation interface to determine the derived contents
 *
 * @param GD the runtime database object
 * @param EdsId The ID of the encapsulation interface
 * @param MessageBuffer The message buffer (read-only, not modified by this call)
 * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
 */
int32_t EdsLib_DataTypeDB_IdentifyBuffer(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const void *MessageBuffer, EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo);

/**
 * Convert the numeric value representation from its current type into the desired type
 *
 * If the current type and desired type is equal, then no change is made.  Otherwise, the value may be converted
 * as necessary to acheive the desired type.  The value buffer is modified in place.
 *
 * @param ValueBuff An EdsLib value buffer object containing a numeric value
 * @param DesiredType The desired type of value
 */
void EdsLib_DataTypeConvert(EdsLib_GenericValueBuffer_t *ValueBuff, EdsLib_BasicType_t DesiredType);


#ifdef __cplusplus
}   /* extern C */
#endif


#endif  /* _EDSLIB_DATATYPEDB_H_ */

