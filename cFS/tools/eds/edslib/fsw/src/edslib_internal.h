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
 * \file     edslib_internal.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Data types and function prototypes that are private to the EDS runtime library.
 * These should not be exposed to the public API, and should not be used outside
 * of this library.
 */

#ifndef _EDSLIB_INTERNAL_H_
#define _EDSLIB_INTERNAL_H_

#include "edslib_database_types.h"
#include "edslib_database_ops.h"
#include "edslib_global.h"
#include "edslib_datatypedb.h"
#include "edslib_displaydb.h"
#include "edslib_intfdb.h"
#include "edslib_binding_objects.h"

/******************************
 * MACROS
 ******************************/

/**
 * The absolute maximum depth that the datatype iterator
 * will descend into substructures when iterating using the "deep"
 * method.
 *
 * This reflects the maximum level of datatype nesting/containment that
 * the runtime library will support.  This also serves as a functional
 * limit in case an (invalid) circular reference somehow exists in the DB.
 */
#define EDSLIB_ITERATOR_MAX_DEEP_DEPTH 32

/**
 * The limit for how deep a basetype hierarchy chain can be.  This is an
 * intermediate limit that exists solely so that functions that are iterating
 * only "immediate" members can still descend into basetypes, but not need
 * to allocate a state structure as large as EDSLIB_ITERATOR_MAX_DEEP_DEPTH
 */
#define EDSLIB_ITERATOR_MAX_BASETYPE_DEPTH 8

/**
 * The limit for depth when only iterating through the immediate type members.
 * This is the smallest and most stack friendly depth but it does not support
 * descending into sub-structures at all, including base types.
 */
#define EDSLIB_ITERATOR_MAX_SHALLOW_DEPTH 2

/**
 * An internal macro used to combine a type and size from the database into
 * a single value that can be used in a switch/case statement.
 */
#define EDSLIB_TYPE_AND_SIZE(x, y) (((x) << 8) | (y))

/**
 * Some fields such as error control cannot be calculated immediately, they
 * must be calculated only after all other encoding is done.  This reflects
 * the maximum number of fields that can have deferred calculation in a single
 * encode/decode operation.
 */
#define EDSLIB_PACKEDPOSTPROC_MAX_DEFERRED_FIELDS 8

/******************************
 * TYPEDEFS
 ******************************/
/**
 * A pointer that can access various width numeric data, as well as
 * be interpreted as a raw memory address.
 *
 * This is the constant version for reading.
 */
typedef union
{
    const void                       *Ptr;
    const EdsLib_GenericValueUnion_t *u;
    const uint8_t                    *Addr;
} EdsLib_ConstPtr_t;

/**
 * A pointer that can access various width numeric data, as well as
 * be interpreted as a raw memory address.
 *
 * This is the non-constant version for writing.
 */
typedef union
{
    void                       *Ptr;
    EdsLib_GenericValueUnion_t *u;
    uint8_t                    *Addr;
} EdsLib_Ptr_t;

/**
 * Argument to the basic iterator callback function.
 * Indicates the type of callback taking place.
 */
typedef enum
{
    EDSLIB_ITERATOR_CBTYPE_UNDEFINED = 0,
    EDSLIB_ITERATOR_CBTYPE_START,
    EDSLIB_ITERATOR_CBTYPE_MEMBER,
    EDSLIB_ITERATOR_CBTYPE_END
} EdsLib_Iterator_CbType_t;

/**
 * Return value from the basic data dictionary walk callback function.
 * Indicates the next step to take.
 */
typedef enum
{
    EDSLIB_ITERATOR_RC_DEFAULT = 0, /**< Unspecified action, usable as a placeholder before setting the value */
    EDSLIB_ITERATOR_RC_CONTINUE,    /**< Continue to the next element at the same depth */
    EDSLIB_ITERATOR_RC_DESCEND,     /**< Descend into the current element (must be array or structure) */
    EDSLIB_ITERATOR_RC_ASCEND,      /**< Ascend up to the parent element */
    EDSLIB_ITERATOR_RC_STOP         /**< Stop walking the dictionary and return to the caller */
} EdsLib_Iterator_Rc_t;

typedef struct
{
    uint16_t                         CurrIndex;
    uint16_t                         CurrDepth;
    EdsLib_SizeInfo_t                StartOffset;
    EdsLib_SizeInfo_t                EndOffset;
    EdsLib_FieldDetailEntry_t        Details;
    const EdsLib_DataTypeDB_Entry_t *DataDictPtr;
} EdsLib_DataTypeIterator_StackEntry_t;

