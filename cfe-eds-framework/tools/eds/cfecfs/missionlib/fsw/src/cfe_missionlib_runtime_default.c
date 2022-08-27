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
 * \file     cfe_missionlib_runtime_default.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Provides the conversions between CFE software bus abstract "MsgId" values
 * and the actual values used inside message headers.  This is actually compiled
 * a small self contained library such that it can be linked easily into both
 * CFE flight software as well as ground tools and utilities that also need to
 * interpret or generate software bus messages.
 *
 * Provided that everything links to this library and therefore uses the same
 * conversion logic, everything should stay synchronized all the time even if
 * the logic needs to change at some point.
 *
 * This also allows the logic to be customized per-mission.
 *
 * The default logic is to perform a bitwise shift and mapping, but this is
 * not required.  It can be arbitrary, e.g. a table lookup if necessary.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ccsds_spacepacket_eds_typedefs.h"
#include "cfe_sb_eds_typedefs.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_missionlib_runtime.h"

#define CFE_MISSIONLIB_GETSET_MSGID_BITS(hdr, action, ...) CFE_MissionLib_##hdr##_##action(__VA_ARGS__)

#define CFE_MISSIONLIB_GET_MSGID_BITS(hdr, mid, pkt) CFE_MISSIONLIB_GETSET_MSGID_BITS(hdr, BitsToMsgId, mid, pkt)
#define CFE_MISSIONLIB_SET_MSGID_BITS(hdr, pkt, mid) CFE_MISSIONLIB_GETSET_MSGID_BITS(hdr, BitsFromMsgId, pkt, mid)

/*
 * NOTE -
 * These bit masks/shifts correlate with the bits in the first word of the
 * CCSDS spacepacket header, for historical/backward compatibility reasons.
 * This is just in case some existing code was assuming this relationship.
 *
 * There is no requirement that the MsgId bits match the CCSDS spacepacket
 * bits for either EDS or software bus.
 */

/* Primary header type bits - always used */
#define CFE_MISSIONLIB_MSGID_TYPE_MASK  0x3 /**< Bit mask to get the CMD/TLM type from MsgId Value */
#define CFE_MISSIONLIB_MSGID_TYPE_SHIFT 11  /**< Bit shift to get the CMD/TLM type from MsgId Value */

#define CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS \
    0x3 /**< Fixed value to set in MsgId for command packets (LSB-justified) */
#define CFE_MISSIONLIB_MSGID_TELEMETRY_BITS \
    0x1 /**< Fixed value to set in MsgId for telemetry packets (LSB-justified) */

/* When using only basic (v1) headers, the TopicID + Subsystem are squeezed into the historical 11-bit field */
#if CFE_MISSIONLIB_SELECTED_HDRTYPE == CFE_MISSIONLIB_SpacePacketBasic_HEADERS

#define CFE_MISSIONLIB_MSGID_APID_MASK    0x003F /**< Bit mask to get the APID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_APID_SHIFT   0      /**< Bit shift to get the APID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SUBSYS_MASK  0x001F /**< Bit mask to get the Subsystem ID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SUBSYS_SHIFT 6      /**< Bit shift to get the Subsystem ID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SYS_MASK     0      /**< Bit mask to get the System ID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SYS_SHIFT    0      /**< Bit shift to get the System ID from MsgId Value */

/* When using extended (v2) headers, more of the MsgId bits can be used */
#elif CFE_MISSIONLIB_SELECTED_HDRTYPE == CFE_MISSIONLIB_SpacePacketApidQ_HEADERS

/* Extended MsgId information - usable when extended headers are enabled, should be all 0 otherwise */
#define CFE_MISSIONLIB_MSGID_APID_MASK    0x07FF /**< Bit mask to get the APID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_APID_SHIFT   0      /**< Bit shift to get the APID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SUBSYS_MASK  0x00FF /**< Bit mask to get the Subsystem ID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SUBSYS_SHIFT 16     /**< Bit shift to get the Subsystem ID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SYS_MASK     0x007F /**< Bit mask to get the System ID from MsgId Value */
#define CFE_MISSIONLIB_MSGID_SYS_SHIFT    24     /**< Bit shift to get the System ID from MsgId Value */

#else

/* Need to define how to map the bits here */
#error "No MsgId bitfield mappings defined for current header type"

#endif

static inline uint32_t CFE_MissionLib_GetMsgIdApid(const CFE_SB_MsgId_t *MsgId)
{
    return (MsgId->Value >> CFE_MISSIONLIB_MSGID_APID_SHIFT) & CFE_MISSIONLIB_MSGID_APID_MASK;
}

static inline uint32_t CFE_MissionLib_GetMsgIdInterfaceType(const CFE_SB_MsgId_t *MsgId)
{
    return (MsgId->Value >> CFE_MISSIONLIB_MSGID_TYPE_SHIFT) & CFE_MISSIONLIB_MSGID_TYPE_MASK;
}

