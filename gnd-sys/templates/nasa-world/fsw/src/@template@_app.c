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
 * \file
 *   This file contains the source code for the @TEMPLATE@ App.
 */

/*
** Include Files:
*/
#include "@template@_app.h"
#include "@template@_cmds.h"
#include "@template@_utils.h"
#include "@template@_eventids.h"
#include "@template@_dispatch.h"
#include "@template@_version.h"

/*
** global data
*/
@TEMPLATE@_Data_t @TEMPLATE@_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void @TEMPLATE@_AppMain(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(@TEMPLATE@_PERF_ID);

    /*
    ** Perform application-specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = @TEMPLATE@_Init();
    if (status != CFE_SUCCESS)
    {
        @TEMPLATE@_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** @TEMPLATE@ App Runloop
    */
    while (CFE_ES_RunLoop(&@TEMPLATE@_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(@TEMPLATE@_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, @TEMPLATE@_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(@TEMPLATE@_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            @TEMPLATE@_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(@TEMPLATE@_PIPE_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@ App: SB Pipe Read Error, App Will Exit");

            @TEMPLATE@_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(@TEMPLATE@_PERF_ID);

    CFE_ES_ExitApp(@TEMPLATE@_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t @TEMPLATE@_Init(void)
{
    CFE_Status_t status;
    char         VersionString[@TEMPLATE@_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    memset(&@TEMPLATE@_Data, 0, sizeof(@TEMPLATE@_Data));

    @TEMPLATE@_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("@TEMPLATE@ App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(@TEMPLATE@_Data.HkTlm.TelemetryHeader),
                     CFE_SB_ValueToMsgId(@TEMPLATE@_HK_TLM_MID),
                     sizeof(@TEMPLATE@_Data.HkTlm));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&@TEMPLATE@_Data.CommandPipe,
                                   @TEMPLATE@_PLATFORM_PIPE_DEPTH,
                                   @TEMPLATE@_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(@TEMPLATE@_CR_PIPE_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@ App: Error creating SB Command Pipe, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to Housekeeping request commands
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(@TEMPLATE@_SEND_HK_MID), @TEMPLATE@_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(@TEMPLATE@_SUB_HK_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@ App: Error Subscribing to HK request, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(@TEMPLATE@_CMD_MID), @TEMPLATE@_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(@TEMPLATE@_SUB_CMD_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@ App: Error Subscribing to Commands, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {

        CFE_Config_GetVersionString(VersionString,
                                    @TEMPLATE@_CFG_MAX_VERSION_STR_LEN,
                                    "@TEMPLATE@ App",
                                    @TEMPLATE@_VERSION,
                                    @TEMPLATE@_BUILD_CODENAME,
                                    @TEMPLATE@_LAST_OFFICIAL);

        CFE_EVS_SendEvent(@TEMPLATE@_INIT_INF_EID,
                          CFE_EVS_EventType_INFORMATION,
                          "@TEMPLATE@ App Initialized.%s",
                          VersionString);
    }

    return status;
}