/**
 * Prototype for the basic dictionary iterator callback functions.
 */
typedef EdsLib_Iterator_Rc_t (*EdsLib_DataTypeIterator_Callback_t)(
    const EdsLib_DatabaseObject_t              *GD,
    EdsLib_Iterator_CbType_t                    CbType,
    const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
    void                                       *OpaqueArg);

typedef struct
{
    EdsLib_DataTypeIterator_Callback_t Callback;
    void                              *CallbackArg;
    uint16_t                           StackSize;

    /*
     * The "stack" is variably sized and must contain at least one item
     * for the base object.
     *
     * This structure must be allocated with enough memory beyond the end
     * of this structure for the complete stack.  This is generally accomplished
     * by wrapping this control block into a larger structure containing the
     * required number of stack entries.
     */
    EdsLib_DataTypeIterator_StackEntry_t StackBase[1];
} EdsLib_DataTypeIterator_ControlBlock_t;

/**
 * Macro to instantiate an iterator control block for the depth specified.
 *
 * This wraps the control block structure with enough spare memory for
 * a stack to hold the state at each iteration level up to the specified maximum.
 */
#define EDSLIB_ITERATOR_CB_TYPE(depth)                                  \
    struct                                                              \
    {                                                                   \
        EdsLib_DataTypeIterator_ControlBlock_t Cb;                      \
        EdsLib_DataTypeIterator_StackEntry_t   StackEntries[depth - 1]; \
    }

/**
 * Macro to initialize an iterator control block for the depth specified.
 *
 * This wraps the control block structure with enough spare memory for
 * a stack to hold the state at each iteration level up to the specified maximum.
 */
#define EDSLIB_INITIALIZE_ITERATOR_CB(name, depth, cbfunc, cbarg) \
    do                                                            \
    {                                                             \
        name.Cb.Callback    = cbfunc;                             \
        name.Cb.CallbackArg = cbarg;                              \
        name.Cb.StackSize   = depth;                              \
    } while (0)

/**
 * A combined macro to both instantiate and initialize an iterator
 */
#define EDSLIB_DECLARE_ITERATOR_CB(name, depth, cbfunc, cbarg) \
    EDSLIB_ITERATOR_CB_TYPE(depth) name;                       \
    EDSLIB_INITIALIZE_ITERATOR_CB(name, depth, cbfunc, cbarg)

/**
 * Macro to set an iterator to the start of a given object
 * The object is specified directory as an EdsLib_DatabaseRef_t
 */
#define EDSLIB_RESET_ITERATOR_FROM_REFOBJ(name, baseref)                \
    do                                                                  \
    {                                                                   \
        memset(&name.Cb.StackBase[0], 0, sizeof(name.Cb.StackBase[0])); \
        name.Cb.StackBase[0].Details.RefObj = baseref;                  \
    } while (0)

/**
 * Macro to set an iterator to the start of a given object
 * The object is specified directory as an EdsLib_Id_t
 */
#define EDSLIB_RESET_ITERATOR_FROM_EDSID(name, edsid)                        \
    do                                                                       \
    {                                                                        \
        memset(&name.Cb.StackBase[0], 0, sizeof(name.Cb.StackBase[0]));      \
        EdsLib_Decode_StructId(&name.Cb.StackBase[0].Details.RefObj, edsid); \
    } while (0)

typedef enum
{
    EDSLIB_BITPACK_OPERMODE_NONE = 0,
    EDSLIB_BITPACK_OPERMODE_PACK,
    EDSLIB_BITPACK_OPERMODE_UNPACK
} EdsLib_BitPack_OperMode_t;

typedef enum
{
    EDSLIB_PACKACTION_NONE = 0,
    EDSLIB_PACKACTION_PREPARE,
    EDSLIB_PACKACTION_BITPACK,
    EDSLIB_PACKACTION_BYTECOPY_INVERT,
    EDSLIB_PACKACTION_BYTECOPY_STRAIGHT,
    EDSLIB_PACKACTION_SUBCOMPONENTS,
    EDSLIB_PACKACTION_FAULT
} EdsLib_PackAction_t;

