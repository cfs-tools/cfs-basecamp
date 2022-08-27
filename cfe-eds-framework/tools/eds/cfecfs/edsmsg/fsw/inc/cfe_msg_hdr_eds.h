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

/**
 * @file
 *
 * Define cFS standard full header
 *  - Avoid direct access for portability, use APIs
 *  - Used to construct message structures
 */

#ifndef CFE_MSG_HDR_EDS_H
#define CFE_MSG_HDR_EDS_H

/*
 * Include Files
 */

#include "common_types.h"
#include "cfe_msg_api_typedefs.h"

/* In this build the structure definitions come from EDS */
#include "cfe_hdr_eds_typedefs.h"

/*
 * Macro Definitions
 */

/**
 * \brief Convert from a CFE_MSG_TelemetryHeader_t or CFE_MSG_CommandHeader_t to a CFE_MSG_Message_t
 *
 * \par Description
 *       Given a pointer to a secondary header structure, return a pointer
 *       to the CFE_MSG_Message_t (base header/primary header) object.
 *
 * \par Notes
 *       Implemented as a macro, so it should work with both Command and Telemetry headers.
 *
 *       In the EDS version it goes through a "void*" type, so as to not trigger alias warnings, however
 *       all MSG-related structs must all begin with an actual instance of CFE_HDR_Message_t for
 *       this to be valid/safe.  All EDS-generated message structures meet this requirement because
 *       they all (eventually) derive from this base type, but any hand-written struct could be incorrect,
 *       and unfortunately will not trigger an error when using void* cast.
 *
 */
#define CFE_MSG_PTR(shdr) ((void *)(&(shdr)))

/*
 * Type Definitions
 */

/**********************************************************************
 * Structure definitions for full header
 *
 * These are based on historically supported definitions and not
 * an open source standard.
 */

/**
 * \brief cFS generic base message
 *
 * This provides the definition of CFE_MSG_Message_t
 */
union CFE_MSG_Message
{
    /**
     * EDS-defined base message structure
     */
    CFE_HDR_Message_t BaseMsg;

    /**
     * \brief Byte level access
     */
    uint8 Byte[sizeof(CFE_HDR_Message_t)];
};

/**
 * \brief cFS command header
 *
 * This provides the definition of CFE_MSG_CommandHeader_t
 */
struct CFE_MSG_CommandHeader
{
    /**
     * EDS-defined command header structure
     */
    CFE_HDR_CommandHeader_t HeaderData;
};

/**
 * \brief cFS telemetry header
 *
 * This provides the definition of CFE_MSG_TelemetryHeader_t
 */
struct CFE_MSG_TelemetryHeader
{
    /**
     * EDS-defined telemetry header structure
     */
    CFE_HDR_TelemetryHeader_t HeaderData;
};

/**
 * Helper function to cast an aribtrary base pointer to a CFE_MSG_Message_t* for use with SB APIs
 */
static inline CFE_MSG_Message_t *CFE_MSG_CastBaseMsg(void *BaseMsg)
{
    /*
     * In a pedantic sense, this is "promoting" the base message pointer to the
     * union type.  This should not be an actual violation though, as the union
     * contains a CFE_HDR_Message_t member.
     */
    return ((CFE_MSG_Message_t *)BaseMsg);
}

#endif /* CFE_MSG_HDR_EDS_H */
