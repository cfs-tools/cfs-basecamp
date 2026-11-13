/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
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
 * Main header file for the NASA_WORL application
 */

#ifndef @TEMPLATE@_H
#define @TEMPLATE@_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_config.h"

#include "@template@_mission_cfg.h"
#include "@template@_platform_cfg.h"

#include "@template@_perfids.h"
#include "@template@_msgids.h"
#include "@template@_msg.h"

/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CommandCounter;
    uint8 CommandErrorCounter;

    /*
    ** Housekeeping telemetry packet...
    */
    @TEMPLATE@_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

} @TEMPLATE@_Data_t;

/*
** Global data structure
*/
extern @TEMPLATE@_Data_t @TEMPLATE@_Data;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (@TEMPLATE@_Main), these
**       functions are not called from any other source module.
*/
void         @TEMPLATE@_AppMain(void);
CFE_Status_t @TEMPLATE@_Init(void);

#endif /* @TEMPLATE@_H */
