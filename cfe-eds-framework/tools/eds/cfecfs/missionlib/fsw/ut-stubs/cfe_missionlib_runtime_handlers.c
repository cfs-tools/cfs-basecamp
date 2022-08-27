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
** Includes
*/
#include <string.h>
#include "cfe_missionlib_stub_helpers.h"
#include "cfe_missionlib_runtime.h"

#include "utstubs.h"
#include "utassert.h"

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_PubSub_IsListenerComponent coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_PubSub_IsListenerComponent(void *UserObj, UT_EntryKey_t FuncKey,
                                                                 const UT_StubContext_t *Context)
{
    int32 status;
    bool  result;

    UT_Stub_GetInt32StatusCode(Context, &status);
    result = (status != 0);

    UT_Stub_SetReturnValue(FuncKey, result);
}

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_PubSub_IsPublisherComponent coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_PubSub_IsPublisherComponent(void *UserObj, UT_EntryKey_t FuncKey,
                                                                  const UT_StubContext_t *Context)
{
    int32 status;
    bool  result;

    UT_Stub_GetInt32StatusCode(Context, &status);
    result = (status != 0);

    UT_Stub_SetReturnValue(FuncKey, result);
}

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_MapListenerComponent coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_MapListenerComponent(void *UserObj, UT_EntryKey_t FuncKey,
                                                           const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "Output", CFE_SB_SoftwareBus_PubSub_Interface_t *),
        sizeof(CFE_SB_SoftwareBus_PubSub_Interface_t));
}

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_UnmapListenerComponent coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_UnmapListenerComponent(void *UserObj, UT_EntryKey_t FuncKey,
                                                             const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(FuncKey, Context,
                                          UT_Hook_GetArgValueByName(Context, "Output", CFE_SB_Listener_Component_t *),
                                          sizeof(CFE_SB_Listener_Component_t));
}

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_MapPublisherComponent coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_MapPublisherComponent(void *UserObj, UT_EntryKey_t FuncKey,
                                                            const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "Output", CFE_SB_SoftwareBus_PubSub_Interface_t *),
        sizeof(CFE_SB_SoftwareBus_PubSub_Interface_t));
}

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_UnmapPublisherComponent coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_UnmapPublisherComponent(void *UserObj, UT_EntryKey_t FuncKey,
                                                              const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(FuncKey, Context,
                                          UT_Hook_GetArgValueByName(Context, "Output", CFE_SB_Publisher_Component_t *),
                                          sizeof(CFE_SB_Publisher_Component_t));
}

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_Get_PubSub_Parameters coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_Get_PubSub_Parameters(void *UserObj, UT_EntryKey_t FuncKey,
                                                            const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "Params", CFE_SB_SoftwareBus_PubSub_Interface_t *),
        sizeof(CFE_SB_SoftwareBus_PubSub_Interface_t));
}

/*------------------------------------------------------------
 *
 * Default handler for CFE_MissionLib_Set_PubSub_Parameters coverage stub function
 *
 *------------------------------------------------------------*/
void UT_DefaultHandler_CFE_MissionLib_Set_PubSub_Parameters(void *UserObj, UT_EntryKey_t FuncKey,
                                                            const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "Packet", CFE_HDR_Message_t *), sizeof(CFE_HDR_Message_t));
}
