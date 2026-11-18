/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/*
** File   : cfe_sb_eds.h
**
** Author : Joe Hickey
**
** Purpose:
**      API for the Electronic Data Sheets (EDS) for software bus messages
**
**      Contains prototypes for the generic pack, unpack, and dispatch routines
**      that are part of the Software Bus (SB) application.  These are the EDS
**      API calls that actually work with instances of message objects, which
**      are all some form of CCSDS_SpacePacket_t (the base type of all SB messages).
**
**      This is a separate header file as these API calls are only available in
**      an EDS-enabled build.
**
**  Note - this public header file does not include any "EdsLib" headers directly...
**  The SB EDS implementation abstracts the relevant EdsLib data types so that
**  CFS applications are only dealing with the SB API directly, not going directly to
**  EdsLib to perform data operations.  This keeps the include paths simpler and
**  eases migration.
**
*/

#ifndef EDSMSG_DISPATCHER_H
#define EDSMSG_DISPATCHER_H

#include <common_types.h>
#include <cfe_sb_api_typedefs.h>
#include <cfe_mission_eds_parameters.h>
#include <edslib_id.h>

/* It is an error to include this file in a non-EDS build (the function does not exist) */
#ifndef CFE_EDS_ENABLED
#error "Cannot use EDS dispatcher in a non-EDS build"
#endif

/**
 * \brief Dispatch an incoming message to its handler function, given a dispatch table.
 *
 * The incoming message will be identified and matched against the given
 * EDS-defined interface.
 *
 * If the message payload is defined by the application's EDS, the correct
 * handler function in the dispatch table will be called.
 *
 * \note this function is generally not directly invoked from applications, it
 * should be invoked through a wrapper generated from the EDS tool.  The wrapper
 * has a simpler API and ensures that the ID values passed in are correct.
 *
 * \param DeclIntfId    The declared interface ID from EDS
 * \param ComponentId   The component ID from EDS
 * \param Buffer        Pointer to the Software Bus buffer (with headers)
 * \param DispatchTable Pointer to the dispatch table
 * \returns If successfully dispatched, returns code from handler function, otherwise an error code.
 */
CFE_Status_t CFE_EDSMSG_Dispatch(EdsLib_Id_t            DeclIntfId,
                                 EdsLib_Id_t            ComponentId,
                                 const CFE_SB_Buffer_t *Buffer,
                                 const void            *DispatchTable);

#endif /* edsmsg_dispatcher_H */
