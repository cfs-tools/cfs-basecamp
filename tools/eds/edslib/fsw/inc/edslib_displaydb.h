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
 * \file     edslib_displaydb.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 *
 * Full data lookup functions
 *
 * The API calls in this file require lookups from the EdsLib data dictionary AND name
 * dictionary structures to obtain the information.  They require a full set of dictionaries
 * to be loaded in order to operate.
 *
 * This extends the "basic" data-only API defined in edslib_datatypedb.h
 */

#ifndef _EDSLIB_DISPLAYDB_H_
#define _EDSLIB_DISPLAYDB_H_


#include "edslib_datatypedb.h"


/******************************
 * TYPEDEFS
 ******************************/

/**
 * Callback parameter for Name/Display information related to a dictionary walk callback
 * This extends the basic information supplied via EdsLib_Iterator_MemberInfo_t
 *
 * \sa EdsLib_Iterator_MemberInfo_t
 */
struct EdsLib_EntityDescriptor
{
    EdsLib_DataTypeDB_EntityInfo_t EntityInfo; /**< Information about the component itself (type ID, offset within parent) */
    const char *FullName;                  /**< Name of current element as obtained from name dictionary */
    uint16_t SeqNum;
};

typedef struct EdsLib_EntityDescriptor EdsLib_EntityDescriptor_t;

/**
 * Simplified Callback function prototype
 *
 * Used by functions that look up information in the dictionary of structures.
 * This is different than the normal callback in two ways:
 *    - It is only invoked for "MEMBER" types (EDSLIB_ITERATOR_CBTYPE_MEMBER)
 *    - It does not have a return code / EDSLIB_ITERATOR_RC_CONTINUE is assumed.
 *
 * Certain high-level API calls will use this version of the callback function to keep the use case as
 * straightforward as possible.
 */
typedef void (*EdsLib_EntityCallback_t)(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc);

/**
 * Callback function prototype used by functions that look up information
 * from the compile-time the symbol table
 */
typedef void (*EdsLib_SymbolCallback_t)(void *Arg, const char *SymbolName, int32_t SymbolValue);


/******************************
 * API CALLS
 ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initialize the DisplayDB subsystem
 *
 * This should be called once during initialization by
 * programs wishing to use the the Display API calls.
 */
void EdsLib_DisplayDB_Initialize(void);

/**
 * Gets the printable name of the EDS associated with the given Message ID.
 *
 * \note
 * This function returns the constant string "UNDEF" instead of NULL if the lookup fails,
 * so it is safe to use in directly in printf()-style calls.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the message ID to look up
 * @returns a constant character string representing the application name
 */
const char *EdsLib_DisplayDB_GetEdsName(const EdsLib_DatabaseObject_t *GD, uint16_t AppId);

/**
 * Gets the base printable message/structure name associated with the given Message ID.
 *
 * This does NOT include any namespace qualifier.  It returns only the base name of the object.
 *
 * \note
 * This function returns the constant string "UNDEF" instead of NULL if the lookup fails,
 * so it is safe to use in directly in printf()-style calls.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the message ID to look up
 * @returns a constant character string representing the message/structure name
 */
const char *EdsLib_DisplayDB_GetBaseName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId);


/**
 * Gets the base printable namespace name associated with the given Message ID.
 *
 * This is usually (by convention) the same as the EDS name but that is not required
 *
 * \note
 * This function returns the constant string "UNDEF" instead of NULL if the lookup fails,
 * so it is safe to use in directly in printf()-style calls.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the message ID to look up
 * @returns a constant character string representing the namespace name
 */
const char *EdsLib_DisplayDB_GetNamespace(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId);

/**
 * Gets the fully qualified message/structure name associated with the given Message ID.
 *
 * This includes a namespace qualifier i.e. NAME_SPACE/name format.  This name is created by
 * concatenating the two so a storage buffer must be supplied by the user.  The same buffer is
 * also returned by the function to ease use inside printf-style arguments.
 *
 * \note
 * This function returns the constant string "UNDEF" instead of NULL if the lookup fails,
 * so it is safe to use in directly in printf()-style calls.  However in this case the user
 * buffer will be set to an empty string.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the message ID to look up
 * @param Buffer memory to store result string
 * @param BufferSize size of Buffer
 * @returns the user buffer that was passed in.
 */
const char *EdsLib_DisplayDB_GetTypeName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *Buffer, uint32_t BufferSize);

/**
 * Gets details on how to display a given data type, if available.
 *
 * Returns a "DisplayHint" and associated argument if present in the database.
 *  - For EDSLIB_DISPLAYHINT_ENUM_SYMTABLE, the argument is the symbolic name/value table
 *  - For EDSLIB_DISPLAYHINT_SUBNAMES, the argument is a const char array of the names
 *
 * Other types/usage of the pointer argument may be added.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the message ID to look up
 * @return Display Hint type if successful
 */
