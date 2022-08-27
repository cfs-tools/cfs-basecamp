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
** File: ci_lab_app.c
**
** Purpose:
**   This file contains the source code for the Command Ingest task.
**
*******************************************************************************/

/*
**   Include Files:
*/

#include "ci_lab_app.h"
#include "ci_lab_perfids.h"
#include "ci_lab_msgids.h"
#include "ci_lab_msg.h"
#include "ci_lab_events.h"
#include "ci_lab_version.h"

#include "cfe_config.h"

#include "edslib_datatypedb.h"
#include "ci_lab_eds_typedefs.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

/*
** CI global data...
*/
CI_LAB_GlobalData_t CI_LAB_Global;

static CFE_EVS_BinFilter_t CI_LAB_EventFilters[] =
    {/* Event ID    mask */
     {CI_LAB_SOCKETCREATE_ERR_EID, 0x0000}, {CI_LAB_SOCKETBIND_ERR_EID, 0x0000}, {CI_LAB_STARTUP_INF_EID, 0x0000},
     {CI_LAB_COMMAND_ERR_EID, 0x0000},      {CI_LAB_COMMANDNOP_INF_EID, 0x0000}, {CI_LAB_COMMANDRST_INF_EID, 0x0000},
     {CI_LAB_INGEST_INF_EID, 0x0000},       {CI_LAB_INGEST_LEN_ERR_EID, 0x0000}, {CI_LAB_INGEST_ALLOC_ERR_EID, 0x0000},
     {CI_LAB_INGEST_SEND_ERR_EID, 0x0000}};

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* CI_Lab_AppMain() -- Application entry point and main process loop          */
/* Purpose: This is the Main task event loop for the Command Ingest Task      */
/*            The task handles all interfaces to the data system through      */
/*            the software bus. There is one pipeline into this task          */
/*            The task is scheduled by input into this pipeline.               */
/*            It can receive Commands over this pipeline                      */
/*            and acts accordingly to process them.                           */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void CI_Lab_AppMain(void)
{
    int32            status;
    uint32           RunStatus = CFE_ES_RunStatus_APP_RUN;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(CI_LAB_MAIN_TASK_PERF_ID);

    CI_LAB_TaskInit();

    /*
    ** CI Runloop
    */
    while (CFE_ES_RunLoop(&RunStatus) == true)
    {
        CFE_ES_PerfLogExit(CI_LAB_MAIN_TASK_PERF_ID);

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, CI_LAB_Global.CommandPipe, 500);

        CFE_ES_PerfLogEntry(CI_LAB_MAIN_TASK_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            CI_LAB_ProcessCommandPacket(SBBufPtr);
        }

        /* Regardless of packet vs timeout, always process uplink queue      */
        if (CI_LAB_Global.SocketConnected)
        {
            CI_LAB_ReadUpLink();
        }
    }

    CFE_ES_ExitApp(RunStatus);

} /* End of CI_Lab_AppMain() */

