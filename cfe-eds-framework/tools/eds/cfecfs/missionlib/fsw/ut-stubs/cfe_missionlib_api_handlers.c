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
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_stub_helpers.h"

#include "utstubs.h"
#include "utassert.h"

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_FindCommandByName()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_FindCommandByName(void *UserObj, UT_EntryKey_t FuncKey,
                                                        const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "CommandIdBuffer", uint16_t *), sizeof(uint16_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_FindInterfaceByName()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_FindInterfaceByName(void *UserObj, UT_EntryKey_t FuncKey,
                                                          const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "InterfaceIdBuffer", uint16_t *), sizeof(uint16_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_FindTopicByName()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_FindTopicByName(void *UserObj, UT_EntryKey_t FuncKey,
                                                      const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "TopicIdBuffer", uint16_t *), sizeof(uint16_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetArgumentType()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetArgumentType(void *UserObj, UT_EntryKey_t FuncKey,
                                                      const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(FuncKey, Context, UT_Hook_GetArgValueByName(Context, "Id", EdsLib_Id_t *),
                                          sizeof(EdsLib_Id_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetIndicationInfo()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetIndicationInfo(void *UserObj, UT_EntryKey_t FuncKey,
                                                        const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "IndInfo", CFE_MissionLib_IndicationInfo_t *),
        sizeof(CFE_MissionLib_IndicationInfo_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetInterfaceInfo()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetInterfaceInfo(void *UserObj, UT_EntryKey_t FuncKey,
                                                       const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(
        FuncKey, Context, UT_Hook_GetArgValueByName(Context, "IntfInfo", CFE_MissionLib_InterfaceInfo_t *),
        sizeof(CFE_MissionLib_InterfaceInfo_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetSubcommandOffset()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetSubcommandOffset(void *UserObj, UT_EntryKey_t FuncKey,
                                                          const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(FuncKey, Context, UT_Hook_GetArgValueByName(Context, "Offset", uint16_t *),
                                          sizeof(uint16_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetTopicInfo()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetTopicInfo(void *UserObj, UT_EntryKey_t FuncKey,
                                                   const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultZeroOutput(FuncKey, Context,
                                          UT_Hook_GetArgValueByName(Context, "TopicInfo", CFE_MissionLib_TopicInfo_t *),
                                          sizeof(CFE_MissionLib_TopicInfo_t));
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetCommandName()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetCommandName(void *UserObj, UT_EntryKey_t FuncKey,
                                                     const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultStringOutput(FuncKey, Context);
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetInterfaceName()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetInterfaceName(void *UserObj, UT_EntryKey_t FuncKey,
                                                       const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultStringOutput(FuncKey, Context);
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetInstanceName()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetInstanceName(void *UserObj, UT_EntryKey_t FuncKey,
                                                      const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultStringOutput(FuncKey, Context);
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetInstanceNumber()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetInstanceNumber(void *UserObj, UT_EntryKey_t FuncKey,
                                                        const UT_StubContext_t *Context)
{
    int32_t  Status;
    uint16_t Result;

    UT_Stub_GetInt32StatusCode(Context, &Status);
    Result = Status;
    UT_SetDefaultReturnValue(FuncKey, Result);
}

/*
 * ----------------------------------------------------
 * Handler function for CFE_MissionLib_GetTopicName()
 * ----------------------------------------------------
 */
void UT_DefaultHandler_CFE_MissionLib_GetTopicName(void *UserObj, UT_EntryKey_t FuncKey,
                                                   const UT_StubContext_t *Context)
{
    CFE_MissionLib_Stub_DefaultStringOutput(FuncKey, Context);
}