typedef struct
{
    EdsLib_EntryType_t               EntryType;
    uint32_t                         PackedBitOffset;
    EdsLib_HandlerArgument_t         HandlerArg;
    const EdsLib_DataTypeDB_Entry_t *DictPtr;
} EdsLib_PackedPostProc_DeferredField_t;

typedef struct EdsLib_DataTypePackUnpack_ControlBlock EdsLib_DataTypePackUnpack_ControlBlock_t;

/**
 * Callback for processing a structure member during pack/unpack operations
 *
 * This permits a different function to be used when encoding, decoding, or
 * populating or verifying error control fields.
 */
typedef int32_t (*EdsLib_DataTypePackUnpack_HandleMemberFunc_t)(EdsLib_DataTypePackUnpack_ControlBlock_t *,
                                                                const EdsLib_DataTypeIterator_StackEntry_t *,
                                                                uint32_t,
                                                                EdsLib_PackAction_t);

struct EdsLib_DataTypePackUnpack_ControlBlock
{
    const void                                  *NativeBufferPtr;
    EdsLib_DatabaseRef_t                         RefObj;
    EdsLib_SizeInfo_t                            MaxSize;
    EdsLib_SizeInfo_t                            LastNominalTail;
    uint32_t                                     LastActualTailBitPos;
    int32_t                                      Status;
    uint32_t                                     MaxPasses;
    EdsLib_DataTypePackUnpack_HandleMemberFunc_t HandleMember;
};

typedef struct
{
    EdsLib_DataTypePackUnpack_ControlBlock_t Common;
    const uint8_t                           *NativeSrcPtr;
    uint8_t                                 *PackedDstPtr;
} EdsLib_DataTypePack_State_t;

typedef struct
{
    EdsLib_DataTypePackUnpack_ControlBlock_t Common;
    const uint8_t                           *PackedSrcPtr;
    uint8_t                                 *NativeDstPtr;
} EdsLib_DataTypeUnpack_State_t;

typedef struct
{
    EdsLib_DataTypePack_State_t           Pack;
    uint16_t                              DeferredFieldCount;
    EdsLib_PackedPostProc_DeferredField_t DeferredFields[EDSLIB_PACKEDPOSTPROC_MAX_DEFERRED_FIELDS];
} EdsLib_PackedPostProc_ControlBlock_t;

typedef struct
{
    EdsLib_DataTypeUnpack_State_t Unpack;
    uint32_t                      CheckFields;
    uint32_t                      RecomputeFields;
} EdsLib_NativePostProc_ControlBlock_t;

typedef struct
{
    const EdsLib_DisplayDB_Entry_t *DisplayInf;
} EdsLib_DisplayIterator_StackEntry_t;

typedef EdsLib_Iterator_Rc_t (*EdsLib_DisplayIterator_Callback_t)(
    const EdsLib_DatabaseObject_t              *GD,
    EdsLib_Iterator_CbType_t                    CbType,
    const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
    const char                                 *EntityName,
    void                                       *OpaqueArg);

typedef struct
{
    EdsLib_DisplayIterator_StackEntry_t *NextStackEntry;
    EdsLib_DisplayIterator_Callback_t    NextCallback;
    void                                *NextCallbackArg;
} EdsLib_DisplayDB_InternalIterator_ControlBlock_t;

EdsLib_Iterator_Rc_t EdsLib_DisplayIterator_Wrapper(const EdsLib_DatabaseObject_t              *GD,
                                                    EdsLib_Iterator_CbType_t                    CbType,
                                                    const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
                                                    void                                       *OpaqueArg);

#define EDSLIB_DECLARE_DISPLAY_ITERATOR_CB(name, depth, cbfunc, cbarg)                                        \
    struct                                                                                                    \
    {                                                                                                         \
        EDSLIB_ITERATOR_CB_TYPE(depth) BaseIter;                                                              \
        EdsLib_DisplayDB_InternalIterator_ControlBlock_t DisplayCb;                                           \
        EdsLib_DisplayIterator_StackEntry_t              DisplayStack[depth];                                 \
    } name;                                                                                                   \
    do                                                                                                        \
    {                                                                                                         \
        EDSLIB_INITIALIZE_ITERATOR_CB(name.BaseIter, depth, EdsLib_DisplayIterator_Wrapper, &name.DisplayCb); \
        name.DisplayCb.NextCallback    = cbfunc;                                                              \
        name.DisplayCb.NextCallbackArg = cbarg;                                                               \
    } while (0)

