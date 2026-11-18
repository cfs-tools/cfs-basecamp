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
 * Auto-Generated stub implementations for functions defined in cfe_missionlib_api header
 */

#include "cfe_missionlib_api.h"
#include "utgenstub.h"

void UT_DefaultHandler_CFE_MissionLib_FindTopicIdFromIntfId(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_CFE_MissionLib_GetInstanceNumber(void *, UT_EntryKey_t, const UT_StubContext_t *);
void UT_DefaultHandler_CFE_MissionLib_GetTopicInfo(void *, UT_EntryKey_t, const UT_StubContext_t *);

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_EnumerateTopics()
 * ----------------------------------------------------
 */
void CFE_MissionLib_EnumerateTopics(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                    CFE_MissionLib_TopicInfo_Callback_t           Callback,
                                    void                                         *OpaqueArg)
{
    UT_GenStub_AddParam(CFE_MissionLib_EnumerateTopics, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);
    UT_GenStub_AddParam(CFE_MissionLib_EnumerateTopics, CFE_MissionLib_TopicInfo_Callback_t, Callback);
    UT_GenStub_AddParam(CFE_MissionLib_EnumerateTopics, void *, OpaqueArg);

    UT_GenStub_Execute(CFE_MissionLib_EnumerateTopics, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_FindTopicIdFromIntfId()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_FindTopicIdFromIntfId(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                             EdsLib_Id_t                                   IntfEdsId,
                                             uint16_t                                     *TopicIdBuffer)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_FindTopicIdFromIntfId, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_FindTopicIdFromIntfId, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);
    UT_GenStub_AddParam(CFE_MissionLib_FindTopicIdFromIntfId, EdsLib_Id_t, IntfEdsId);
    UT_GenStub_AddParam(CFE_MissionLib_FindTopicIdFromIntfId, uint16_t *, TopicIdBuffer);

    UT_GenStub_Execute(CFE_MissionLib_FindTopicIdFromIntfId,
                       Basic,
                       UT_DefaultHandler_CFE_MissionLib_FindTopicIdFromIntfId);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_FindTopicIdFromIntfId, int32_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetInstanceNameNonNull()
 * ----------------------------------------------------
 */
const char *CFE_MissionLib_GetInstanceNameNonNull(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                  uint16_t                                      InstanceNum)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetInstanceNameNonNull, const char *);

    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNameNonNull, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);
    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNameNonNull, uint16_t, InstanceNum);

    UT_GenStub_Execute(CFE_MissionLib_GetInstanceNameNonNull, Basic, NULL);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetInstanceNameNonNull, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetInstanceNameOrNull()
 * ----------------------------------------------------
 */
const char *CFE_MissionLib_GetInstanceNameOrNull(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                 uint16_t                                      InstanceNum)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetInstanceNameOrNull, const char *);

    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNameOrNull, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);
    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNameOrNull, uint16_t, InstanceNum);

    UT_GenStub_Execute(CFE_MissionLib_GetInstanceNameOrNull, Basic, NULL);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetInstanceNameOrNull, const char *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetInstanceNumber()
 * ----------------------------------------------------
 */
uint16_t CFE_MissionLib_GetInstanceNumber(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB, const char *String)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetInstanceNumber, uint16_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNumber, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);
    UT_GenStub_AddParam(CFE_MissionLib_GetInstanceNumber, const char *, String);

    UT_GenStub_Execute(CFE_MissionLib_GetInstanceNumber, Basic, UT_DefaultHandler_CFE_MissionLib_GetInstanceNumber);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetInstanceNumber, uint16_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetNumInstances()
 * ----------------------------------------------------
 */
uint16_t CFE_MissionLib_GetNumInstances(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetNumInstances, uint16_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetNumInstances, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);

    UT_GenStub_Execute(CFE_MissionLib_GetNumInstances, Basic, NULL);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetNumInstances, uint16_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetNumTopics()
 * ----------------------------------------------------
 */
uint16_t CFE_MissionLib_GetNumTopics(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetNumTopics, uint16_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetNumTopics, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);

    UT_GenStub_Execute(CFE_MissionLib_GetNumTopics, Basic, NULL);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetNumTopics, uint16_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetParent()
 * ----------------------------------------------------
 */
const EdsLib_DatabaseObject_t *CFE_MissionLib_GetParent(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetParent, const EdsLib_DatabaseObject_t *);

    UT_GenStub_AddParam(CFE_MissionLib_GetParent, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);

    UT_GenStub_Execute(CFE_MissionLib_GetParent, Basic, NULL);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetParent, const EdsLib_DatabaseObject_t *);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_GetTopicInfo()
 * ----------------------------------------------------
 */
int32_t CFE_MissionLib_GetTopicInfo(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                    uint16_t                                      TopicId,
                                    CFE_MissionLib_TopicInfo_t                   *TopicInfo)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_GetTopicInfo, int32_t);

    UT_GenStub_AddParam(CFE_MissionLib_GetTopicInfo, const CFE_MissionLib_SoftwareBus_Interface_t *, SBDB);
    UT_GenStub_AddParam(CFE_MissionLib_GetTopicInfo, uint16_t, TopicId);
    UT_GenStub_AddParam(CFE_MissionLib_GetTopicInfo, CFE_MissionLib_TopicInfo_t *, TopicInfo);

    UT_GenStub_Execute(CFE_MissionLib_GetTopicInfo, Basic, UT_DefaultHandler_CFE_MissionLib_GetTopicInfo);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_GetTopicInfo, int32_t);
}