/*
** CI delete callback function.
** This function will be called in the event that the CI app is killed.
** It will close the network socket for CI
*/
void CI_LAB_delete_callback(void)
{
    OS_printf("CI delete callback -- Closing CI Network socket.\n");
    OS_close(CI_LAB_Global.SocketID);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* CI_LAB_TaskInit() -- CI initialization                                     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void CI_LAB_TaskInit(void)
{
    int32  status;
    uint16 DefaultListenPort;

    memset(&CI_LAB_Global, 0, sizeof(CI_LAB_Global));

    CFE_EVS_Register(CI_LAB_EventFilters, sizeof(CI_LAB_EventFilters) / sizeof(CFE_EVS_BinFilter_t),
                     CFE_EVS_EventFilter_BINARY);

    CFE_SB_CreatePipe(&CI_LAB_Global.CommandPipe, CI_LAB_PIPE_DEPTH, "CI_LAB_CMD_PIPE");
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CI_LAB_CMD_MID), CI_LAB_Global.CommandPipe);
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CI_LAB_SEND_HK_MID), CI_LAB_Global.CommandPipe);

    status = OS_SocketOpen(&CI_LAB_Global.SocketID, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
    if (status != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(CI_LAB_SOCKETCREATE_ERR_EID, CFE_EVS_EventType_ERROR, "CI: create socket failed = %d",
                          (int)status);
    }
    else
    {
        OS_SocketAddrInit(&CI_LAB_Global.SocketAddress, OS_SocketDomain_INET);
        DefaultListenPort = CI_LAB_BASE_UDP_PORT + CFE_PSP_GetProcessorId() - 1;
        OS_SocketAddrSetPort(&CI_LAB_Global.SocketAddress, DefaultListenPort);

        status = OS_SocketBind(CI_LAB_Global.SocketID, &CI_LAB_Global.SocketAddress);

        if (status != OS_SUCCESS)
        {
            CFE_EVS_SendEvent(CI_LAB_SOCKETBIND_ERR_EID, CFE_EVS_EventType_ERROR, "CI: bind socket failed = %d",
                              (int)status);
        }
        else
        {
            CI_LAB_Global.SocketConnected = true;
            CFE_ES_WriteToSysLog("CI_LAB listening on UDP port: %u\n", (unsigned int)DefaultListenPort);
        }
    }

    CI_LAB_ResetCounters_Internal();

    /*
    ** Install the delete handler
    */
    OS_TaskInstallDeleteHandler(&CI_LAB_delete_callback);

    CFE_MSG_Init(CFE_MSG_PTR(CI_LAB_Global.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(CI_LAB_HK_TLM_MID),
                 sizeof(CI_LAB_Global.HkTlm));

    CFE_EVS_SendEvent(CI_LAB_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "CI Lab Initialized.%s",
                      CI_LAB_VERSION_STRING);

} /* End of CI_LAB_TaskInit() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*  Name:  CI_LAB_Noop                                                         */
/*                                                                             */
/*  Purpose:                                                                   */
/*     Handle NOOP command packets                                             */
/*                                                                             */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 CI_LAB_Noop(const CI_LAB_NoopCmd_t *data)
{
    /* Does everything the name implies */
    CI_LAB_Global.HkTlm.Payload.CommandCounter++;

    CFE_EVS_SendEvent(CI_LAB_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION, "CI: NOOP command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*  Name:  CI_LAB_ResetCounters                                                */
/*                                                                             */
/*  Purpose:                                                                   */
/*     Handle ResetCounters command packets                                    */
/*                                                                             */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 CI_LAB_ResetCounters(const CI_LAB_ResetCountersCmd_t *data)
{
    CFE_EVS_SendEvent(CI_LAB_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION, "CI: RESET command");
    CI_LAB_ResetCounters_Internal();
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  CI_LAB_ReportHousekeeping                                          */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the CI task  */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 CI_LAB_ReportHousekeeping(const CFE_MSG_CommandHeader_t *data)
{
    CI_LAB_Global.HkTlm.Payload.SocketConnected = CI_LAB_Global.SocketConnected;
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(CI_LAB_Global.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(CI_LAB_Global.HkTlm.TelemetryHeader), true);
    return CFE_SUCCESS;

} /* End of CI_LAB_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  CI_LAB_ResetCounters_Internal                                      */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void CI_LAB_ResetCounters_Internal(void)
{
    /* Status of commands processed by CI task */
    CI_LAB_Global.HkTlm.Payload.CommandCounter      = 0;
    CI_LAB_Global.HkTlm.Payload.CommandErrorCounter = 0;

    /* Status of packets ingested by CI task */
    CI_LAB_Global.HkTlm.Payload.IngestPackets = 0;
    CI_LAB_Global.HkTlm.Payload.IngestErrors  = 0;

    return;

} /* End of CI_LAB_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* CI_LAB_ReadUpLink() --                                                     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void CI_LAB_ReadUpLink(void)
{
    int                                   i;
    int32                                 status;
    uint32                                BitSize;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Listener_Component_t           ListenerParams;
    EdsLib_DataTypeDB_TypeInfo_t          CmdHdrInfo;
    EdsLib_DataTypeDB_TypeInfo_t          FullCmdInfo;
    EdsLib_Id_t                           EdsId;
    CFE_SB_Buffer_t *                     NextIngestBufPtr;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    NextIngestBufPtr = NULL;
    EdsId            = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), CFE_HDR_CommandHeader_DATADICTIONARY);
    status           = EdsLib_DataTypeDB_GetTypeInfo(EDS_DB, EdsId, &CmdHdrInfo);
    if (status != EDSLIB_SUCCESS)
    {
        OS_printf("EdsLib_DataTypeDB_GetTypeInfo(): %d\n", (int)status);
        return;
    }

    for (i = 0; i <= 10; i++)
    {
        status = OS_SocketRecvFrom(CI_LAB_Global.SocketID, CI_LAB_Global.NetworkBuffer,
                                   sizeof(CI_LAB_Global.NetworkBuffer), &CI_LAB_Global.SocketAddress, OS_CHECK);

        if (status >= 0)
        {
            BitSize = status;
            BitSize *= 8;
        }
        else
        {
            BitSize = 0;
        }

        if (BitSize >= CmdHdrInfo.Size.Bits)
        {
            if (NextIngestBufPtr == NULL)
            {
                NextIngestBufPtr = CFE_SB_AllocateMessageBuffer(sizeof(CFE_HDR_CommandHeader_Buffer_t));
                if (NextIngestBufPtr == NULL)
                {
                    CFE_EVS_SendEvent(CI_LAB_INGEST_ALLOC_ERR_EID, CFE_EVS_EventType_ERROR,
                                      "CI: L%d, buffer allocation failed\n", __LINE__);
                    break;
                }
            }

            /* Packet is in external wire-format byte order - unpack it and copy */
            EdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), CFE_HDR_CommandHeader_DATADICTIONARY);
            status =
                EdsLib_DataTypeDB_UnpackPartialObject(EDS_DB, &EdsId, NextIngestBufPtr, CI_LAB_Global.NetworkBuffer,
                                                      sizeof(CFE_HDR_CommandHeader_Buffer_t), BitSize, 0);
            if (status != EDSLIB_SUCCESS)
            {
                OS_printf("EdsLib_DataTypeDB_UnpackPartialObject(1): %d\n", (int)status);
                break;
            }

            /* Header decoded successfully - Now need to determine the type for the rest of the payload */
            CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &NextIngestBufPtr->Msg.BaseMsg);
            CFE_MissionLib_UnmapListenerComponent(&ListenerParams, &PubSubParams);

            status = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telecommand_Interface_ID,
                                                    ListenerParams.Telecommand.TopicId, 1, 1, &EdsId);
            if (status != CFE_MISSIONLIB_SUCCESS)
            {
                OS_printf("CFE_MissionLib_GetArgumentType(): %d\n", (int)status);
                break;
            }

            status = EdsLib_DataTypeDB_UnpackPartialObject(
                EDS_DB, &EdsId, NextIngestBufPtr, CI_LAB_Global.NetworkBuffer, sizeof(CFE_HDR_CommandHeader_Buffer_t),
                BitSize, sizeof(CFE_HDR_CommandHeader_t));
            if (status != EDSLIB_SUCCESS)
            {
                OS_printf("EdsLib_DataTypeDB_UnpackPartialObject(2): %d\n", (int)status);
                break;
            }

            /* Verify that the checksum and basic fields are correct, and recompute the length entry */
            status = EdsLib_DataTypeDB_VerifyUnpackedObject(
                EDS_DB, EdsId, NextIngestBufPtr, CI_LAB_Global.NetworkBuffer, EDSLIB_DATATYPEDB_RECOMPUTE_LENGTH);
            if (status != EDSLIB_SUCCESS)
            {
                OS_printf("EdsLib_DataTypeDB_VerifyUnpackedObject(): %d\n", (int)status);
                break;
            }

            status = EdsLib_DataTypeDB_GetTypeInfo(EDS_DB, EdsId, &FullCmdInfo);
            if (status != EDSLIB_SUCCESS)
            {
                OS_printf("EdsLib_DataTypeDB_GetTypeInfo(): %d\n", (int)status);
                return;
            }

            CFE_ES_PerfLogEntry(CI_LAB_SOCKET_RCV_PERF_ID);
            CI_LAB_Global.HkTlm.Payload.IngestPackets++;
            status = CFE_SB_TransmitBuffer(NextIngestBufPtr, false);
            CFE_ES_PerfLogExit(CI_LAB_SOCKET_RCV_PERF_ID);

            if (status == CFE_SUCCESS)
            {
                /* Set NULL so a new buffer will be obtained next time around */
                NextIngestBufPtr = NULL;
            }
            else
            {
                CFE_EVS_SendEvent(CI_LAB_INGEST_SEND_ERR_EID, CFE_EVS_EventType_ERROR,
                                  "CI: L%d, CFE_SB_TransmitBuffer() failed, status=%d\n", __LINE__, (int)status);
            }
        }
        else if (status > 0)
        {
            /* bad size, report as ingest error */
            CI_LAB_Global.HkTlm.Payload.IngestErrors++;

            CFE_EVS_SendEvent(CI_LAB_INGEST_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                              "CI: L%d, cmd dropped, bad length=%u bits\n", __LINE__, (unsigned int)BitSize);
        }
    }

    if (NextIngestBufPtr != NULL)
    {
        CFE_SB_ReleaseMessageBuffer(NextIngestBufPtr);
    }

    return;

} /* End of CI_LAB_ReadUpLink() */
