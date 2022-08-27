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

extern void EdsLib_Basic_Test(void);
extern void EdsLib_Full_Test(void);
extern void EdsLib_StringConv_Test(void);

void UtTest_Setup(void)
{
    UtTest_Add(EdsLib_Basic_Test, NULL, NULL, "EDS Basic");
    UtTest_Add(EdsLib_Full_Test, NULL, NULL, "EDS Full");
    UtTest_Add(EdsLib_StringConv_Test, NULL, NULL, "EDS String Conversions");
}