static inline void CFE_MissionLib_SetMsgIdSubsystem(CFE_SB_MsgId_t *MsgId, uint32_t Val)
{
    MsgId->Value |= (Val & CFE_MISSIONLIB_MSGID_SUBSYS_MASK) << CFE_MISSIONLIB_MSGID_SUBSYS_SHIFT;
}

static inline uint32_t CFE_MissionLib_GetMsgIdSubsystem(const CFE_SB_MsgId_t *MsgId)
{
    return (MsgId->Value >> CFE_MISSIONLIB_MSGID_SUBSYS_SHIFT) & CFE_MISSIONLIB_MSGID_SUBSYS_MASK;
}

static inline void CFE_MissionLib_SetMsgIdSystem(CFE_SB_MsgId_t *MsgId, uint32_t Val)
{
    MsgId->Value |= (Val & CFE_MISSIONLIB_MSGID_SYS_MASK) << CFE_MISSIONLIB_MSGID_SYS_SHIFT;
}

static inline uint32_t CFE_MissionLib_GetMsgIdSystem(const CFE_SB_MsgId_t *MsgId)
{
    return (MsgId->Value >> CFE_MISSIONLIB_MSGID_SYS_SHIFT) & CFE_MISSIONLIB_MSGID_SYS_MASK;
}

static inline void CFE_MissionLib_SetMsgIdApid(CFE_SB_MsgId_t *MsgId, uint32_t Val)
{
    MsgId->Value |= (Val & CFE_MISSIONLIB_MSGID_APID_MASK) << CFE_MISSIONLIB_MSGID_APID_SHIFT;
}

static inline void CFE_MissionLib_SetMsgIdInterfaceType(CFE_SB_MsgId_t *MsgId, uint32_t Val)
{
    MsgId->Value |= (Val & CFE_MISSIONLIB_MSGID_TYPE_MASK) << CFE_MISSIONLIB_MSGID_TYPE_SHIFT;
}

/*
 * The following translations apply only for V1 (Basic) headers
 *
 * NOTE: For backward compatibility this uses the 11-bit Apid as a MsgId Value
 * This header format packs both subsystem + topicid into this field.  Note it
 * is not required to pack the bits this way, but the shifts/masks used above
 * will produce a MsgId bit pattern comparable to existing versions of CFE.
 */
static inline void CFE_MissionLib_SpacePacketBasic_BitsToMsgId(CFE_SB_MsgId_t *                MsgId,
                                                               const CCSDS_SpacePacketBasic_t *Packet)
{
    CFE_SB_MsgId_t ApidPart;

    ApidPart.Value = Packet->CommonHdr.AppId;
    CFE_MissionLib_SetMsgIdApid(MsgId, CFE_MissionLib_GetMsgIdApid(&ApidPart));
    CFE_MissionLib_SetMsgIdSubsystem(MsgId, CFE_MissionLib_GetMsgIdSubsystem(&ApidPart));
    CFE_MissionLib_SetMsgIdInterfaceType(MsgId, Packet->CommonHdr.SecHdrFlags);
}

static inline void CFE_MissionLib_SpacePacketBasic_BitsFromMsgId(CCSDS_SpacePacketBasic_t *Packet,
                                                                 const CFE_SB_MsgId_t *    MsgId)
{
    CFE_SB_MsgId_t ApidPart;

    ApidPart.Value = 0;
    CFE_MissionLib_SetMsgIdApid(&ApidPart, CFE_MissionLib_GetMsgIdApid(MsgId));
    CFE_MissionLib_SetMsgIdSubsystem(&ApidPart, CFE_MissionLib_GetMsgIdSubsystem(MsgId));

    Packet->CommonHdr.VersionId   = 0;
    Packet->CommonHdr.AppId       = ApidPart.Value;
    Packet->CommonHdr.SecHdrFlags = CFE_MissionLib_GetMsgIdInterfaceType(MsgId);
}

/* The following translations apply only for V2 (ApidQ) headers */
static inline void CFE_MissionLib_SpacePacketApidQ_BitsToMsgId(CFE_SB_MsgId_t *                MsgId,
                                                               const CCSDS_SpacePacketApidQ_t *Packet)
{
    CFE_MissionLib_SetMsgIdApid(MsgId, Packet->CommonHdr.AppId);
    CFE_MissionLib_SetMsgIdInterfaceType(MsgId, Packet->CommonHdr.SecHdrFlags);
    CFE_MissionLib_SetMsgIdSystem(MsgId, Packet->ApidQ.SystemId);
    CFE_MissionLib_SetMsgIdSubsystem(MsgId, Packet->ApidQ.SubsystemId);
}