EdsLib_DisplayHint_t EdsLib_DisplayDB_GetDisplayHint(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId);


/**
 * Parse a string and convert into an EDS identifier value
 *
 * @param GD the active EdsLib runtime database object
 * @param String The null-terminated string to parse as an EDS identifier
 * @returns The converted EdsLib_Id_t value
 *
 * \sa EdsLib_EdsId_ToString()
 */
EdsLib_Id_t EdsLib_DisplayDB_LookupTypeName(const EdsLib_DatabaseObject_t *GD, const char *String);


/**
 * Converts a named member entity into an integer index if possible
 *
 * Attempts to find a member identified by the Name parameter within the specified EDS object.
 * If found, the index/position of that name is stored in the SubIndex output buffer, which can
 * then be passed to the EdsLib_DataTypeDB_GetMemberByIndex() function to obtain further
 * information about the entity.
 *
 * This function only works on containers and array types that have sub entities; it does not
 * work on scalar types.
 *
 * @note This function is _not_ a substitute for a structure name lookup as done by the
 * locate function.  In particular this API only operates at the given direct data type level
 * and it will not search inside any sub-objects to find a matching name.  In particular
 * this function will not find anything that is not directly named within the given data type,
 * such as a base type objects which are "hidden".
 *
 * Other types/usage of the pointer argument may be added.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the ID of the data type to operate on
 * @param Name the subentity name to find
 * @param SubIndex buffer to store the index/position
 * @return EDSLIB_SUCCESS if successful, or appropriate error code if unsuccessful
 */
int32_t EdsLib_DisplayDB_GetIndexByName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name, uint16_t *SubIndex);

/**
 * Converts an integer index into a member name if possible
 *
 * Looks up the display name of a sub entity at the given index.  If there is no sub member
 * at the given index or it has no name associated with it in the database, then NULL is
 * returned.
 *
 * This function only works on containers and array types that have sub entities; it does not
 * work on scalar types.  NULL will be returned in all those cases.
 *
 * In particular, this method can be used to check if an EDS index refers to a "base type"
 * of a container object.  These are valid container indices that have no name associated,
 * and thus this function will return NULL.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the ID of the data type to operate on
 * @param SubIndex the index/position to look up
 * @return pointer to member name if successful, NULL if no name exists
 */
const char *EdsLib_DisplayDB_GetNameByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t SubIndex);

/**
 * Walk through the payload structure of a given message/structure ID and generate a callback for each
 * element within the structure with the detailed parameter information.
 *
 * This "ShortNames" variant will NOT descend into sub-structures within the parent structure given.  The "FullName"
 * in the parameter descriptor will always be just a single (short) element name.  An element that is a sub structure will have
 * the "SubStructureId" value set to a nonzero value and array elements will only get a single callback for the first
 * element of the array.
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the message ID to walk.  This may be any known/valid ID (message ID or a bare structure ID).
 * @param Callback the callback function to invoke for each element in the message payload
 * @param Arg opaque argument that is passed directly to the callback function
 *
 * \sa EdsLib_Iterator_Payload_FullNames()
 */
void EdsLib_DisplayDB_IterateBaseEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_EntityCallback_t Callback, void *Arg);
void EdsLib_DisplayDB_IterateAllEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_EntityCallback_t Callback, void *Arg);
/**
 * Walk through the deep payload structure of a given message/structure ID and find an element name matching
 * the name supplied as the "FullName" value within the LocateDesc parameter.  If found, the rest of the parameters
 * in LocateDesc will be set according to the found element.
 *
 * This version will descend into all sub structures and name matching is performed on the fully qualified name which should
 * be in C-style syntax.  For instance:
 *
 *    Member.SubMember.SubArray[ArrayIndex].Value
 *
 * @param GD the active EdsLib runtime database object
 * @param EdsId the message ID to walk.  This may be any known/valid ID (message ID or a bare structure ID).
 * @param Name The parameter to locate.
 * @param CompInfo Output buffer representing member location.
 * @returns EDSLIB_SUCCESS if successful, error code if element name was not found.
 */
int32_t EdsLib_DisplayDB_LocateSubEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name, EdsLib_DataTypeDB_EntityInfo_t *CompInfo);


/**
 * Debugging utility function to write arbitrary data contents to the stdio stream as hexadecimal
 * The data is treated as a binary blob.
 *
 * @param output any valid STDIO output stream (e.g. stdout)
 * @param DataPtr pointer to the block of data
 * @param DisplayOffset the offset of the first byte in the data block.  This is typically zero but may be nonzero if the data
 *        should be viewed as a continuation of a previous display.
 * @param Count The number of bytes to display.
 */
