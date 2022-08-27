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
 * Auto-Generated stub implementations for functions defined in edslib_displaydb header
 */

#include "edslib_displaydb.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_Base64Decode()
 * ----------------------------------------------------
 */
void EdsLib_DisplayDB_Base64Decode(uint8_t *Out, uint32_t OutputLenBits, const char *In)
{
    UT_GenStub_AddParam(EdsLib_DisplayDB_Base64Decode, uint8_t *, Out);
    UT_GenStub_AddParam(EdsLib_DisplayDB_Base64Decode, uint32_t, OutputLenBits);
    UT_GenStub_AddParam(EdsLib_DisplayDB_Base64Decode, const char *, In);

    UT_GenStub_Execute(EdsLib_DisplayDB_Base64Decode, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_Base64Encode()
 * ----------------------------------------------------
 */
void EdsLib_DisplayDB_Base64Encode(char *Out, uint32_t OutputLenBytes, const uint8_t *In, uint32_t InputLenBits)
{
    UT_GenStub_AddParam(EdsLib_DisplayDB_Base64Encode, char *, Out);
    UT_GenStub_AddParam(EdsLib_DisplayDB_Base64Encode, uint32_t, OutputLenBytes);
    UT_GenStub_AddParam(EdsLib_DisplayDB_Base64Encode, const uint8_t *, In);
    UT_GenStub_AddParam(EdsLib_DisplayDB_Base64Encode, uint32_t, InputLenBits);

    UT_GenStub_Execute(EdsLib_DisplayDB_Base64Encode, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetBaseName()
 * ----------------------------------------------------
 */
const char *EdsLib_DisplayDB_GetBaseName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetBaseName, const char *);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetBaseName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetBaseName, EdsLib_Id_t, EdsId);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetBaseName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetBaseName, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetDisplayHint()
 * ----------------------------------------------------
 */
EdsLib_DisplayHint_t EdsLib_DisplayDB_GetDisplayHint(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetDisplayHint, EdsLib_DisplayHint_t);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetDisplayHint, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetDisplayHint, EdsLib_Id_t, EdsId);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetDisplayHint, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetDisplayHint, EdsLib_DisplayHint_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetEdsName()
 * ----------------------------------------------------
 */
const char *EdsLib_DisplayDB_GetEdsName(const EdsLib_DatabaseObject_t *GD, uint16_t AppId)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetEdsName, const char *);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEdsName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEdsName, uint16_t, AppId);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetEdsName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetEdsName, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetEnumLabel()
 * ----------------------------------------------------
 */
const char *EdsLib_DisplayDB_GetEnumLabel(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                          const EdsLib_GenericValueBuffer_t *ValueBuffer)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetEnumLabel, const char *);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEnumLabel, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEnumLabel, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEnumLabel, const EdsLib_GenericValueBuffer_t *, ValueBuffer);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetEnumLabel, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetEnumLabel, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetEnumValue()
 * ----------------------------------------------------
 */
void EdsLib_DisplayDB_GetEnumValue(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *String,
                                   EdsLib_GenericValueBuffer_t *ValueBuffer)
{
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEnumValue, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEnumValue, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEnumValue, const char *, String);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetEnumValue, EdsLib_GenericValueBuffer_t *, ValueBuffer);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetEnumValue, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetIndexByName()
 * ----------------------------------------------------
 */
int32_t EdsLib_DisplayDB_GetIndexByName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name,
                                        uint16_t *SubIndex)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetIndexByName, int32_t);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetIndexByName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetIndexByName, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetIndexByName, const char *, Name);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetIndexByName, uint16_t *, SubIndex);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetIndexByName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetIndexByName, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetNameByIndex()
 * ----------------------------------------------------
 */
const char *EdsLib_DisplayDB_GetNameByIndex(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t SubIndex)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetNameByIndex, const char *);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetNameByIndex, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetNameByIndex, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetNameByIndex, uint16_t, SubIndex);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetNameByIndex, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetNameByIndex, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetNamespace()
 * ----------------------------------------------------
 */
const char *EdsLib_DisplayDB_GetNamespace(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetNamespace, const char *);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetNamespace, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetNamespace, EdsLib_Id_t, EdsId);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetNamespace, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetNamespace, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_GetTypeName()
 * ----------------------------------------------------
 */