static inline void CFE_MissionLib_SpacePacketApidQ_BitsFromMsgId(CCSDS_SpacePacketApidQ_t *Packet,
                                                                 const CFE_SB_MsgId_t *    MsgId)
{
    Packet->CommonHdr.VersionId   = 1;
    Packet->CommonHdr.AppId       = CFE_MissionLib_GetMsgIdApid(MsgId);
    Packet->CommonHdr.SecHdrFlags = CFE_MissionLib_GetMsgIdInterfaceType(MsgId);
    Packet->ApidQ.SystemId        = CFE_MissionLib_GetMsgIdSystem(MsgId);
    Packet->ApidQ.SubsystemId     = CFE_MissionLib_GetMsgIdSubsystem(MsgId);
}

void CFE_MissionLib_MapListenerComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output,
                                         const CFE_SB_Listener_Component_t *    Input)
{
    memset(Output, 0, sizeof(*Output));

    if (Input->Telecommand.InstanceNumber <= CFE_MISSIONLIB_MSGID_SUBSYS_MASK &&
        Input->Telecommand.TopicId >= CFE_MISSION_TELECOMMAND_BASE_TOPICID &&
        Input->Telecommand.TopicId < CFE_MISSION_TELECOMMAND_MAX_TOPICID)
    {
        CFE_MissionLib_SetMsgIdInterfaceType(&Output->MsgId, CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS);
        CFE_MissionLib_SetMsgIdApid(&Output->MsgId, Input->Telecommand.TopicId - CFE_MISSION_TELECOMMAND_BASE_TOPICID);
        CFE_MissionLib_SetMsgIdSubsystem(&Output->MsgId, Input->Telecommand.InstanceNumber);
    }
}

void CFE_MissionLib_UnmapListenerComponent(CFE_SB_Listener_Component_t *                Output,
                                           const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    memset(Output, 0, sizeof(*Output));

    if (CFE_MissionLib_GetMsgIdInterfaceType(&Input->MsgId) == CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS)
    {
        Output->Telecommand.TopicId = CFE_MissionLib_GetMsgIdApid(&Input->MsgId) + CFE_MISSION_TELECOMMAND_BASE_TOPICID;
        Output->Telecommand.InstanceNumber = CFE_MissionLib_GetMsgIdSubsystem(&Input->MsgId);
    }
}

bool CFE_MissionLib_PubSub_IsListenerComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    return (CFE_MissionLib_GetMsgIdInterfaceType(&Params->MsgId) == CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS);
}

void CFE_MissionLib_MapPublisherComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output,
                                          const CFE_SB_Publisher_Component_t *   Input)
{
    memset(Output, 0, sizeof(*Output));

    if (Input->Telemetry.InstanceNumber <= CFE_MISSIONLIB_MSGID_SUBSYS_MASK &&
        Input->Telemetry.TopicId >= CFE_MISSION_TELEMETRY_BASE_TOPICID &&
        Input->Telemetry.TopicId < CFE_MISSION_TELEMETRY_MAX_TOPICID)
    {
        CFE_MissionLib_SetMsgIdInterfaceType(&Output->MsgId, CFE_MISSIONLIB_MSGID_TELEMETRY_BITS);
        CFE_MissionLib_SetMsgIdApid(&Output->MsgId, Input->Telemetry.TopicId - CFE_MISSION_TELEMETRY_BASE_TOPICID);
        CFE_MissionLib_SetMsgIdSubsystem(&Output->MsgId, Input->Telemetry.InstanceNumber);
    }
}

void CFE_MissionLib_UnmapPublisherComponent(CFE_SB_Publisher_Component_t *               Output,
                                            const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    memset(Output, 0, sizeof(*Output));

    if (CFE_MissionLib_GetMsgIdInterfaceType(&Input->MsgId) == CFE_MISSIONLIB_MSGID_TELEMETRY_BITS)
    {
        Output->Telemetry.TopicId = CFE_MissionLib_GetMsgIdApid(&Input->MsgId) + CFE_MISSION_TELEMETRY_BASE_TOPICID;
        Output->Telemetry.InstanceNumber = CFE_MissionLib_GetMsgIdSubsystem(&Input->MsgId);
    }
}

bool CFE_MissionLib_PubSub_IsPublisherComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    return (CFE_MissionLib_GetMsgIdInterfaceType(&Params->MsgId) == CFE_MISSIONLIB_MSGID_TELEMETRY_BITS);
}

void CFE_MissionLib_Get_PubSub_Parameters(CFE_SB_SoftwareBus_PubSub_Interface_t *Params,
                                          const CFE_HDR_Message_t *              Packet)
{
    memset(Params, 0, sizeof(*Params));
    CFE_MISSIONLIB_GET_MSGID_BITS(CFE_MISSION_MSG_HEADER_TYPE, &Params->MsgId, &Packet->CCSDS);
}

void CFE_MissionLib_Set_PubSub_Parameters(CFE_HDR_Message_t *                          Packet,
                                          const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    CFE_MISSIONLIB_SET_MSGID_BITS(CFE_MISSION_MSG_HEADER_TYPE, &Packet->CCSDS, &Params->MsgId);
}