void EdsLib_Generate_Hexdump(void *output, const uint8_t *DataPtr, uint16_t DisplayOffset, uint16_t Count);


/* **************************************************************************************
 * EDS BINARY <-> STRING CONVERTERS
 *
 * Thes functions convert between user-friendly string representations and the numeric
 * representations of EDS identifiers and EDS-defined datatypes.
 * **************************************************************************************/


/**
 * Convert a binary/machine value into a printable string using the EdsLib_DisplayHint_t as guidelines for how to interpret the data.
 *  - For character strings, printable chars (as indicated by isprint()) will copied to the output buffer and null termination is ensured.
 *  - For integer numbers, these will be converted to symbolic constants if the "EnumNamespace" is set and a matching value is found.
 *  - For numbers for which a symbolic constant is not defined, conversion will fall back to an appropriate sprintf-like behavior
 *
 * EdsLib_Scalar_FromString() will perform the reverse operation
 *
 * @param GD the active EdsLib runtime database object
 * @param OutputBuffer location to store the output string
 * @param BufferSize maximum size of the OutputBuffer
 * @param SrcPtr pointer to binary/machine data.  Typically this is a pointer to a byte offset inside an "unpacked" message or structure,
 *          retrieved via the "MemberAbsOffset" field as returned from a locate or walk routine.
 * @returns EDSLIB_SUCCESS if successful, error code if conversion failed.
 *
 * \sa EdsLib_Scalar_FromString()
 */
int32_t EdsLib_Scalar_ToString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *OutputBuffer, uint32_t BufferSize, const void *SrcPtr);


/**
 * Convert string into binary/machine data using the EdsLib_DisplayHint_t as guidelines for how to interpret the data.
 *  - For character strings, all chars will copied to the destination up to the size limit or the terminating NULL
 *  - For integer numbers, these may be symbolic constants if the "EnumNamespace" is set and a matching value is found.
 *  - For numbers for which a symbolic constant is not defined, conversion will fall back to an appropriate strtol/ul/d/f system call
 *
 * EdsLib_Scalar_ToString() will perform the reverse operation
 *
 * @param GD the active EdsLib runtime database object
 * @param DestPtr pointer to binary/machine data.  Typically this is a pointer to a byte offset inside a message or structure, retrieved
 *          via the "MemberAbsOffset" field as returned from a locate or walk routine.
 * @param SrcString The null-terminated string to convert into machine data
 * @returns EDSLIB_SUCCESS if successful, error code if conversion failed.
 *
 * \sa EdsLib_Scalar_ToString()
 */
int32_t EdsLib_Scalar_FromString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr, const char *SrcString);

const char *EdsLib_DisplayDB_GetEnumLabel(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const EdsLib_GenericValueBuffer_t *ValueBuffer);
void EdsLib_DisplayDB_GetEnumValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *String, EdsLib_GenericValueBuffer_t *ValueBuffer);
void EdsLib_DisplayDB_IterateEnumValues(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_SymbolCallback_t Callback, void *Arg);

intmax_t EdsLib_DisplayDB_GetEnumValueByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t Index);
const char *EdsLib_DisplayDB_GetEnumLabelByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t Index, char *Buffer, uint32_t BufferSize);

/**
 * Base64 encoding helper function
 *
 * Encodes the input using base64, where each character in the
 * output represents 6 bits of binary data.  The resulting
 * string contains only normal printable characters, and can be
 * passed through terminals or any other transmission media that is
 * not 8-bit clean.  Added whitespace, newlines, etc are not an issue.
 *
 * Input data will be padded with zero bits as necessary to make
 * the binary data size an exact multiple of 6 bits.
 *
 * Output data will be truncated at the last non-zero character.
 *
 * The original binary data can be reconstituted using the decode function.
 *
 * @note The Out buffer should be sized appropriately to store
 *      exactly 1 char per 6 bits of input, plus 1 extra char
 *      for null termination of the output string.
 */
void EdsLib_DisplayDB_Base64Encode(char *Out, uint32_t OutputLenBytes, const uint8_t *In, uint32_t InputLenBits);

/**
 * Base64 decoding helper function
 *
 * Decodes the input using base64, where each character in the
 * input represents 6 bits of binary data.  This function will stop reading the
 * input stream when the first null character is encountered.
 *
 * In all cases the output will be zero-filled with bits to as necessary fill the field
 * width as specified in the OutputLenBits parameter.
 */
void EdsLib_DisplayDB_Base64Decode(uint8_t *Out, uint32_t OutputLenBits, const char *In);

#ifdef __cplusplus
}   /* extern C */
#endif



#endif  /* _EDSLIB_DISPLAYDB_H_ */

