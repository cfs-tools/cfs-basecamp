/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
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
 *   This file contains the source code for the @TEMPLATE@.
 */

/*
** Include Files:
*/
#include "@template@_events.h"
#include "@template@_version.h"
#include "@template@_app.h"

#include <string.h>

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
    int32            status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(@TEMPLATE@_PERF_ID);

    /*
    ** Perform application specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = @TEMPLATE@_Init();
    if (status != CFE_SUCCESS)
    {
        @TEMPLATE@_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** @TEMPLATE@ Runloop
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
            @TEMPLATE@_ProcessCommandPacket(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(@TEMPLATE@_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@: SB Pipe Read Error, App Will Exit");

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
int32 @TEMPLATE@_Init(void)
{
    int32 status;

    @TEMPLATE@_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize app command execution counters
    */
    @TEMPLATE@_Data.CmdCounter = 0;
    @TEMPLATE@_Data.ErrCounter = 0;

    /*
    ** Initialize Example Param Command Data
    */
    @TEMPLATE@_Data.ExampleParamCmdVal = 0;
    
    /*
    ** Initialize app configuration data
    */
    @TEMPLATE@_Data.PipeDepth = @TEMPLATE@_PIPE_DEPTH;

    strncpy(@TEMPLATE@_Data.PipeName, "@TEMPLATE@_CMD_PIPE", sizeof(@TEMPLATE@_Data.PipeName));
    @TEMPLATE@_Data.PipeName[sizeof(@TEMPLATE@_Data.PipeName) - 1] = 0;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("@TEMPLATE@: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Initialize housekeeping packet (clear user data area).
    */
    CFE_MSG_Init(CFE_MSG_PTR(@TEMPLATE@_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(@TEMPLATE@_HK_TLM_MID),
                 sizeof(@TEMPLATE@_Data.HkTlm));

    /*
    ** Create Software Bus message pipe.
    */
    status = CFE_SB_CreatePipe(&@TEMPLATE@_Data.CommandPipe, @TEMPLATE@_Data.PipeDepth, @TEMPLATE@_Data.PipeName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("@TEMPLATE@: Error creating pipe, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Subscribe to Housekeeping request commands
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(@TEMPLATE@_SEND_HK_MID), @TEMPLATE@_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("@TEMPLATE@: Error Subscribing to HK request, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Subscribe to ground command packets
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(@TEMPLATE@_CMD_MID), @TEMPLATE@_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("@TEMPLATE@: Error Subscribing to Command, RC = 0x%08lX\n", (unsigned long)status);

        return status;
    }

    CFE_EVS_SendEvent(@TEMPLATE@_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "@TEMPLATE@ Initialized.%s",
                      @TEMPLATE@_VERSION_STRING);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the           */
/*     @TEMPLATE@ command pipe.                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void @TEMPLATE@_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case @TEMPLATE@_CMD_MID:
            @TEMPLATE@_ProcessGroundCommand(SBBufPtr);
            break;

        case @TEMPLATE@_SEND_HK_MID:
            @TEMPLATE@_ReportHousekeeping((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(@TEMPLATE@_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "@TEMPLATE@: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* @TEMPLATE@ ground commands                                                 */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void @TEMPLATE@_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process "known" @TEMPLATE@ ground commands
    */
    switch (CommandCode)
    {
        case @TEMPLATE@_NOOP_CC:
            if (@TEMPLATE@_VerifyCmdLength(&SBBufPtr->Msg, sizeof(@TEMPLATE@_NoopCmd_t)))
            {
                @TEMPLATE@_Noop((@TEMPLATE@_NoopCmd_t *)SBBufPtr);
            }

            break;

        case @TEMPLATE@_RESET_COUNTERS_CC:
            if (@TEMPLATE@_VerifyCmdLength(&SBBufPtr->Msg, sizeof(@TEMPLATE@_ResetCountersCmd_t)))
            {
                @TEMPLATE@_ResetCounters((@TEMPLATE@_ResetCountersCmd_t *)SBBufPtr);
            }

            break;

        case @TEMPLATE@_EXAMPLE_PARAM_CC:
            if (@TEMPLATE@_VerifyCmdLength(&SBBufPtr->Msg, sizeof(@TEMPLATE@_ExampleParamCmd_t)))
            {
                @TEMPLATE@_ExampleParamCmd((@TEMPLATE@_ExampleParamCmd_t *)SBBufPtr);
            }

            break;
            
        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(@TEMPLATE@_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid ground command code: CC = %d", CommandCode);
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 @TEMPLATE@_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg)
{

    /*
    ** Get command execution counters...
    */
    @TEMPLATE@_Data.HkTlm.Payload.CommandErrorCounter = @TEMPLATE@_Data.ErrCounter;
    @TEMPLATE@_Data.HkTlm.Payload.CommandCounter      = @TEMPLATE@_Data.CmdCounter;

    /*
    ** Get the parameter set by last 'ExampleParamCmd' command
    */
    @TEMPLATE@_Data.HkTlm.Payload.ExampleParamCmdVal = @TEMPLATE@_Data.ExampleParamCmdVal;
    
    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(@TEMPLATE@_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(@TEMPLATE@_Data.HkTlm.TelemetryHeader), true);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* @TEMPLATE@ NOOP commands                                                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 @TEMPLATE@_Noop(const @TEMPLATE@_NoopCmd_t *Msg)
{
    @TEMPLATE@_Data.CmdCounter++;

    CFE_EVS_SendEvent(@TEMPLATE@_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION, "@TEMPLATE@: NOOP command %s",
                      @TEMPLATE@_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 @TEMPLATE@_ResetCounters(const @TEMPLATE@_ResetCountersCmd_t *Msg)
{
    @TEMPLATE@_Data.CmdCounter = 0;
    @TEMPLATE@_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(@TEMPLATE@_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION, "@TEMPLATE@: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function set a global data parameter that is sent in the      */
/*         housekeeping telemetry message.                                    */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 @TEMPLATE@_ExampleParamCmd(const @TEMPLATE@_ExampleParamCmd_t *Msg)
{
   
   const @TEMPLATE@_ExampleParamCmd_Payload_t *Cmd = &Msg->Payload;
   
   @TEMPLATE@_Data.ExampleParamCmdVal = Cmd->Param;
   
   CFE_EVS_SendEvent (@TEMPLATE@_EXAMPLE_PARAM_CMD_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "Example parameter commmand received a parameter value of %d",
                      Cmd->Param);
   
   @TEMPLATE@_Data.CmdCounter++;
    
   return CFE_SUCCESS;


} /* End @TEMPLATE@_ExampleParamCmd() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool @TEMPLATE@_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(@TEMPLATE@_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        @TEMPLATE@_Data.ErrCounter++;
    }

    return result;
}
