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
 * \file     edslib_displaydb_lookup.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Public API functions for dealing with the EDS-defined "Mission-Unique"
 * message identifiers.  This includes:
 *  - looking up a descriptive name for a message id
 *  - converting a user-supplied string to a message id
 *
 * This is linked as part of the "full" EDS library
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "edslib_internal.h"

EdsLib_DisplayDB_t EdsLib_DisplayDB_GetTopLevel(const EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
{
   /*
    * As name tables are dependent on data tables, check that data table is also present
    */
   if (GD->DisplayDB_Table == NULL)
   {
      return NULL;
   }

   if (AppIdx >= GD->AppTableSize)
   {
      return NULL;
   }

   return GD->DisplayDB_Table[AppIdx];
}

const EdsLib_DisplayDB_Entry_t *EdsLib_DisplayDB_GetEntry(const EdsLib_DatabaseObject_t *GD, const EdsLib_DatabaseRef_t *RefObj)
{
   EdsLib_DisplayDB_t NameDict;
   EdsLib_DataTypeDB_t DataDict;

   if (RefObj == NULL)
   {
       return NULL;
   }

   DataDict = EdsLib_DataTypeDB_GetTopLevel(GD, RefObj->AppIndex);
   if (DataDict == NULL)
   {
       return NULL;
   }

   NameDict = EdsLib_DisplayDB_GetTopLevel(GD, RefObj->AppIndex);
   if (NameDict == NULL)
   {
       return NULL;
   }

   if (NameDict->DisplayInfoTable == NULL ||
           RefObj->TypeIndex >= DataDict->DataTypeTableSize)
   {
       return NULL;
   }

   return &NameDict->DisplayInfoTable[RefObj->TypeIndex];
}

