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
 * \file     edslib_init.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation EDS library initialization function.
 *
 * Depending on the type of environment/executable that the EDS runtime library is
 * utilized in, it may need a custom initialization function.  This source file
 * is kept separate so it can be easily replaced/overridden by a custom initializer
 * if necessary.
 */

#include "edslib_internal.h"

/*
 * *************************************************************************************
 * Complete Initializer Function
 * Initializes the DataTypeDB, DisplayDB, and Binding Object subsystems.
 * *************************************************************************************
 */

void EdsLib_Initialize(void)
{
    EdsLib_DataTypeDB_Initialize();
    EdsLib_DisplayDB_Initialize();
    EdsLib_Binding_Initialize();
}

