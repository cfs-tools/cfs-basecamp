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
 *   This file is main hdr file for the Command Ingest lab application.
 */
#ifndef CI_LAB_APP_H
#define CI_LAB_APP_H

/*
** Required header files...
*/
#include "common_types.h"
#include "osapi.h"
#include "cfe.h"
#include "cfe_config.h"

#include "ci_lab_mission_cfg.h"
#include "ci_lab_platform_cfg.h"
#include "ci_lab_eventids.h"
#include "ci_lab_dispatch.h"
#include "ci_lab_cmds.h"

#include "ci_lab_msg.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/************************************************************************
** Type Definitions
*************************************************************************/

typedef struct
{
    bool            SocketConnected;
    bool            Scheduled;
    bool            AllowPassthrough;
    CFE_SB_PipeId_t CommandPipe;
    osal_id_t       SocketID;
    OS_SockAddr_t   SocketAddress;

    CI_LAB_HkTlm_t HkTlm;

    void  *NetBufPtr;
    size_t NetBufSize;

} CI_LAB_GlobalData_t;

/****************************************************************************/
/*
** Local function prototypes...
**
** Note: Except for the entry point (CI_LAB_AppMain), these
**       functions are not called from any other source module.
*/
void CI_LAB_AppMain(void);
void CI_LAB_TaskInit(void);
void CI_LAB_ResetCounters_Internal(void);
void CI_LAB_ReadUpLink(void);

/* Global State Object */
extern CI_LAB_GlobalData_t CI_LAB_Global;

#endif
