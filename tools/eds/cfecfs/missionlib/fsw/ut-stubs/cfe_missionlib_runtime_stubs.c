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
 * Auto-Generated stub implementations for functions defined in cfe_missionlib_runtime header
 */

#include "cfe_missionlib_runtime.h"
#include "utgenstub.h"

extern void UT_DefaultHandler_CFE_MissionLib_Get_PubSub_Parameters(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_MapListenerComponent(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_MapPublisherComponent(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_PubSub_IsListenerComponent(void *, UT_EntryKey_t,
                                                                        const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_PubSub_IsPublisherComponent(void *, UT_EntryKey_t,
                                                                         const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_Set_PubSub_Parameters(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_UnmapListenerComponent(void *, UT_EntryKey_t, const UT_StubContext_t *);
extern void UT_DefaultHandler_CFE_MissionLib_UnmapPublisherComponent(void *, UT_EntryKey_t, const UT_StubContext_t *);

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_Get_PubSub_Parameters()
 * ----------------------------------------------------
 */
void CFE_MissionLib_Get_PubSub_Parameters(CFE_SB_SoftwareBus_PubSub_Interface_t *Params,
                                          const CFE_HDR_Message_t *              Packet)
{
    UT_GenStub_AddParam(CFE_MissionLib_Get_PubSub_Parameters, CFE_SB_SoftwareBus_PubSub_Interface_t *, Params);
    UT_GenStub_AddParam(CFE_MissionLib_Get_PubSub_Parameters, const CFE_HDR_Message_t *, Packet);

    UT_GenStub_Execute(CFE_MissionLib_Get_PubSub_Parameters, Basic,
                       UT_DefaultHandler_CFE_MissionLib_Get_PubSub_Parameters);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_MapListenerComponent()
 * ----------------------------------------------------
 */
void CFE_MissionLib_MapListenerComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output,
                                         const CFE_SB_Listener_Component_t *    Input)
{
    UT_GenStub_AddParam(CFE_MissionLib_MapListenerComponent, CFE_SB_SoftwareBus_PubSub_Interface_t *, Output);
    UT_GenStub_AddParam(CFE_MissionLib_MapListenerComponent, const CFE_SB_Listener_Component_t *, Input);

    UT_GenStub_Execute(CFE_MissionLib_MapListenerComponent, Basic,
                       UT_DefaultHandler_CFE_MissionLib_MapListenerComponent);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_MapPublisherComponent()
 * ----------------------------------------------------
 */
void CFE_MissionLib_MapPublisherComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output,
                                          const CFE_SB_Publisher_Component_t *   Input)
{
    UT_GenStub_AddParam(CFE_MissionLib_MapPublisherComponent, CFE_SB_SoftwareBus_PubSub_Interface_t *, Output);
    UT_GenStub_AddParam(CFE_MissionLib_MapPublisherComponent, const CFE_SB_Publisher_Component_t *, Input);

    UT_GenStub_Execute(CFE_MissionLib_MapPublisherComponent, Basic,
                       UT_DefaultHandler_CFE_MissionLib_MapPublisherComponent);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_PubSub_IsListenerComponent()
 * ----------------------------------------------------
 */
bool CFE_MissionLib_PubSub_IsListenerComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_PubSub_IsListenerComponent, bool);

    UT_GenStub_AddParam(CFE_MissionLib_PubSub_IsListenerComponent, const CFE_SB_SoftwareBus_PubSub_Interface_t *,
                        Params);

    UT_GenStub_Execute(CFE_MissionLib_PubSub_IsListenerComponent, Basic,
                       UT_DefaultHandler_CFE_MissionLib_PubSub_IsListenerComponent);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_PubSub_IsListenerComponent, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_PubSub_IsPublisherComponent()
 * ----------------------------------------------------
 */
bool CFE_MissionLib_PubSub_IsPublisherComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    UT_GenStub_SetupReturnBuffer(CFE_MissionLib_PubSub_IsPublisherComponent, bool);

    UT_GenStub_AddParam(CFE_MissionLib_PubSub_IsPublisherComponent, const CFE_SB_SoftwareBus_PubSub_Interface_t *,
                        Params);

    UT_GenStub_Execute(CFE_MissionLib_PubSub_IsPublisherComponent, Basic,
                       UT_DefaultHandler_CFE_MissionLib_PubSub_IsPublisherComponent);

    return UT_GenStub_GetReturnValue(CFE_MissionLib_PubSub_IsPublisherComponent, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_Set_PubSub_Parameters()
 * ----------------------------------------------------
 */
void CFE_MissionLib_Set_PubSub_Parameters(CFE_HDR_Message_t *                          Packet,
                                          const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    UT_GenStub_AddParam(CFE_MissionLib_Set_PubSub_Parameters, CFE_HDR_Message_t *, Packet);
    UT_GenStub_AddParam(CFE_MissionLib_Set_PubSub_Parameters, const CFE_SB_SoftwareBus_PubSub_Interface_t *, Params);

    UT_GenStub_Execute(CFE_MissionLib_Set_PubSub_Parameters, Basic,
                       UT_DefaultHandler_CFE_MissionLib_Set_PubSub_Parameters);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_UnmapListenerComponent()
 * ----------------------------------------------------
 */
void CFE_MissionLib_UnmapListenerComponent(CFE_SB_Listener_Component_t *                Output,
                                           const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    UT_GenStub_AddParam(CFE_MissionLib_UnmapListenerComponent, CFE_SB_Listener_Component_t *, Output);
    UT_GenStub_AddParam(CFE_MissionLib_UnmapListenerComponent, const CFE_SB_SoftwareBus_PubSub_Interface_t *, Input);

    UT_GenStub_Execute(CFE_MissionLib_UnmapListenerComponent, Basic,
                       UT_DefaultHandler_CFE_MissionLib_UnmapListenerComponent);
}

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MissionLib_UnmapPublisherComponent()
 * ----------------------------------------------------
 */
void CFE_MissionLib_UnmapPublisherComponent(CFE_SB_Publisher_Component_t *               Output,
                                            const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    UT_GenStub_AddParam(CFE_MissionLib_UnmapPublisherComponent, CFE_SB_Publisher_Component_t *, Output);
    UT_GenStub_AddParam(CFE_MissionLib_UnmapPublisherComponent, const CFE_SB_SoftwareBus_PubSub_Interface_t *, Input);

    UT_GenStub_Execute(CFE_MissionLib_UnmapPublisherComponent, Basic,
                       UT_DefaultHandler_CFE_MissionLib_UnmapPublisherComponent);
}
