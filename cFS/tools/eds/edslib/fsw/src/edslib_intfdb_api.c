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
 * \file     edslib_intfdb_api.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of public EDS API functions to locate members within EDS structure objects.
 *
 * Linked as part of the "full" EDS runtime library
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "edslib_intfdb.h"
#include "edslib_internal.h"

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 *
 * Initialization routine placeholder.
 *
 * Currently a no-op, as there are no global data
 * structures needing initialization at this time.
 *
 * For now, this is a placeholder in case that changes,
 * but this also serves an important role for static linking
 * to ensure that this code is linked into the target.
 *
 *-----------------------------------------------------------------*/
void EdsLib_IntfDB_Initialize(void)
{
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function
 *
 *-----------------------------------------------------------------*/
void EdsLib_IntfDB_ExportComponentInfo(const EdsLib_IntfDB_ComponentEntry_t *CompEntry,
                                       EdsLib_IntfDB_ComponentInfo_t        *CompInfoBuffer)
{
    if (CompInfoBuffer != NULL)
    {
        /* Not copying the name for now, should be OK as the output is const */
        CompInfoBuffer->ComponentName     = CompEntry->Name;
        CompInfoBuffer->RequiredIntfCount = CompEntry->RequiredIntfCount;
    }
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function
 *
 *-----------------------------------------------------------------*/
void EdsLib_IntfDB_ExportCommandInfo(const EdsLib_IntfDB_CommandEntry_t *CmdEntry,
                                     EdsLib_IntfDB_CommandInfo_t        *CmdInfoBuffer,
                                     const EdsLib_DatabaseRef_t         *SelfRefObj)
{
    EdsLib_DatabaseRef_t ParentRefObj;

    if (CmdInfoBuffer != NULL)
    {
        /* Not copying the name for now, should be OK as the output is const */
        CmdInfoBuffer->CommandName      = CmdEntry->Name;
        ParentRefObj                    = *SelfRefObj;
        ParentRefObj.SubIndex           = CmdEntry->ParentIdx;
        CmdInfoBuffer->ParentDeclIntfId = EdsLib_Encode_StructId(&ParentRefObj);
        CmdInfoBuffer->ArgumentCount    = CmdEntry->ArgumentCount;
    }
}

/*----------------------------------------------------------------
 *
 * EdsLib local helper function
 *
 *-----------------------------------------------------------------*/
void EdsLib_IntfDB_ExportInterfaceInfo(const EdsLib_IntfDB_InterfaceEntry_t *IntfEntry,
                                       EdsLib_IntfDB_InterfaceInfo_t        *IntfInfoBuffer,
                                       const EdsLib_DatabaseRef_t           *SelfRefObj)
{
    EdsLib_DatabaseRef_t ParentRefObj;

    if (IntfInfoBuffer != NULL)
    {
        /* Not copying the name for now, should be OK as the output is const */
        IntfInfoBuffer->InterfaceName   = IntfEntry->Name;
        IntfInfoBuffer->IntfTypeEdsId   = EdsLib_Encode_StructId(&IntfEntry->IntfTypeRef);
        ParentRefObj                    = *SelfRefObj;
        ParentRefObj.SubIndex           = IntfEntry->ParentIdx;
        IntfInfoBuffer->ParentCompEdsId = EdsLib_Encode_StructId(&ParentRefObj);

        IntfInfoBuffer->GenericTypeMapCount = IntfEntry->GenericTypeMapCount;
    }
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_GetComponentInfo(const EdsLib_DatabaseObject_t *GD,
                                       EdsLib_Id_t                    ComponentId,
                                       EdsLib_IntfDB_ComponentInfo_t *CompInfoBuffer)
{
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    EdsLib_DatabaseRef_t                  RefObj;
    int32_t                               Status;

    EdsLib_Decode_StructId(&RefObj, ComponentId);

    CompEntry = EdsLib_IntfDB_GetComponentEntry(GD, &RefObj);
    if (CompEntry == NULL)
    {
        Status = EDSLIB_INVALID_INDEX;
    }
    else
    {
        EdsLib_IntfDB_ExportComponentInfo(CompEntry, CompInfoBuffer);
        Status = EDSLIB_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindComponentByLocalName(const EdsLib_DatabaseObject_t *GD,
                                               uint16_t                       AppIdx,
                                               const char                    *CompName,
                                               EdsLib_Id_t                   *IdBuffer)
{
    EdsLib_IntfDB_t      IntfDBPtr;
    EdsLib_DatabaseRef_t RefObj;
    int32_t              Status;

    Status = EDSLIB_NAME_NOT_FOUND;
    memset(&RefObj, 0, sizeof(RefObj));

    RefObj.Qualifier = EdsLib_DbRef_Qualifier_INTERFACE;
    RefObj.AppIndex  = AppIdx;

    IntfDBPtr = EdsLib_IntfDB_GetTopLevel(GD, AppIdx);
    if (IntfDBPtr != NULL)
    {
        Status = EdsLib_IntfDB_FindComponentBySubstring(IntfDBPtr, CompName, strlen(CompName), &RefObj.SubIndex);
    }

    if (IdBuffer != NULL)
    {
        if (Status == EDSLIB_SUCCESS)
        {
            *IdBuffer = EdsLib_Encode_StructId(&RefObj);
        }
        else
        {
            *IdBuffer = EDSLIB_ID_INVALID;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindComponentInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                        EdsLib_Id_t                    ComponentId,
                                                        const char                    *IntfName,
                                                        EdsLib_Id_t                   *IdBuffer)
{
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    EdsLib_DatabaseRef_t                  RefObj;
    int32_t                               Status;

    EdsLib_Decode_StructId(&RefObj, ComponentId);

    CompEntry = EdsLib_IntfDB_GetComponentEntry(GD, &RefObj);
    if (CompEntry == NULL)
    {
        Status = EDSLIB_INVALID_INDEX;
    }
    else
    {
        Status =
            EdsLib_IntfDB_FindComponentInterfaceBySubstring(CompEntry, IntfName, strlen(IntfName), &RefObj.SubIndex);
    }

    if (IdBuffer != NULL)
    {
        if (Status == EDSLIB_SUCCESS)
        {
            *IdBuffer = EdsLib_Encode_StructId(&RefObj);
        }
        else
        {
            *IdBuffer = EDSLIB_ID_INVALID;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindDeclaredInterfaceByFullName(const EdsLib_DatabaseObject_t *GD,
                                                      const char                    *IntfName,
                                                      EdsLib_Id_t                   *IdBuffer)
{
    EdsLib_DatabaseRef_t RefObj;
    int32_t              Status;

    Status = EdsLib_IntfDB_FindDeclIntfByQualifiedName(GD, IntfName, strlen(IntfName), &RefObj);

    if (IdBuffer != NULL)
    {
        if (Status == EDSLIB_SUCCESS)
        {
            *IdBuffer = EdsLib_Encode_StructId(&RefObj);
        }
        else
        {
            *IdBuffer = EDSLIB_ID_INVALID;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindComponentInterfaceByFullName(const EdsLib_DatabaseObject_t *GD,
                                                       const char                    *IntfName,
                                                       EdsLib_Id_t                   *IdBuffer)
{
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    const char                           *Separator;
    EdsLib_DatabaseRef_t                  RefObj;
    int32_t                               Status;

    /* The fully-qualified SEDS name uses / chars to separate name parts,
     * first is the package name (app), then the component name, then the interface name.
     * This needs to find the second separator char, which should be the qualified component name */
    Separator = strchr(IntfName, '/');
    if (Separator != NULL)
    {
        Separator = strchr(Separator + 1, '/');
    }

    if (Separator == NULL)
    {
        Status = EDSLIB_NAME_NOT_FOUND;
    }
    else
    {
        Status = EdsLib_IntfDB_FindComponentByQualifiedName(GD, IntfName, Separator - IntfName, &RefObj);
        ++Separator;
    }

    if (Status == EDSLIB_SUCCESS)
    {
        CompEntry = EdsLib_IntfDB_GetComponentEntry(GD, &RefObj);
        if (CompEntry == NULL)
        {
            Status = EDSLIB_INVALID_INDEX;
        }
        else
        {
            Status = EdsLib_IntfDB_FindComponentInterfaceBySubstring(CompEntry,
                                                                     Separator,
                                                                     strlen(Separator),
                                                                     &RefObj.SubIndex);
        }
    }

    if (IdBuffer != NULL)
    {
        if (Status == EDSLIB_SUCCESS)
        {
            *IdBuffer = EdsLib_Encode_StructId(&RefObj);
        }
        else
        {
            *IdBuffer = EDSLIB_ID_INVALID;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindDeclaredInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                       uint16_t                       AppIdx,
                                                       const char                    *IntfName,
                                                       EdsLib_Id_t                   *IdBuffer)
{
    EdsLib_IntfDB_t                      IntfDBPtr;
    const EdsLib_IntfDB_DeclIntfEntry_t *IntfPtr;
    EdsLib_DatabaseRef_t                 RefObj;
    int32_t                              Status;
    uint16_t                             i;

    memset(&RefObj, 0, sizeof(RefObj));
    Status    = EDSLIB_NAME_NOT_FOUND;
    IntfDBPtr = EdsLib_IntfDB_GetTopLevel(GD, AppIdx);

    if (IntfDBPtr != NULL)
    {
        for (i = 0; i < IntfDBPtr->DeclIntfListSize; ++i)
        {
            IntfPtr = &IntfDBPtr->DeclIntfListEntries[i];

            if (IntfPtr->Name != NULL && strcmp(IntfPtr->Name, IntfName) == 0)
            {
                RefObj.Qualifier = EdsLib_DbRef_Qualifier_INTERFACE;
                RefObj.AppIndex  = AppIdx;
                RefObj.SubIndex  = 1 + i;

                Status = EDSLIB_SUCCESS;
                break;
            }
        }
    }

    if (IdBuffer != NULL)
    {
        if (Status == EDSLIB_SUCCESS)
        {
            *IdBuffer = EdsLib_Encode_StructId(&RefObj);
        }
        else
        {
            *IdBuffer = EDSLIB_ID_INVALID;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindCommandByLocalName(const EdsLib_DatabaseObject_t *GD,
                                             EdsLib_Id_t                    IntfId,
                                             const char                    *CmdName,
                                             EdsLib_Id_t                   *IdBuffer)
{
    const EdsLib_IntfDB_DeclIntfEntry_t *IntfEntry;
    const EdsLib_IntfDB_CommandEntry_t  *CmdEntry;
    EdsLib_DatabaseRef_t                 RefObj;
    uint16_t                             IntfIdx;
    uint16_t                             Count;
    int32_t                              Status;

    Status = EDSLIB_INVALID_INDEX;

    EdsLib_Decode_StructId(&RefObj, IntfId);

    IntfEntry = EdsLib_IntfDB_GetDeclIntfEntry(GD, &RefObj);
    if (IntfEntry != NULL)
    {
        Count   = IntfEntry->CommandCount;
        IntfIdx = IntfEntry->FirstIdx + 1;
        while (Count > 0)
        {
            CmdEntry = EdsLib_IntfDB_GetCommandEntryFromIntfIdx(IntfEntry, IntfIdx);

            if (CmdEntry != NULL && CmdEntry->Name != NULL && strcmp(CmdEntry->Name, CmdName) == 0)
            {
                /* Just update the RefObj so it refers to this intf */
                RefObj.SubIndex = IntfIdx;

                Status = EDSLIB_SUCCESS;
                break;
            }

            ++IntfIdx;
            --Count;
        }
    }

    if (IdBuffer != NULL)
    {
        if (Status == EDSLIB_SUCCESS)
        {
            *IdBuffer = EdsLib_Encode_StructId(&RefObj);
        }
        else
        {
            *IdBuffer = EDSLIB_ID_INVALID;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_GetCommandInfo(const EdsLib_DatabaseObject_t *GD,
                                     EdsLib_Id_t                    CommandId,
                                     EdsLib_IntfDB_CommandInfo_t   *CommandInfoBuffer)
{
    const EdsLib_IntfDB_CommandEntry_t *CmdEntry;
    EdsLib_DatabaseRef_t                RefObj;
    int32_t                             Status;

    EdsLib_Decode_StructId(&RefObj, CommandId);

    CmdEntry = EdsLib_IntfDB_GetDeclIntfCommandEntry(GD, &RefObj, NULL);
    if (CmdEntry == NULL)
    {
        Status = EDSLIB_INVALID_INDEX;
    }
    else
    {
        EdsLib_IntfDB_ExportCommandInfo(CmdEntry, CommandInfoBuffer, &RefObj);
        Status = EDSLIB_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_GetComponentInterfaceInfo(const EdsLib_DatabaseObject_t *GD,
                                                EdsLib_Id_t                    IntfId,
                                                EdsLib_IntfDB_InterfaceInfo_t *IntfInfoBuffer)
{
    const EdsLib_IntfDB_InterfaceEntry_t *IntfEntry;
    EdsLib_DatabaseRef_t                  RefObj;
    int32_t                               Status;

    EdsLib_Decode_StructId(&RefObj, IntfId);

    IntfEntry = EdsLib_IntfDB_GetComponentInterfaceEntry(GD, &RefObj, NULL);
    if (IntfEntry == NULL)
    {
        Status = EDSLIB_INVALID_INDEX;
    }
    else
    {
        EdsLib_IntfDB_ExportInterfaceInfo(IntfEntry, IntfInfoBuffer, &RefObj);
        Status = EDSLIB_SUCCESS;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindAllArgumentTypes(const EdsLib_DatabaseObject_t *GD,
                                           EdsLib_Id_t                    CmdEdsId,
                                           EdsLib_Id_t                    CompIntfEdsId,
                                           EdsLib_Id_t                   *IdBuffer,
                                           size_t                         NumIdBufs)
{
    const EdsLib_IntfDB_InterfaceEntry_t     *IntfEntry;
    const EdsLib_IntfDB_CommandEntry_t       *CmdEntry;
    const EdsLib_IntfDB_GenericTypeMapInfo_t *GenericTypeMapEntry;
    const EdsLib_IntfDB_ArgumentEntry_t      *ArgumentEntry;
    const EdsLib_DataTypeDB_Entry_t          *DataTypeEntry;

    EdsLib_DatabaseRef_t        CmdRefObj;
    EdsLib_DatabaseRef_t        IntfRefObj;
    const EdsLib_DatabaseRef_t *ArgRef;
    uint16_t                    j;
    uint16_t                    k;
    int32_t                     Status;

    if (IdBuffer == NULL && NumIdBufs > 0)
    {
        /* This is a caller error */
        return EDSLIB_FAILURE;
    }

    Status = EDSLIB_INVALID_INDEX;
    EdsLib_Decode_StructId(&CmdRefObj, CmdEdsId);
    EdsLib_Decode_StructId(&IntfRefObj, CompIntfEdsId);

    IntfEntry = EdsLib_IntfDB_GetComponentInterfaceEntry(GD, &IntfRefObj, NULL);
    CmdEntry  = EdsLib_IntfDB_GetDeclIntfCommandEntry(GD, &CmdRefObj, NULL);
    if (IntfEntry != NULL && CmdEntry != NULL)
    {
        for (j = 0; j < CmdEntry->ArgumentCount; ++j)
        {
            ArgumentEntry = &CmdEntry->ArgumentList[j];
            ArgRef        = &ArgumentEntry->RefObj;

            /* If this is a generic, then try to map it to a concrete type */
            DataTypeEntry = EdsLib_DataTypeDB_GetEntry(GD, ArgRef);
            if (DataTypeEntry != NULL && DataTypeEntry->BasicType == EDSLIB_BASICTYPE_GENERIC)
            {
                for (k = 0; k < IntfEntry->GenericTypeMapCount; ++k)
                {
                    GenericTypeMapEntry = &IntfEntry->GenericTypeMapList[k];

                    if (EdsLib_DatabaseRef_IsEqual(&GenericTypeMapEntry->GenTypeRef, ArgRef))
                    {
                        ArgRef = &GenericTypeMapEntry->ActualTypeRef;
                        break;
                    }
                }

                if (k == IntfEntry->GenericTypeMapCount)
                {
                    /* failed to map the generic type to a real type */
                    Status = EDSLIB_TYPE_MAP_FAILED;
                    break;
                }
            }

            if (j < NumIdBufs)
            {
                IdBuffer[j] = EdsLib_Encode_StructId(ArgRef);
            }
            else
            {
                /* the supplied buffer is too small */
                Status = EDSLIB_BUFFER_SIZE_ERROR;
                break;
            }
        }

        if (j == CmdEntry->ArgumentCount)
        {
            Status = EDSLIB_SUCCESS;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindAllCommands(const EdsLib_DatabaseObject_t *GD,
                                      EdsLib_Id_t                    IntfId,
                                      EdsLib_Id_t                   *IdBuffer,
                                      size_t                         NumIdBufs)
{
    const EdsLib_IntfDB_DeclIntfEntry_t *DeclIntfEntry;
    EdsLib_DatabaseRef_t                 RefObj;
    uint16_t                             i;
    int32_t                              Status;

    if (IdBuffer == NULL && NumIdBufs > 0)
    {
        /* This is a caller error */
        return EDSLIB_FAILURE;
    }

    Status = EDSLIB_INVALID_INDEX;
    EdsLib_Decode_StructId(&RefObj, IntfId);

    DeclIntfEntry = EdsLib_IntfDB_GetDeclIntfEntry(GD, &RefObj);
    if (DeclIntfEntry != NULL)
    {
        for (i = 0; i < DeclIntfEntry->CommandCount && i < NumIdBufs; ++i)
        {
            RefObj.SubIndex = 1 + DeclIntfEntry->FirstIdx + i;

            IdBuffer[i] = EdsLib_Encode_StructId(&RefObj);
        }

        if (i == DeclIntfEntry->CommandCount)
        {
            Status = EDSLIB_SUCCESS;
        }
        else
        {
            /* the supplied buffer is too small */
            Status = EDSLIB_BUFFER_SIZE_ERROR;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib public API function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t
EdsLib_IntfDB_GetFullName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *BufferPtr, size_t BufferLen)
{
    int32_t                     Status;
    EdsLib_IntfDB_FullContext_t Ctxt;
    EdsLib_DatabaseRef_t        RefObj;
    char                       *CurrPtr;
    size_t                      LengthRemain;
    int                         PrintLen;

    EdsLib_Decode_StructId(&RefObj, EdsId);
    Status = EdsLib_IntfDB_FindFullContext(GD, &RefObj, &Ctxt);

    do
    {
        if (Status != EDSLIB_SUCCESS)
        {
            break;
        }

        if (BufferLen == 0 || BufferPtr == NULL)
        {
            Status = EDSLIB_BUFFER_SIZE_ERROR;
            break;
        }

        CurrPtr      = BufferPtr;
        LengthRemain = BufferLen;

        PrintLen = snprintf(CurrPtr, LengthRemain, "%s", EdsLib_GetPackageNameNonNull(GD, RefObj.AppIndex));
        if (PrintLen > LengthRemain)
        {
            Status = EDSLIB_BUFFER_SIZE_ERROR;
            break;
        }

        LengthRemain -= PrintLen;
        CurrPtr      += PrintLen;

        /* Note, only one or two of these pointers can be non-null at the same time */
        if (Ctxt.DeclIntfEntry != NULL)
        {
            PrintLen = snprintf(CurrPtr, LengthRemain, "/%s", Ctxt.DeclIntfEntry->Name);
            if (PrintLen > LengthRemain)
            {
                Status = EDSLIB_BUFFER_SIZE_ERROR;
                break;
            }
            LengthRemain -= PrintLen;
            CurrPtr      += PrintLen;
        }
        if (Ctxt.CompEntry != NULL)
        {
            PrintLen = snprintf(CurrPtr, LengthRemain, "/%s", Ctxt.CompEntry->Name);
            if (PrintLen > LengthRemain)
            {
                Status = EDSLIB_BUFFER_SIZE_ERROR;
                break;
            }
            LengthRemain -= PrintLen;
            CurrPtr      += PrintLen;
        }
        if (Ctxt.CmdEntry != NULL)
        {
            PrintLen = snprintf(CurrPtr, LengthRemain, "/%s", Ctxt.CmdEntry->Name);
            if (PrintLen > LengthRemain)
            {
                Status = EDSLIB_BUFFER_SIZE_ERROR;
                break;
            }
            LengthRemain -= PrintLen;
            CurrPtr      += PrintLen;
        }
        if (Ctxt.CompIntfEntry != NULL)
        {
            PrintLen = snprintf(CurrPtr, LengthRemain, "/%s", Ctxt.CompIntfEntry->Name);
            if (PrintLen > LengthRemain)
            {
                Status = EDSLIB_BUFFER_SIZE_ERROR;
                break;
            }
            LengthRemain -= PrintLen;
            CurrPtr      += PrintLen;
        }
    } while (false);

    return Status;
}
