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
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
EdsLib_IntfDB_t EdsLib_IntfDB_GetTopLevel(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
{
    if (GD == NULL || GD->IntfDB_Table == NULL)
    {
        return NULL;
    }

    if (AppIdx >= GD->AppTableSize)
    {
        return NULL;
    }

    return GD->IntfDB_Table[AppIdx];
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_IntfDB_ComponentEntry_t *EdsLib_IntfDB_GetComponentEntry(const EdsLib_DatabaseObject_t *GD,
                                                                      const EdsLib_DatabaseRef_t    *RefObj)
{
    EdsLib_IntfDB_t                       IntfDB;
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    uint16_t                              SubIndex;

    CompEntry = NULL;
    SubIndex  = RefObj->SubIndex - 1;

    do
    {
        if (RefObj == NULL || RefObj->Qualifier != EdsLib_DbRef_Qualifier_INTERFACE)
        {
            break;
        }

        IntfDB = EdsLib_IntfDB_GetTopLevel(GD, RefObj->AppIndex);
        if (IntfDB == NULL)
        {
            break;
        }

        SubIndex -= IntfDB->DeclIntfListSize;

        if (SubIndex >= IntfDB->ComponentListSize)
        {
            break;
        }

        CompEntry = &IntfDB->ComponentListEntries[SubIndex];
    } while (false);

    return CompEntry;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_IntfDB_DeclIntfEntry_t *EdsLib_IntfDB_GetDeclIntfEntry(const EdsLib_DatabaseObject_t *GD,
                                                                    const EdsLib_DatabaseRef_t    *RefObj)
{
    EdsLib_IntfDB_t                      IntfDB;
    const EdsLib_IntfDB_DeclIntfEntry_t *IntfEntry;
    uint16_t                             SubIndex;

    IntfEntry = NULL;
    SubIndex  = RefObj->SubIndex - 1;

    do
    {
        if (RefObj == NULL || RefObj->Qualifier != EdsLib_DbRef_Qualifier_INTERFACE)
        {
            break;
        }

        IntfDB = EdsLib_IntfDB_GetTopLevel(GD, RefObj->AppIndex);
        if (IntfDB == NULL)
        {
            break;
        }

        if (SubIndex >= IntfDB->DeclIntfListSize)
        {
            break;
        }

        IntfEntry = &IntfDB->DeclIntfListEntries[SubIndex];
    } while (false);

    return IntfEntry;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_IntfDB_InterfaceEntry_t *
EdsLib_IntfDB_GetInterfaceEntryFromCompIdx(const EdsLib_IntfDB_ComponentEntry_t *CompEntry, uint16_t CompIdx)
{
    const EdsLib_IntfDB_InterfaceEntry_t *IntfEntry;
    uint16_t                              LocalIdx;

    IntfEntry = NULL;
    LocalIdx  = CompIdx - CompEntry->FirstIdx - 1;
    if (LocalIdx < CompEntry->ProvidedIntfCount)
    {
        IntfEntry = &CompEntry->ProvidedIntfList[LocalIdx];
    }
    else
    {
        LocalIdx -= CompEntry->ProvidedIntfCount;
        if (LocalIdx < CompEntry->RequiredIntfCount)
        {
            IntfEntry = &CompEntry->RequiredIntfList[LocalIdx];
        }
    }

    /* This will be null if it was out of range */
    return IntfEntry;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_IntfDB_CommandEntry_t *
EdsLib_IntfDB_GetCommandEntryFromIntfIdx(const EdsLib_IntfDB_DeclIntfEntry_t *IntfEntry, uint16_t IntfIdx)
{
    const EdsLib_IntfDB_CommandEntry_t *CmdEntry;
    uint16_t                            LocalIdx;

    CmdEntry = NULL;
    LocalIdx = IntfIdx - IntfEntry->FirstIdx - 1;
    if (LocalIdx < IntfEntry->CommandCount)
    {
        CmdEntry = &IntfEntry->CommandList[LocalIdx];
    }

    /* This will be null if it was out of range */
    return CmdEntry;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_IntfDB_InterfaceEntry_t *
EdsLib_IntfDB_GetComponentInterfaceEntry(const EdsLib_DatabaseObject_t         *GD,
                                         const EdsLib_DatabaseRef_t            *RefObj,
                                         const EdsLib_IntfDB_ComponentEntry_t **CompEntryOut)
{
    EdsLib_IntfDB_t                       IntfDB;
    const EdsLib_IntfDB_InterfaceEntry_t *IntfEntry;
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    uint16_t                              i;

    IntfEntry = NULL;
    CompEntry = NULL;

    do
    {
        if (RefObj == NULL || RefObj->Qualifier != EdsLib_DbRef_Qualifier_INTERFACE)
        {
            break;
        }

        IntfDB = EdsLib_IntfDB_GetTopLevel(GD, RefObj->AppIndex);
        if (IntfDB == NULL)
        {
            break;
        }

        for (i = 0; i < IntfDB->ComponentListSize; ++i)
        {
            CompEntry = &IntfDB->ComponentListEntries[i];
            IntfEntry = EdsLib_IntfDB_GetInterfaceEntryFromCompIdx(CompEntry, RefObj->SubIndex);

            if (IntfEntry != NULL)
            {
                if (CompEntryOut != NULL)
                {
                    *CompEntryOut = CompEntry;
                }
                break;
            }
        }
    } while (false);

    return IntfEntry;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
const EdsLib_IntfDB_CommandEntry_t *
EdsLib_IntfDB_GetDeclIntfCommandEntry(const EdsLib_DatabaseObject_t        *GD,
                                      const EdsLib_DatabaseRef_t           *RefObj,
                                      const EdsLib_IntfDB_DeclIntfEntry_t **DeclIntfOut)
{
    EdsLib_IntfDB_t                      IntfDB;
    const EdsLib_IntfDB_DeclIntfEntry_t *IntfEntry;
    const EdsLib_IntfDB_CommandEntry_t  *CmdEntry;
    uint16_t                             i;

    IntfEntry = NULL;
    CmdEntry  = NULL;

    do
    {
        if (RefObj == NULL || RefObj->Qualifier != EdsLib_DbRef_Qualifier_INTERFACE)
        {
            break;
        }

        IntfDB = EdsLib_IntfDB_GetTopLevel(GD, RefObj->AppIndex);
        if (IntfDB == NULL)
        {
            break;
        }

        for (i = 0; i < IntfDB->DeclIntfListSize; ++i)
        {
            IntfEntry = &IntfDB->DeclIntfListEntries[i];
            CmdEntry  = EdsLib_IntfDB_GetCommandEntryFromIntfIdx(IntfEntry, RefObj->SubIndex);

            if (CmdEntry != NULL)
            {
                if (DeclIntfOut != NULL)
                {
                    *DeclIntfOut = IntfEntry;
                }
                break;
            }
        }
    } while (false);

    return CmdEntry;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindComponentBySubstring(EdsLib_IntfDB_t IntfDBPtr,
                                               const char     *CompName,
                                               size_t          MatchLen,
                                               uint16_t       *IdxOut)
{
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    int32_t                               Status;
    uint16_t                              i;

    Status = EDSLIB_NAME_NOT_FOUND;

    for (i = 0; i < IntfDBPtr->ComponentListSize; ++i)
    {
        CompEntry = &IntfDBPtr->ComponentListEntries[i];

        if (CompEntry->Name != NULL && strlen(CompEntry->Name) == MatchLen
            && memcmp(CompEntry->Name, CompName, MatchLen) == 0)
        {
            *IdxOut = 1 + IntfDBPtr->DeclIntfListSize + i;
            Status  = EDSLIB_SUCCESS;
            break;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindDeclIntfBySubstring(EdsLib_IntfDB_t IntfDBPtr,
                                              const char     *IntfName,
                                              size_t          MatchLen,
                                              uint16_t       *IdxOut)
{
    const EdsLib_IntfDB_DeclIntfEntry_t *DeclIntfEntry;
    int32_t                              Status;
    uint16_t                             i;

    Status = EDSLIB_NAME_NOT_FOUND;

    for (i = 0; i < IntfDBPtr->DeclIntfListSize; ++i)
    {
        DeclIntfEntry = &IntfDBPtr->DeclIntfListEntries[i];

        if (DeclIntfEntry->Name != NULL && strlen(DeclIntfEntry->Name) == MatchLen
            && memcmp(DeclIntfEntry->Name, IntfName, MatchLen) == 0)
        {
            *IdxOut = 1 + i;
            Status  = EDSLIB_SUCCESS;
            break;
        }
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindComponentInterfaceBySubstring(const EdsLib_IntfDB_ComponentEntry_t *CompEntry,
                                                        const char                           *IntfName,
                                                        size_t                                MatchLen,
                                                        uint16_t                             *IdxOut)
{
    const EdsLib_IntfDB_InterfaceEntry_t *IntfEntry;
    uint16_t                              CompIdx;
    uint16_t                              Count;
    int32_t                               Status;

    Status  = EDSLIB_NAME_NOT_FOUND;
    Count   = CompEntry->RequiredIntfCount + CompEntry->ProvidedIntfCount;
    CompIdx = CompEntry->FirstIdx + 1;
    while (Count > 0)
    {
        IntfEntry = EdsLib_IntfDB_GetInterfaceEntryFromCompIdx(CompEntry, CompIdx);

        if (IntfEntry != NULL && IntfEntry->Name != NULL && strlen(IntfEntry->Name) == MatchLen
            && memcmp(IntfEntry->Name, IntfName, MatchLen) == 0)
        {
            *IdxOut = CompIdx;

            Status = EDSLIB_SUCCESS;
            break;
        }

        ++CompIdx;
        --Count;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindDeclIntfByQualifiedName(const EdsLib_DatabaseObject_t *GD,
                                                  const char                    *IntfName,
                                                  size_t                         MatchLen,
                                                  EdsLib_DatabaseRef_t          *RefObj)
{
    EdsLib_IntfDB_t IntfDBPtr;
    const char     *Sep;
    const char     *CurrChunkPtr;
    size_t          CurrChunkLen;
    int32_t         Status;

    memset(RefObj, 0, sizeof(*RefObj));
    RefObj->Qualifier = EdsLib_DbRef_Qualifier_INTERFACE;

    Status    = EDSLIB_NAME_NOT_FOUND;
    IntfDBPtr = NULL;

    /* The fully-qualified SEDS name uses / chars to separate name parts,
     * first is the package name (app), then the component name, then the interface name */
    CurrChunkPtr = IntfName;
    Sep          = memchr(CurrChunkPtr, '/', MatchLen - 1);
    if (Sep != NULL)
    {
        CurrChunkLen = Sep - CurrChunkPtr;

        Status = EdsLib_FindPackageIdxBySubstring(GD, CurrChunkPtr, CurrChunkLen, &RefObj->AppIndex);
        if (Status == EDSLIB_SUCCESS)
        {
            IntfDBPtr = EdsLib_IntfDB_GetTopLevel(GD, RefObj->AppIndex);
        }

        CurrChunkPtr += CurrChunkLen + 1;
        CurrChunkLen  = MatchLen - (CurrChunkPtr - IntfName);
    }

    if (IntfDBPtr != NULL)
    {
        /* Now attempt to find the local component name with the remaining substring */
        Status = EdsLib_IntfDB_FindDeclIntfBySubstring(IntfDBPtr, CurrChunkPtr, CurrChunkLen, &RefObj->SubIndex);
    }
    else
    {
        Status = EDSLIB_NAME_NOT_FOUND;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindComponentByQualifiedName(const EdsLib_DatabaseObject_t *GD,
                                                   const char                    *CompName,
                                                   size_t                         MatchLen,
                                                   EdsLib_DatabaseRef_t          *RefObj)
{
    EdsLib_IntfDB_t IntfDBPtr;
    const char     *Sep;
    const char     *CurrChunkPtr;
    size_t          CurrChunkLen;
    int32_t         Status;

    memset(RefObj, 0, sizeof(*RefObj));
    RefObj->Qualifier = EdsLib_DbRef_Qualifier_INTERFACE;

    Status    = EDSLIB_NAME_NOT_FOUND;
    IntfDBPtr = NULL;

    /* The fully-qualified SEDS name uses / chars to separate name parts,
     * first is the package name (app), then the component name, then the interface name */
    CurrChunkPtr = CompName;
    Sep          = memchr(CurrChunkPtr, '/', MatchLen - 1);
    if (Sep != NULL)
    {
        CurrChunkLen = Sep - CurrChunkPtr;

        Status = EdsLib_FindPackageIdxBySubstring(GD, CurrChunkPtr, CurrChunkLen, &RefObj->AppIndex);
        if (Status == EDSLIB_SUCCESS)
        {
            IntfDBPtr = EdsLib_IntfDB_GetTopLevel(GD, RefObj->AppIndex);
        }

        CurrChunkPtr += CurrChunkLen + 1;
        CurrChunkLen  = MatchLen - (CurrChunkPtr - CompName);
    }

    if (IntfDBPtr != NULL)
    {
        /* Now attempt to find the local component name with the remaining substring */
        Status = EdsLib_IntfDB_FindComponentBySubstring(IntfDBPtr, CurrChunkPtr, CurrChunkLen, &RefObj->SubIndex);
    }
    else
    {
        Status = EDSLIB_NAME_NOT_FOUND;
    }

    return Status;
}

/*----------------------------------------------------------------
 *
 * EdsLib internal function
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32_t EdsLib_IntfDB_FindFullContext(const EdsLib_DatabaseObject_t *GD,
                                      const EdsLib_DatabaseRef_t    *RefObj,
                                      EdsLib_IntfDB_FullContext_t   *CtxtOut)
{
    EdsLib_IntfDB_t                       IntfDBPtr;
    const EdsLib_IntfDB_ComponentEntry_t *CompEntry;
    const EdsLib_IntfDB_DeclIntfEntry_t  *DeclIntfEntry;
    const EdsLib_IntfDB_InterfaceEntry_t *CompIntfEntry;
    const EdsLib_IntfDB_CommandEntry_t   *CmdEntry;

    int32_t  Status;
    uint16_t SubIndex;
    uint16_t i;

    Status        = EDSLIB_INVALID_INDEX;
    IntfDBPtr     = NULL;
    CompEntry     = NULL;
    DeclIntfEntry = NULL;
    CompIntfEntry = NULL;
    CmdEntry      = NULL;

    do
    {
        if (RefObj->Qualifier != EdsLib_DbRef_Qualifier_INTERFACE)
        {
            break;
        }
        IntfDBPtr = EdsLib_IntfDB_GetTopLevel(GD, RefObj->AppIndex);
        if (IntfDBPtr == NULL)
        {
            break;
        }
        SubIndex = RefObj->SubIndex - 1;
        if (SubIndex < IntfDBPtr->DeclIntfListSize)
        {
            /* It directly refers to a declared interface */
            DeclIntfEntry = &IntfDBPtr->DeclIntfListEntries[SubIndex];
            Status        = EDSLIB_SUCCESS;
            break;
        }
        SubIndex -= IntfDBPtr->DeclIntfListSize;
        if (SubIndex < IntfDBPtr->ComponentListSize)
        {
            /* It directly refers to a component */
            CompEntry = &IntfDBPtr->ComponentListEntries[SubIndex];
            Status    = EDSLIB_SUCCESS;
            break;
        }

        /* If we get here then it is a virtual index, could be under decl or comp,
         * Check the first component for a hint of which list to check */
        SubIndex = RefObj->SubIndex;
        if (IntfDBPtr->ComponentListSize > 0 && SubIndex <= IntfDBPtr->ComponentListEntries[0].FirstIdx)
        {
            /* It must be a command under a declared interface */
            for (i = 0; i < IntfDBPtr->DeclIntfListSize; ++i)
            {
                DeclIntfEntry = &IntfDBPtr->DeclIntfListEntries[i];
                CmdEntry      = EdsLib_IntfDB_GetCommandEntryFromIntfIdx(DeclIntfEntry, SubIndex);

                if (CmdEntry != NULL)
                {
                    Status = EDSLIB_SUCCESS;
                    break;
                }

                DeclIntfEntry = NULL;
            }
        }
        else
        {
            /* It must be a utilized interface under a component */
            for (i = 0; i < IntfDBPtr->ComponentListSize; ++i)
            {
                CompEntry     = &IntfDBPtr->ComponentListEntries[i];
                CompIntfEntry = EdsLib_IntfDB_GetInterfaceEntryFromCompIdx(CompEntry, SubIndex);

                if (CompIntfEntry != NULL)
                {
                    Status = EDSLIB_SUCCESS;
                    break;
                }

                CompEntry = NULL;
            }
        }
    } while (false);

    CtxtOut->IntfDBPtr     = IntfDBPtr;
    CtxtOut->CompEntry     = CompEntry;
    CtxtOut->DeclIntfEntry = DeclIntfEntry;
    CtxtOut->CompIntfEntry = CompIntfEntry;
    CtxtOut->CmdEntry      = CmdEntry;

    return Status;
}