#define EDSLIB_RESET_DISPLAY_ITERATOR_FROM_REFOBJ(name, baseref)   \
    do                                                             \
    {                                                              \
        EDSLIB_RESET_ITERATOR_FROM_REFOBJ(name.BaseIter, baseref); \
        name.DisplayCb.NextStackEntry = name.DisplayStack;         \
    } while (0)

#define EDSLIB_RESET_DISPLAY_ITERATOR_FROM_EDSID(name, edsid)   \
    do                                                          \
    {                                                           \
        EDSLIB_RESET_ITERATOR_FROM_EDSID(name.BaseIter, edsid); \
        name.DisplayCb.NextStackEntry = name.DisplayStack;      \
    } while (0)

typedef struct
{
    EdsLib_DatabaseRef_t           TargetRef;
    bool                           Recursive;
    EdsLib_ConstraintCallback_t    UserCallback;
    void                          *CbArg;
    EdsLib_GenericValueBuffer_t    TempConstraintValue;
    EdsLib_DataTypeDB_EntityInfo_t TempMemberInfo;
} EdsLib_ConstraintIterator_ControlBlock_t;

typedef enum
{
    EDSLIB_MATCHQUALITY_NONE,
    EDSLIB_MATCHQUALITY_PARTIAL,
    EDSLIB_MATCHQUALITY_EXACT
} EdsLib_MatchQuality_t;

typedef struct
{
    const char                      *NextTokenPos;
    const char                      *ContentPos;
    const EdsLib_DataTypeDB_Entry_t *DataDict;
    uint32_t                         ContentLength;
    EdsLib_MatchQuality_t            MatchQuality;
    EdsLib_DatabaseRef_t             RefObj;
    EdsLib_SizeInfo_t                StartOffset;
    EdsLib_SizeInfo_t                MaxSize;
} EdsLib_DisplayLocateMember_ControlBlock_t;

typedef struct
{
    EdsLib_EntityCallback_t UserCallback;
    void                   *UserArg;
} EdsLib_DisplayUserIterator_BaseName_ControlBlock_t;

typedef struct
{
    uint16_t ScratchOffset;
    bool     HasNamedMembers;
} EdsLib_DisplayUserIterator_FullName_StackEntry_t;

#define EDSLIB_ITERATOR_NAME_MAX_SIZE 256

typedef struct
{
    EdsLib_DisplayUserIterator_BaseName_ControlBlock_t Base;
    EdsLib_DisplayUserIterator_FullName_StackEntry_t  *NextEntry;
    char                                               ScratchNameBuffer[EDSLIB_ITERATOR_NAME_MAX_SIZE];
} EdsLib_DisplayUserIterator_FullName_ControlBlock_t;

typedef struct EdsLib_IntfDB_FullContext
{
    EdsLib_IntfDB_t                       IntfDBPtr;
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    const EdsLib_IntfDB_DeclIntfEntry_t  *DeclIntfEntry;
    const EdsLib_IntfDB_InterfaceEntry_t *CompIntfEntry;
    const EdsLib_IntfDB_CommandEntry_t   *CmdEntry;
} EdsLib_IntfDB_FullContext_t;

/**********************************************************
 * PROTOTYPES - General purpose helper functions
 **********************************************************/

/**
 * Initialize the error control algorithms
 */
void EdsLib_ErrorControl_Initialize(void);

/**********************************************************
 * PROTOTYPES - DataTypeDB helper functions
 *
 * These are internal helper/utility functions implemented as part of the
 * data type DB aka "basic" runtime library.  These are NOT part of the public API.
 *
 **********************************************************/

EdsLib_DataTypeDB_t              EdsLib_DataTypeDB_GetTopLevel(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx);
const EdsLib_DataTypeDB_Entry_t *EdsLib_DataTypeDB_GetEntry(const EdsLib_DatabaseObject_t *GD,
                                                            const EdsLib_DatabaseRef_t    *RefObj);
void                             EdsLib_DataTypeDB_CopyTypeInfo(const EdsLib_DataTypeDB_Entry_t *DataDictEntry,
                                                                EdsLib_DataTypeDB_TypeInfo_t    *TypeInfo);

void EdsLib_DataTypeLoad_Impl(EdsLib_GenericValueBuffer_t     *ValueBuff,
                              EdsLib_ConstPtr_t                SrcPtr,
                              const EdsLib_DataTypeDB_Entry_t *DictEntryPtr);
