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
 * \file     cfe_missionlib_runtime.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * In typical ground/debug applications the entire EDS of the mission may be
 * statically linked into the executable using a special object named "MISSION_EDS"
 *
 * This header only provides an "extern" declaration for such an object.  It does not
 * define the object; that is up to the user application to do.
 */

#ifndef _CFE_MISSIONLIB_RUNTIME_H_
#define _CFE_MISSIONLIB_RUNTIME_H_

#include "cfe_mission_eds_parameters.h"
#include "cfe_sb_eds_datatypes.h"
#include "cfe_hdr_eds_datatypes.h"

/*
 * This code is intended to support both versions of the header, but
 * tuned at compile time via some ifdef macros to account for the difference.
 *
 * NOTE: It is sometimes required to do source selection at compile-time due
 * to differences in how the structures are defined (i.e. to avoid code that
 * references a field that does not exist in this config).
 *
 * These numeric values correlate with the CCSDS primary header version
 * field +1 (this avoids zero, since undefined macros may evaluate as 0).
 */
#define CFE_MISSIONLIB_SpacePacketBasic_HEADERS 1
#define CFE_MISSIONLIB_SpacePacketApidQ_HEADERS 2

#define CFE_MISSIONLIB_CFGSYM_TO_HDRTYPE(hdr) CFE_MISSIONLIB_##hdr##_HEADERS
#define CFE_MISSIONLIB_HDRTYPE(sym)           CFE_MISSIONLIB_CFGSYM_TO_HDRTYPE(sym)

/*
 * This definition of CFE_MISSIONLIB_SELECTED_HDRTYPE preprocessor macro yields a single
 * string that should resolve to either "CCSDS_SpacePacketBasic_VERSION" or
 * "CCSDS_SpacePacketApidQ_VERSION" depending on which is selected in
 * the mission configuration EDS file.
 *
 * When used in a numeric context, it resolves to the CCSDS version field +1.
 */
#define CFE_MISSIONLIB_SELECTED_HDRTYPE CFE_MISSIONLIB_HDRTYPE(EdsParam_CFE_MISSION_MSG_HEADER_TYPE)

#ifdef __cplusplus
extern "C"
{
#endif

    bool CFE_MissionLib_MapListenerComponent(EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Output,
                                             const EdsComponent_CFE_SB_Listener_t     *Input);
    bool CFE_MissionLib_UnmapListenerComponent(EdsComponent_CFE_SB_Listener_t                 *Output,
                                               const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Input);
    bool CFE_MissionLib_MapPublisherComponent(EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Output,
                                              const EdsComponent_CFE_SB_Publisher_t    *Input);
    bool CFE_MissionLib_UnmapPublisherComponent(EdsComponent_CFE_SB_Publisher_t                *Output,
                                                const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Input);

    bool CFE_MissionLib_PubSub_IsListenerComponent(const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Params);
    bool CFE_MissionLib_PubSub_IsPublisherComponent(const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Params);

    void CFE_MissionLib_Get_PubSub_Parameters(EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Params,
                                              const EdsDataType_CFE_HDR_Message_t      *Packet);
    void CFE_MissionLib_Set_PubSub_Parameters(EdsDataType_CFE_HDR_Message_t                  *Packet,
                                              const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *Params);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CFE_MISSIONLIB_RUNTIME_H_ */
