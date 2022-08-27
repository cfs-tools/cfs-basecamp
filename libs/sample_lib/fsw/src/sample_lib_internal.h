/************************************************************************
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
** File: sample_lib_internal.h
**
** Purpose:
**  An example of an internal (private) header file for SAMPLE Lib
**
** Notes:
**
*************************************************************************/
#ifndef _sample_lib_internal_h_
#define _sample_lib_internal_h_

/* Include all external/public definitions */
#include "sample_lib.h"

/*************************************************************************
** Macro Definitions
*************************************************************************/

#define SAMPLE_LIB_BUFFER_SIZE 16

/*************************************************************************
** Internal Data Structures
*************************************************************************/
extern char SAMPLE_LIB_Buffer[SAMPLE_LIB_BUFFER_SIZE];

/*************************************************************************
** Function Declarations
*************************************************************************/

/**
 * Library initialization routine/entry point
 */
int32 SAMPLE_LIB_Init(void);

#endif /* _sample_lib_internal_h_ */

/************************/
/*  End of File Comment */
/************************/
