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
** File: ci_lab_app.h
**
** Purpose:
**   This file is main hdr file for the Command Ingest lab application.
**
*******************************************************************************/

#ifndef _ci_lab_app_h_
#define _ci_lab_app_h_

/*
** Required header files...
*/
#include "common_types.h"
#include "cfe.h"

#include "osapi.h"

#include "ci_lab_eds_typedefs.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>

/****************************************************************************/

#define CI_LAB_BASE_UDP_PORT 1234
#define CI_LAB_PIPE_DEPTH    32

/************************************************************************
** Type Definitions
*************************************************************************/
typedef struct
{
    bool            SocketConnected;
    CFE_SB_PipeId_t CommandPipe;
    osal_id_t       SocketID;
    OS_SockAddr_t   SocketAddress;

    CI_LAB_HkTlm_t HkTlm;

    CFE_HDR_Message_PackedBuffer_t NetworkBuffer;

} CI_LAB_GlobalData_t;

extern CI_LAB_GlobalData_t CI_LAB_Global;

/****************************************************************************/
/*
** Local function prototypes...
**
** Note: Except for the entry point (CI_LAB_AppMain), these
**       functions are not called from any other source module.
*/
void CI_Lab_AppMain(void);
void CI_LAB_TaskInit(void);
void CI_LAB_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void CI_LAB_ResetCounters_Internal(void);
void CI_LAB_ReadUpLink(void);

/*
 * Individual message handler function prototypes
 *
 * Per the recommended code pattern, these should accept a const pointer
 * to a structure type which matches the message, and return an int32
 * where CFE_SUCCESS (0) indicates successful handling of the message.
 */
int32 CI_LAB_Noop(const CI_LAB_NoopCmd_t *data);
int32 CI_LAB_ResetCounters(const CI_LAB_ResetCountersCmd_t *data);

/* Housekeeping message handler */
int32 CI_LAB_ReportHousekeeping(const CFE_MSG_CommandHeader_t *data);

#endif /* _ci_lab_app_h_ */
