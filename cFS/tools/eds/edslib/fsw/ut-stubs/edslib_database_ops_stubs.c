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
 * Auto-Generated stub implementations for functions defined in edslib_database_ops header
 */

#include "edslib_database_ops.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_DatabaseRef_IsEqual()
 * ----------------------------------------------------
 */
bool EdsLib_DatabaseRef_IsEqual(const EdsLib_DatabaseRef_t *RefObj1, const EdsLib_DatabaseRef_t *RefObj2)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_DatabaseRef_IsEqual, bool);

    UT_GenStub_AddParam(EdsLib_DatabaseRef_IsEqual, const EdsLib_DatabaseRef_t *, RefObj1);
    UT_GenStub_AddParam(EdsLib_DatabaseRef_IsEqual, const EdsLib_DatabaseRef_t *, RefObj2);

    UT_GenStub_Execute(EdsLib_DatabaseRef_IsEqual, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_DatabaseRef_IsEqual, bool);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Decode_StructId()
 * ----------------------------------------------------
 */
void EdsLib_Decode_StructId(EdsLib_DatabaseRef_t *RefObj, EdsLib_Id_t EdsId)
{
    UT_GenStub_AddParam(EdsLib_Decode_StructId, EdsLib_DatabaseRef_t *, RefObj);
    UT_GenStub_AddParam(EdsLib_Decode_StructId, EdsLib_Id_t, EdsId);

    UT_GenStub_Execute(EdsLib_Decode_StructId, Basic, NULL);
}

/*
 * ----------------------------------------------------
 * Generated stub function for EdsLib_Encode_StructId()
 * ----------------------------------------------------
 */
EdsLib_Id_t EdsLib_Encode_StructId(const EdsLib_DatabaseRef_t *RefObj)
{
    UT_GenStub_SetupReturnBuffer(EdsLib_Encode_StructId, EdsLib_Id_t);

    UT_GenStub_AddParam(EdsLib_Encode_StructId, const EdsLib_DatabaseRef_t *, RefObj);

    UT_GenStub_Execute(EdsLib_Encode_StructId, Basic, NULL);

    return UT_GenStub_GetReturnValue(EdsLib_Encode_StructId, EdsLib_Id_t);
}