void EdsLib_DataTypeStore_Impl(EdsLib_Ptr_t                     DstPtr,
                               EdsLib_GenericValueBuffer_t     *SrcBuff,
                               const EdsLib_DataTypeDB_Entry_t *DictEntryPtr);

typedef int8_t (*EdsLib_EntryCompareFunc_t)(const void *, const EdsLib_FieldDetailEntry_t *);
typedef uint16_t (*EdsLib_GetArrayIdxFunc_t)(const void *, const EdsLib_SizeInfo_t *);

int8_t EdsLib_OffsetCompareBits(const void *SubjectPtr, const EdsLib_FieldDetailEntry_t *RefEntry);
int8_t EdsLib_OffsetCompareBytes(const void *SubjectPtr, const EdsLib_FieldDetailEntry_t *RefEntry);

uint16_t EdsLib_GetArrayIdxFromBits(const void *SubjectPtr, const EdsLib_SizeInfo_t *ElemSize);
uint16_t EdsLib_GetArrayIdxFromBytes(const void *SubjectPtr, const EdsLib_SizeInfo_t *ElemSize);

int32_t EdsLib_DataTypeDB_FindContainerMember_Impl(const EdsLib_DatabaseObject_t   *GD,
                                                   const EdsLib_DataTypeDB_Entry_t *BaseDict,
                                                   const void                      *CompareArg,
                                                   EdsLib_EntryCompareFunc_t        CompFunc,
                                                   EdsLib_DataTypeDB_EntityInfo_t  *CompInfo);

int32_t EdsLib_DataTypeDB_FindArrayMember_Impl(const EdsLib_DatabaseObject_t   *GD,
                                               const EdsLib_DataTypeDB_Entry_t *BaseDict,
                                               const void                      *GetIdxArg,
                                               EdsLib_GetArrayIdxFunc_t         GetIdxFunc,
                                               EdsLib_DataTypeDB_EntityInfo_t  *EntInfo);

int32_t EdsLib_DataTypeIterator_Impl(const EdsLib_DatabaseObject_t          *GD,
                                     EdsLib_DataTypeIterator_ControlBlock_t *StateInfo);

int32_t EdsLib_DataTypeDB_ConstraintIterator_Impl(const EdsLib_DatabaseObject_t            *GD,
                                                  EdsLib_ConstraintIterator_ControlBlock_t *CtlBlock,
                                                  const EdsLib_DatabaseRef_t               *BaseRef);
int32_t EdsLib_DataTypeDB_ConstraintIterator(const EdsLib_DatabaseObject_t *GD,
                                             EdsLib_Id_t                    BaseId,
                                             EdsLib_Id_t                    DerivedId,
                                             EdsLib_ConstraintCallback_t    Callback,
                                             void                          *CbArg);
const EdsLib_IdentSequenceEntry_t *
EdsLib_DataTypeExecuteConstraintSequence_Impl(const EdsLib_DatabaseObject_t        *GD,
                                              const EdsLib_IdentityCheckSequence_t *CheckSequence,
                                              const void                           *Buffer,
                                              size_t                                MaxOffset);

int32_t EdsLib_DataTypePack_HandleMember(EdsLib_DataTypePackUnpack_ControlBlock_t   *Parent,
                                         const EdsLib_DataTypeIterator_StackEntry_t *CbInfo,
                                         uint32_t                                    BitPosition,
                                         EdsLib_PackAction_t                         PackAction);

int32_t EdsLib_DataTypeUnpack_HandleMember(EdsLib_DataTypePackUnpack_ControlBlock_t   *Parent,
                                           const EdsLib_DataTypeIterator_StackEntry_t *CbInfo,
                                           uint32_t                                    BitPosition,
                                           EdsLib_PackAction_t                         PackAction);

int32_t EdsLib_DataTypeDB_PackUnpackFindTailEntity(const EdsLib_DatabaseObject_t  *GD,
                                                   EdsLib_Id_t                     EdsId,
                                                   const EdsLib_SizeInfo_t        *TargetOffset,
                                                   EdsLib_DataTypeDB_EntityInfo_t *ResultInfo);

int32_t EdsLib_DataTypeDB_PackUnpackFindTail(const EdsLib_DatabaseObject_t            *GD,
                                             EdsLib_Id_t                               EdsId,
                                             const EdsLib_SizeInfo_t                  *InputPtr,
                                             EdsLib_DataTypePackUnpack_ControlBlock_t *CbPtr);

