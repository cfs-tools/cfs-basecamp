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
 * \file     edslib_test.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
 * Unit test entry point
 */

#include "utassert.h"
#include "uttest.h"

#include "edslib_test_common.h"
#include "edslib_global.h"
#include "edslib_datatypedb.h"

#define UT_TYPE_ID(x, y)     EDSLIB_MAKE_ID(UT_INDEX_##x, y##_DATADICTIONARY)
#define UT_INSTANCE_ID(x, y) EDSLIB_INTF_ID(UT_INDEX_##x, y##_INSTANCE)
#define UT_DECL_ID(x, y)     EDSLIB_INTF_ID(UT_INDEX_##x, y##_DECLARATION)

const EdsLib_Id_t UT_EDS_UINT32_ID = UT_TYPE_ID(CCSDS_SOIS_SEDS, EdsInteger_CCSDS_SOIS_SEDS_UINT32);
const EdsLib_Id_t UT_EDS_INT8_ID   = UT_TYPE_ID(CCSDS_SOIS_SEDS, EdsInteger_CCSDS_SOIS_SEDS_INT8);
const EdsLib_Id_t UT_EDS_UINT8_ID  = UT_TYPE_ID(CCSDS_SOIS_SEDS, EdsInteger_CCSDS_SOIS_SEDS_UINT8);
const EdsLib_Id_t UT_EDS_ENUM_ID   = UT_TYPE_ID(UTAPP, EdsEnum_UtApp_CMDVAL);
const EdsLib_Id_t UT_EDS_CMD1_ID   = UT_TYPE_ID(UTAPP, EdsContainer_UtApp_Cmd1);
const EdsLib_Id_t UT_EDS_CMDHDR_ID = UT_TYPE_ID(UTHDR, EdsContainer_UtHdr_UtCmdHdr);

const EdsLib_Id_t UT_EDS_COMPONENT_ID = UT_INSTANCE_ID(UTAPP, EdsComponent_UtApp_UnitTest);
const EdsLib_Id_t UT_EDS_CMDDECL_ID   = UT_DECL_ID(UTHDR, EdsInterface_UtHdr_UtCmd);
const EdsLib_Id_t UT_EDS_CMDINTF_ID   = UT_INSTANCE_ID(UTAPP, EdsInterface_UtApp_UnitTest_TestCmd);
const EdsLib_Id_t UT_EDS_RECVCMD_ID   = UT_DECL_ID(UTHDR, EdsCommand_UtHdr_UtCmd_recv);
