/*******************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
*******************************************************************************/

/**
 * @file
 *
 * Define sample app table
 */

#ifndef SAMPLE_APP_TABLE_H
#define SAMPLE_APP_TABLE_H

#include "sample_app_eds_typedefs.h"

/*
 * The EDS defines the table type name as "SampleAppTable" so it matches
 * the runtime table name, but the source code refers to the type
 * as SAMPLE_APP_Table_t
 *
 * This discrepancy can be worked around with a typedef for now
 */
typedef SAMPLE_APP_SampleAppTable_t SAMPLE_APP_Table_t;

#endif /* SAMPLE_APP_TABLE_H */