void    EdsLib_DataTypePackUnpack_Impl(const EdsLib_DatabaseObject_t            *GD,
                                       EdsLib_DataTypePackUnpack_ControlBlock_t *PackState);
int32_t EdsLib_DataTypeIdentifyBuffer_Impl(const EdsLib_DatabaseObject_t   *GD,
                                           const EdsLib_DataTypeDB_Entry_t *DataDictPtr,
                                           const void                      *Buffer,
                                           size_t                           MaxOffset,
                                           uint16_t                        *DerivTableIndex,
                                           EdsLib_DatabaseRef_t            *ActualObj);

void EdsLib_DataTypeConstraintEntityLookup_Impl(const EdsLib_DataTypeDB_Entry_t *DataDictPtr,
                                                uint16_t                         ConstraintIdx,
                                                const EdsLib_DatabaseRef_t     **RefObjPtr,
                                                EdsLib_SizeInfo_t               *Offset);

int32_t EdsLib_NativeObject_PostProc_Callback(EdsLib_DataTypePackUnpack_ControlBlock_t   *Parent,
                                              const EdsLib_DataTypeIterator_StackEntry_t *CbInfo,
                                              uint32_t                                    BitPosition,
                                              EdsLib_PackAction_t                         PackAction);

int32_t EdsLib_PackedObject_PostProc_Callback(EdsLib_DataTypePackUnpack_ControlBlock_t   *Parent,
                                              const EdsLib_DataTypeIterator_StackEntry_t *CbInfo,
                                              uint32_t                                    BitPosition,
                                              EdsLib_PackAction_t                         PackAction);

void EdsLib_PackedObject_PostProc_Deferred(EdsLib_PackedPostProc_ControlBlock_t *Base);

void EdsLib_UpdateErrorControlField(const EdsLib_DataTypeDB_Entry_t *ErrorCtlDictPtr,
                                    void                            *PackedObject,
                                    uint32_t                         TotalBitSize,
                                    EdsLib_ErrorControlType_t        ErrorCtlType,
                                    uint32_t                         ErrorCtlOffsetBits);

/**********************************************************
 * PROTOTYPES - DisplayDB helper functions
 *
 * These are internal helper/utility functions implemented as part of the
 * display/UI DB aka "full" runtime library.  These are NOT part of the public API.
 *
 **********************************************************/

EdsLib_DisplayDB_t              EdsLib_DisplayDB_GetTopLevel(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx);
const EdsLib_DisplayDB_Entry_t *EdsLib_DisplayDB_GetEntry(const EdsLib_DatabaseObject_t *GD,
                                                          const EdsLib_DatabaseRef_t    *RefObj);

const EdsLib_SymbolTableEntry_t *EdsLib_DisplaySymbolLookup_GetByName(const EdsLib_SymbolTableEntry_t *SymbolDict,
                                                                      uint16_t                         TableSize,
                                                                      const char                      *String,
                                                                      uint32_t                         StringLen);
const EdsLib_SymbolTableEntry_t *
EdsLib_DisplaySymbolLookup_GetByValue(const EdsLib_SymbolTableEntry_t *SymbolDict, uint16_t TableSize, intmax_t Value);

void EdsLib_DisplayLocateMember_Impl(const EdsLib_DatabaseObject_t             *GD,
                                     EdsLib_DisplayLocateMember_ControlBlock_t *CtrlBlock);

EdsLib_Iterator_Rc_t
EdsLib_DisplayUserIterator_BaseName_Callback(const EdsLib_DatabaseObject_t              *GD,
                                             EdsLib_Iterator_CbType_t                    CbType,
                                             const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
                                             const char                                 *EntityName,
                                             void                                       *OpaqueArg);

EdsLib_Iterator_Rc_t
EdsLib_DisplayUserIterator_FullName_Callback(const EdsLib_DatabaseObject_t              *GD,
                                             EdsLib_Iterator_CbType_t                    CbType,
                                             const EdsLib_DataTypeIterator_StackEntry_t *EntityInfo,
                                             const char                                 *EntityName,
                                             void                                       *OpaqueArg);

