/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *
 * Auto-Generated stub implementations for functions defined in scriptengine header
 */

#include "scriptengine.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for SCRIPTENGINE_CallFunctionArg()
 * ----------------------------------------------------
 */
int32 SCRIPTENGINE_CallFunctionArg(const char *FunctionName, void *ArgData, uint16 AppIdx, uint16 FormatIdx)
{
    UT_GenStub_SetupReturnBuffer(SCRIPTENGINE_CallFunctionArg, int32);

    UT_GenStub_AddParam(SCRIPTENGINE_CallFunctionArg, const char *, FunctionName);
    UT_GenStub_AddParam(SCRIPTENGINE_CallFunctionArg, void *, ArgData);
    UT_GenStub_AddParam(SCRIPTENGINE_CallFunctionArg, uint16, AppIdx);
    UT_GenStub_AddParam(SCRIPTENGINE_CallFunctionArg, uint16, FormatIdx);

    UT_GenStub_Execute(SCRIPTENGINE_CallFunctionArg, Basic, NULL);

    return UT_GenStub_GetReturnValue(SCRIPTENGINE_CallFunctionArg, int32);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SCRIPTENGINE_CallFunctionVoid()
 * ----------------------------------------------------
 */
int32 SCRIPTENGINE_CallFunctionVoid(const char *FunctionName)
{
    UT_GenStub_SetupReturnBuffer(SCRIPTENGINE_CallFunctionVoid, int32);

    UT_GenStub_AddParam(SCRIPTENGINE_CallFunctionVoid, const char *, FunctionName);

    UT_GenStub_Execute(SCRIPTENGINE_CallFunctionVoid, Basic, NULL);

    return UT_GenStub_GetReturnValue(SCRIPTENGINE_CallFunctionVoid, int32);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SCRIPTENGINE_Init()
 * ----------------------------------------------------
 */
int32 SCRIPTENGINE_Init(void)
{
    UT_GenStub_SetupReturnBuffer(SCRIPTENGINE_Init, int32);

    UT_GenStub_Execute(SCRIPTENGINE_Init, Basic, NULL);

    return UT_GenStub_GetReturnValue(SCRIPTENGINE_Init, int32);
}

/*
 * ----------------------------------------------------
 * Generated stub function for SCRIPTENGINE_LoadFile()
 * ----------------------------------------------------
 */
int32 SCRIPTENGINE_LoadFile(const char *Filename)
{
    UT_GenStub_SetupReturnBuffer(SCRIPTENGINE_LoadFile, int32);

    UT_GenStub_AddParam(SCRIPTENGINE_LoadFile, const char *, Filename);

    UT_GenStub_Execute(SCRIPTENGINE_LoadFile, Basic, NULL);

    return UT_GenStub_GetReturnValue(SCRIPTENGINE_LoadFile, int32);
}