const char *EdsLib_DisplayDB_GetTypeName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *Buffer,
                                         uint32_t BufferSize)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_GetTypeName, const char *);

    UT_GenStub_AddParam(EdsLib_DisplayDB_GetTypeName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetTypeName, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetTypeName, char *, Buffer);
    UT_GenStub_AddParam(EdsLib_DisplayDB_GetTypeName, uint32_t, BufferSize);

    UT_GenStub_Execute(EdsLib_DisplayDB_GetTypeName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_GetTypeName, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_Initialize()
 * ----------------------------------------------------
 */
void EdsLib_DisplayDB_Initialize(void)
{

    UT_GenStub_Execute(EdsLib_DisplayDB_Initialize, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_IterateAllEntities()
 * ----------------------------------------------------
 */
void EdsLib_DisplayDB_IterateAllEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                         EdsLib_EntityCallback_t Callback, void *Arg)
{
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateAllEntities, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateAllEntities, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateAllEntities, EdsLib_EntityCallback_t, Callback);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateAllEntities, void *, Arg);

    UT_GenStub_Execute(EdsLib_DisplayDB_IterateAllEntities, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_IterateBaseEntities()
 * ----------------------------------------------------
 */
void EdsLib_DisplayDB_IterateBaseEntities(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                          EdsLib_EntityCallback_t Callback, void *Arg)
{
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateBaseEntities, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateBaseEntities, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateBaseEntities, EdsLib_EntityCallback_t, Callback);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateBaseEntities, void *, Arg);

    UT_GenStub_Execute(EdsLib_DisplayDB_IterateBaseEntities, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_IterateEnumValues()
 * ----------------------------------------------------
 */
void EdsLib_DisplayDB_IterateEnumValues(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId,
                                        EdsLib_SymbolCallback_t Callback, void *Arg)
{
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateEnumValues, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateEnumValues, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateEnumValues, EdsLib_SymbolCallback_t, Callback);
    UT_GenStub_AddParam(EdsLib_DisplayDB_IterateEnumValues, void *, Arg);

    UT_GenStub_Execute(EdsLib_DisplayDB_IterateEnumValues, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_LocateSubEntity()
 * ----------------------------------------------------
 */
int32_t EdsLib_DisplayDB_LocateSubEntity(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const char *Name,
                                         EdsLib_DataTypeDB_EntityInfo_t *CompInfo)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_LocateSubEntity, int32_t);

    UT_GenStub_AddParam(EdsLib_DisplayDB_LocateSubEntity, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_LocateSubEntity, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_DisplayDB_LocateSubEntity, const char *, Name);
    UT_GenStub_AddParam(EdsLib_DisplayDB_LocateSubEntity, EdsLib_DataTypeDB_EntityInfo_t *, CompInfo);

    UT_GenStub_Execute(EdsLib_DisplayDB_LocateSubEntity, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_LocateSubEntity, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DisplayDB_LookupTypeName()
 * ----------------------------------------------------
 */
EdsLib_Id_t EdsLib_DisplayDB_LookupTypeName(const EdsLib_DatabaseObject_t *GD, const char *String)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DisplayDB_LookupTypeName, EdsLib_Id_t);

    UT_GenStub_AddParam(EdsLib_DisplayDB_LookupTypeName, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_DisplayDB_LookupTypeName, const char *, String);

    UT_GenStub_Execute(EdsLib_DisplayDB_LookupTypeName, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DisplayDB_LookupTypeName, EdsLib_Id_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Generate_Hexdump()
 * ----------------------------------------------------
 */
void EdsLib_Generate_Hexdump(void *output, const uint8_t *DataPtr, uint16_t DisplayOffset, uint16_t Count)
{
    UT_GenStub_AddParam(EdsLib_Generate_Hexdump, void *, output);
    UT_GenStub_AddParam(EdsLib_Generate_Hexdump, const uint8_t *, DataPtr);
    UT_GenStub_AddParam(EdsLib_Generate_Hexdump, uint16_t, DisplayOffset);
    UT_GenStub_AddParam(EdsLib_Generate_Hexdump, uint16_t, Count);

    UT_GenStub_Execute(EdsLib_Generate_Hexdump, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Scalar_FromString()
 * ----------------------------------------------------
 */
int32_t EdsLib_Scalar_FromString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr,
                                 const char *SrcString)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Scalar_FromString, int32_t);

    UT_GenStub_AddParam(EdsLib_Scalar_FromString, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_Scalar_FromString, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_Scalar_FromString, void *, DestPtr);
    UT_GenStub_AddParam(EdsLib_Scalar_FromString, const char *, SrcString);

    UT_GenStub_Execute(EdsLib_Scalar_FromString, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Scalar_FromString, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Scalar_ToString()
 * ----------------------------------------------------
 */
int32_t EdsLib_Scalar_ToString(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *OutputBuffer,
                               uint32_t BufferSize, const void *SrcPtr)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Scalar_ToString, int32_t);

    UT_GenStub_AddParam(EdsLib_Scalar_ToString, const EdsLib_DatabaseObject_t *, GD);
    UT_GenStub_AddParam(EdsLib_Scalar_ToString, EdsLib_Id_t, EdsId);
    UT_GenStub_AddParam(EdsLib_Scalar_ToString, char *, OutputBuffer);
    UT_GenStub_AddParam(EdsLib_Scalar_ToString, uint32_t, BufferSize);
    UT_GenStub_AddParam(EdsLib_Scalar_ToString, const void *, SrcPtr);

    UT_GenStub_Execute(EdsLib_Scalar_ToString, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Scalar_ToString, int32_t);
}
