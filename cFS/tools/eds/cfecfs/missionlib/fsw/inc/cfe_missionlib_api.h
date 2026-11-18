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
 * \file     cfe_missionlib_api.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Defines a runtime API for CFE to obtain information about
 * the software bus interfaces/routing and correlate with EDS.
 */

#ifndef CFE_MISSIONLIB_API_H
#define CFE_MISSIONLIB_API_H

/*
 * The EDS library uses the fixed-width types from the C99 stdint.h header file,
 * rather than the OSAL types.  This allows use in applications that are not
 * based on OSAL.
 */
#include <stdint.h>
#include <stddef.h>

#include <edslib_id.h>
#include <edslib_api_types.h>

/******************************
 * MACROS
 ******************************/

/******************************
 * TYPEDEFS
 ******************************/

/* Note these codes are chosen so that the basic ones (success/failure) are
 * the same as EdsLib, but the more specific error codes do not alias EdsLib errors */
enum
{
    CFE_MISSIONLIB_INVALID_TOPIC     = -105,
    CFE_MISSIONLIB_INVALID_BUFFER    = -104,
    CFE_MISSIONLIB_INVALID_INTERFACE = -103,
    CFE_MISSIONLIB_NOT_IMPLEMENTED   = -102,
    CFE_MISSIONLIB_FAILURE           = -1,
    CFE_MISSIONLIB_SUCCESS           = 0
};

typedef struct CFE_MissionLib_SoftwareBus_Interface CFE_MissionLib_SoftwareBus_Interface_t;

struct CFE_MissionLib_TopicInfo
{
    uint16_t    DispatchStartOffset;
    uint16_t    NumSubcommands;
    EdsLib_Id_t ParentIntfId;
};

typedef struct CFE_MissionLib_TopicInfo CFE_MissionLib_TopicInfo_t;

/**
 * Callback routine for use with topic enumeration
 *
 * @param[inout] Arg       Opaque argument from caller
 * @param[in]    TopicId   the topic ID number
 * @param[in]    IntfEdsId the EDS id of the owner interface
 */
typedef void (*CFE_MissionLib_TopicInfo_Callback_t)(void *Arg, uint16_t TopicId, EdsLib_Id_t IntfEdsId);

/******************************
 * API CALLS
 ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Get the parent EdsLib database object
     *
     * The software bus topic DB contains references to interfaces that are defined
     * in the general EDS database object.  This returns a pointer to that database
     * object.
     *
     * Note that there can be multiple databases in the same system, and EDS ID values
     * are local to the database they came from.  In the case where multiple EDS databases
     * are defined it is important that identifiers are not crossed.
     *
     * @param[in]  SBDB      the software bus database object
     * @return pointer to the EDS general database
     */
    const EdsLib_DatabaseObject_t *CFE_MissionLib_GetParent(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB);

    /**
     * Get the number of topic IDs in the SB interface
     *
     * This returns the size of the lookup table, which reflects the total number
     * of possible topic ids, including those that are undefined
     *
     * @param[in]  SBDB      the software bus database object
     * @return The number of TopicIDs
     */
    uint16_t CFE_MissionLib_GetNumTopics(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB);

    /**
     * Get the number of instance names in the SB interface
     *
     * This returns the size of the instance lookup table, which reflects the total number
     * of CFE instances (aka cpu numbers) from the system build.
     *
     * @param[in]  SBDB      the software bus database object
     * @return The number of Instances
     */
    uint16_t CFE_MissionLib_GetNumInstances(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB);

    /**
     * Get detail about a software bus topic ID
     *
     * This looks up the topic ID in the table and returns the detail information
     * about the that topic, including its parent interface and position in the dispatch table
     *
     * @param[in]  SBDB      the software bus database object
     * @param[in]  TopicId   the topic ID value to query
     * @param[out] TopicInfo output buffer for detail information, if successful
     * @return CFE_MISSIONLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t CFE_MissionLib_GetTopicInfo(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                        uint16_t                                      TopicId,
                                        CFE_MissionLib_TopicInfo_t                   *TopicInfo);

    /**
     * Get the instance name corresponding to an instance number
     *
     * This looks up the instance number and returns the name.  This returns
     * NULL if the instance number has no name associated with it, so the result
     * should not be passed to another API without checking for NULL.
     *
     * @param[in]  SBDB        the software bus database object
     * @param[in]  InstanceNum the instance number to query
     * @return Pointer to Instance name string, or placeholder string if invalid number
     * @retval NULL if there is no name assoicated with the given instance number
     */
    const char *CFE_MissionLib_GetInstanceNameNonNull(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                      uint16_t                                      InstanceNum);

    /**
     * Get the instance name corresponding to an instance number
     *
     * This looks up the instance number and returns the name
     *
     * @note This does not return NULL on failure, it will always return a string pointer,
     * so it can be used directly in printf-style calls.
     *
     * @param[in]  SBDB        the software bus database object
     * @param[in]  InstanceNum the instance number to query
     * @return Pointer to Instance name string, or placeholder string if invalid number
     */
    const char *CFE_MissionLib_GetInstanceNameOrNull(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                     uint16_t                                      InstanceNum);

    /**
     * Get the instance number for an instance name
     *
     * This looks up the instance name and returns the corresponding instance number
     *
     * @note all valid CFE instance numbers are non-zero
     *
     * @param[in]  SBDB      the software bus database object
     * @param[in]  String    the instance name to query
     * @return Instance number, or 0 if the name is not valid
     */
    uint16_t CFE_MissionLib_GetInstanceNumber(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB, const char *String);

    /**
     * Calls a user-supplied function for every topic ID
     *
     * This allows applications to enumerate all known topic IDs in the database, which can in turn be
     * used for providing information to the user about the defined interfaces.
     *
     * @param[in]    SBDB      the software bus database object
     * @param[in]    Callback  the function to call for every topic ID
     * @param[inout] OpaqueArg an argument that is passed to the callback, may be NULL if not needed
     */
    void CFE_MissionLib_EnumerateTopics(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                        CFE_MissionLib_TopicInfo_Callback_t           Callback,
                                        void                                         *OpaqueArg);

    /**
     * Finds a topic ID matching the given interface ID
     *
     * The interface ID value comes from the general EDS database.  This implements a reverse search,
     * to find the topic ID corresponding to the given interface ID.
     *
     * @param[in]  SBDB          the software bus database object
     * @param[in]  IntfEdsId     the interface ID from the general EDS database
     * @param[out] TopicIdBuffer output buffer for topic ID, if successful
     * @return CFE_MISSIONLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t CFE_MissionLib_FindTopicIdFromIntfId(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                 EdsLib_Id_t                                   IntfEdsId,
                                                 uint16_t                                     *TopicIdBuffer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CFE_MISSIONLIB_API_H */
