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
 * Auto-Generated stub implementations for functions defined in cfe_msg_dispatcher header
 */

#include "cfe_msg_dispatcher.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for CFE_MSG_EdsDispatch()
 * ----------------------------------------------------
 */
CFE_Status_t CFE_MSG_EdsDispatch(uint16 InterfaceID, uint16 IndicationIndex, uint16 DispatchTableID,
                                 const CFE_SB_Buffer_t *Buffer, const void *DispatchTable)
{
    UT_GenStub_SetupReturnBuffer(CFE_MSG_EdsDispatch, CFE_Status_t);

    UT_GenStub_AddParam(CFE_MSG_EdsDispatch, uint16, InterfaceID);
    UT_GenStub_AddParam(CFE_MSG_EdsDispatch, uint16, IndicationIndex);
    UT_GenStub_AddParam(CFE_MSG_EdsDispatch, uint16, DispatchTableID);
    UT_GenStub_AddParam(CFE_MSG_EdsDispatch, const CFE_SB_Buffer_t *, Buffer);
    UT_GenStub_AddParam(CFE_MSG_EdsDispatch, const void *, DispatchTable);

    UT_GenStub_Execute(CFE_MSG_EdsDispatch, Basic, NULL);

    return UT_GenStub_GetReturnValue(CFE_MSG_EdsDispatch, CFE_Status_t);
}