int32_t EdsLib_DisplayScalarConv_ToString_Impl(const EdsLib_DataTypeDB_Entry_t *DictEntryPtr,
                                               const EdsLib_DisplayDB_Entry_t  *DisplayInfoPtr,
                                               char                            *OutputBuffer,
                                               uint32_t                         BufferSize,
                                               const void                      *SourcePtr);

int32_t EdsLib_DisplayScalarConv_FromString_Impl(const EdsLib_DataTypeDB_Entry_t *DictEntryPtr,
                                                 const EdsLib_DisplayDB_Entry_t  *DisplayInfoPtr,
                                                 void                            *DestPtr,
                                                 const char                      *SrcString);

uintmax_t EdsLib_ErrorControlCompute(EdsLib_ErrorControlType_t Algorithm,
                                     const void               *Buffer,
                                     uint32_t                  BufferSizeBits,
                                     uint32_t                  ErrCtlBitPos);

/**********************************************************
 * PROTOTYPES - IntfDB lookup helper functions
 *
 * These are internal helper/utility functions implemented as part of the
 * Interface DB.  These are NOT part of the public API.
 *
 **********************************************************/

EdsLib_IntfDB_t                       EdsLib_IntfDB_GetTopLevel(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx);
const EdsLib_IntfDB_ComponentEntry_t *EdsLib_IntfDB_GetComponentEntry(const EdsLib_DatabaseObject_t *GD,
                                                                      const EdsLib_DatabaseRef_t    *RefObj);
const EdsLib_IntfDB_DeclIntfEntry_t  *EdsLib_IntfDB_GetDeclIntfEntry(const EdsLib_DatabaseObject_t *GD,
                                                                     const EdsLib_DatabaseRef_t    *RefObj);
const EdsLib_IntfDB_InterfaceEntry_t *
EdsLib_IntfDB_GetInterfaceEntryFromCompIdx(const EdsLib_IntfDB_ComponentEntry_t *CompEntry, uint16_t CompIdx);
const EdsLib_IntfDB_CommandEntry_t *
EdsLib_IntfDB_GetCommandEntryFromIntfIdx(const EdsLib_IntfDB_DeclIntfEntry_t *IntfEntry, uint16_t IntfIdx);
const EdsLib_IntfDB_InterfaceEntry_t *
EdsLib_IntfDB_GetComponentInterfaceEntry(const EdsLib_DatabaseObject_t         *GD,
                                         const EdsLib_DatabaseRef_t            *RefObj,
                                         const EdsLib_IntfDB_ComponentEntry_t **CompEntryOut);
const EdsLib_IntfDB_CommandEntry_t *
EdsLib_IntfDB_GetDeclIntfCommandEntry(const EdsLib_DatabaseObject_t        *GD,
                                      const EdsLib_DatabaseRef_t           *RefObj,
                                      const EdsLib_IntfDB_DeclIntfEntry_t **DeclIntfOut);

int32_t EdsLib_IntfDB_FindComponentBySubstring(EdsLib_IntfDB_t IntfDBPtr,
                                               const char     *CompName,
                                               size_t          MatchLen,
                                               uint16_t       *IdxOut);
int32_t EdsLib_IntfDB_FindDeclIntfBySubstring(EdsLib_IntfDB_t IntfDBPtr,
                                              const char     *IntfName,
                                              size_t          MatchLen,
                                              uint16_t       *IdxOut);
int32_t EdsLib_IntfDB_FindComponentInterfaceBySubstring(const EdsLib_IntfDB_ComponentEntry_t *CompEntry,
                                                        const char                           *IntfName,
                                                        size_t                                MatchLen,
                                                        uint16_t                             *IdxOut);
int32_t EdsLib_IntfDB_FindComponentByQualifiedName(const EdsLib_DatabaseObject_t *GD,
                                                   const char                    *CompName,
                                                   size_t                         MatchLen,
                                                   EdsLib_DatabaseRef_t          *RefObj);
int32_t EdsLib_IntfDB_FindDeclIntfByQualifiedName(const EdsLib_DatabaseObject_t *GD,
                                                  const char                    *IntfName,
                                                  size_t                         MatchLen,
                                                  EdsLib_DatabaseRef_t          *RefObj);
int32_t EdsLib_IntfDB_FindFullContext(const EdsLib_DatabaseObject_t *GD,
                                      const EdsLib_DatabaseRef_t    *RefObj,
                                      EdsLib_IntfDB_FullContext_t   *CtxtOut);

#endif /* _EDSLIB_INTERNAL_H_ */
