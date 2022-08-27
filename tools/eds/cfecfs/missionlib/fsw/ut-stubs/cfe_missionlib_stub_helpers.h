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

#ifndef CFE_MISSIONLIB_STUB_HELPERS_H
#define CFE_MISSIONLIB_STUB_HELPERS_H

/*
** Includes
*/
#include <string.h>

#include "utstubs.h"
#include "utassert.h"

void CFE_MissionLib_Stub_DefaultZeroOutput(UT_EntryKey_t FuncKey, const UT_StubContext_t *Context, void *OutputBuffer,
                                           size_t OutputSize);
void CFE_MissionLib_Stub_DefaultStringOutput(UT_EntryKey_t FuncKey, const UT_StubContext_t *Context);

#endif /* CFE_MISSIONLIB_STUB_HELPERS_H */
