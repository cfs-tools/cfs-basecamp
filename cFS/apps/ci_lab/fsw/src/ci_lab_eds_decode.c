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
 *   This file contains the source code for the Command Ingest task.
 */

/*
**   Include Files:
*/

#include "ci_lab_app.h"
#include "ci_lab_perfids.h"
#include "ci_lab_msgids.h"
#include "ci_lab_version.h"

#include "cfe_config.h"

#include "edslib_global.h"
#include "edslib_datatypedb.h"
#include "edslib_intfdb.h"
#include "ci_lab_eds_typedefs.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include "cfe_hdr_eds_datatypes.h"

/*
 * ---------------------------------------
 * In an EDS configuration - the data from the network is encoded
 * and needs to be read into an intermediate buffer first
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_GetInputBuffer(void **BufferOut, size_t *SizeOut)
{
    static EdsPackedBuffer_CFE_HDR_CommandHeader_t InputBuffer;

    *BufferOut = &InputBuffer;
    *SizeOut   = sizeof(InputBuffer);

    return CFE_SUCCESS;
}

/*
 * ---------------------------------------
 * Helper function: Decodes the header from the incoming message
 * All incoming messages _MUST_ have the standard command header, even if
 * the app the command is associated with is not in EDS.
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_DecodeInputHeader(CFE_SB_Buffer_t         *IngestBufPtr,
                                      const void              *SourceBuffer,
                                      const EdsLib_SizeInfo_t *MaxSize,
                                      EdsLib_SizeInfo_t       *ProcessedSize)
{
    EdsLib_Id_t                  EdsId;
    int32                        EdsStatus;
    EdsLib_DataTypeDB_TypeInfo_t CmdHdrInfo;
    CFE_Status_t                 ResultStatus;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    EdsId     = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), EdsContainer_CFE_HDR_CommandHeader_DATADICTIONARY);
    EdsStatus = EdsLib_DataTypeDB_GetTypeInfo(EDS_DB, EdsId, &CmdHdrInfo);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        OS_printf("EdsLib_DataTypeDB_GetTypeInfo(): %d\n", (int)EdsStatus);
        ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    else if (MaxSize->Bits < CmdHdrInfo.Size.Bits)
    {
        /* sanity check - validate the incoming packet is at least the size of a command header */
        OS_printf("CI_LAB: Size mismatch, BitSize=%lu (packet) / %lu (min)\n",
                  (unsigned long)MaxSize->Bits,
                  (unsigned long)CmdHdrInfo.Size.Bits);

        ResultStatus = CFE_STATUS_WRONG_MSG_LENGTH;
    }
    else
    {
        /* Packet is in external wire-format byte order - unpack it and copy */
        EdsStatus = EdsLib_DataTypeDB_UnpackPartialObjectVarSize(EDS_DB,
                                                                 &EdsId,
                                                                 IngestBufPtr,
                                                                 SourceBuffer,
                                                                 MaxSize,
                                                                 ProcessedSize);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_UnpackPartialObject(Header): %d\n", (int)EdsStatus);
            ResultStatus = CFE_STATUS_VALIDATION_FAILURE;
        }
        else
        {
            ResultStatus = CFE_SUCCESS;
        }
    }

    return ResultStatus;
}

/*
 * ---------------------------------------
 * Helper function: Decodes the payload from the incoming message
 * This is only possible if the app has an EDS.
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_DecodeInputPayload(CFE_SB_Buffer_t         *IngestBufPtr,
                                       EdsLib_Id_t              ParentIntfId,
                                       const void              *SourceBuffer,
                                       const EdsLib_SizeInfo_t *MaxSize,
                                       EdsLib_SizeInfo_t       *ProcessedSize)
{
    EdsLib_Id_t  EdsId;
    int32        EdsStatus;
    CFE_Status_t ResultStatus;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    static const EdsLib_Id_t CFE_SB_TELECOMMAND_ID =
        EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsCommand_CFE_SB_Telecommand_indication_DECLARATION);

    ResultStatus = CFE_SUCCESS;

    EdsStatus = EdsLib_IntfDB_FindAllArgumentTypes(EDS_DB, CFE_SB_TELECOMMAND_ID, ParentIntfId, &EdsId, 1);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        OS_printf("EdsLib_IntfDB_FindAllArgumentTypes(): %d\n", (int)EdsStatus);
        ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    else
    {
        /* Note, this is invoked even if the message is already fully decoded, as it also serves
         * to identify the message and check against defined constraints.
         * For example, on the CI_LAB_CMD interface, the previous lookup would have
         * yielded the EdsId of CI_LAB/CommandBase, but this will update the EdsId
         * to e.g. CI_LAB/NoopCmd (with NoopCmd being the same size as CommandBase) */
        EdsStatus = EdsLib_DataTypeDB_UnpackPartialObjectVarSize(EDS_DB,
                                                                 &EdsId,
                                                                 IngestBufPtr,
                                                                 SourceBuffer,
                                                                 MaxSize,
                                                                 ProcessedSize);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_UnpackPartialObject(Payload): %d\n", (int)EdsStatus);
            ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }
    }

    if (ResultStatus == CFE_SUCCESS)
    {
        /* Verify that the checksum and basic fields are correct, and recompute the length entry */
        EdsStatus = EdsLib_DataTypeDB_VerifyUnpackedObjectVarSize(EDS_DB,
                                                                  EdsId,
                                                                  IngestBufPtr,
                                                                  SourceBuffer,
                                                                  EDSLIB_DATATYPEDB_RECOMPUTE_LENGTH,
                                                                  ProcessedSize);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_VerifyUnpackedObject(): %d\n", (int)EdsStatus);
            ResultStatus = CFE_STATUS_VALIDATION_FAILURE;
        }
    }

    return ResultStatus;
}

/*
 * ---------------------------------------
 * Helper function: Passes through the payload from the incoming message
 * This is the historical behavior and may be used as a fallback option
 * if the application does not have an EDS.  The bits will be copied verbatim.
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_PassthruInputPayload(CFE_SB_Buffer_t         *IngestBufPtr,
                                         const void              *SourceBuffer,
                                         const EdsLib_SizeInfo_t *MaxSize,
                                         EdsLib_SizeInfo_t       *ProcessedSize)
{
    uint8       *Dest;
    const uint8 *Src;
    size_t       HeaderSize;
    size_t       TotalSize;
    CFE_Status_t ResultStatus;

    HeaderSize = EdsLib_BITS_TO_OCTETS(ProcessedSize->Bits);
    TotalSize  = EdsLib_BITS_TO_OCTETS(MaxSize->Bits);

    if ((TotalSize - HeaderSize) > (MaxSize->Bytes - ProcessedSize->Bytes))
    {
        /* will not fit in dest buffer */
        ResultStatus = CFE_SB_MSG_TOO_BIG;
    }
    else
    {
        /* If necessary, copy the payload.  Some commands (e.g. Noop) do not have a payload */
        if (TotalSize > HeaderSize)
        {
            Dest = (uint8 *)&IngestBufPtr->Msg;
            Src  = (const uint8 *)SourceBuffer;

            Dest += ProcessedSize->Bytes;
            Src  += HeaderSize;

            memcpy(Dest, Src, TotalSize - HeaderSize);
        }

        /* Need to adjust the length, because the header was decoded based on EDS */
        TotalSize -= HeaderSize;
        TotalSize += ProcessedSize->Bytes;

        ResultStatus = CFE_MSG_SetSize(&IngestBufPtr->Msg, TotalSize);
    }

    return ResultStatus;
}

/*
 * ---------------------------------------
 * In an EDS configuration - the data from the network is encoded
 * and this function translates the intermediate buffer content
 * to an instance of the CFE_SB_Buffer_t that can be sent to SB.
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_DecodeInputMessage(void *SourceBuffer, size_t SourceSize, CFE_SB_Buffer_t **DestBufferOut)
{
    int32                                 EdsStatus;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Listener_Component_t           ListenerParams;
    CFE_MissionLib_TopicInfo_t            TopicInfo;
    CFE_SB_Buffer_t                      *IngestBufPtr;
    CFE_Status_t                          ResultStatus;
    EdsLib_SizeInfo_t                     MaxSize;
    EdsLib_SizeInfo_t                     ProcessedSize;

    memset(&MaxSize, 0, sizeof(MaxSize));
    memset(&ProcessedSize, 0, sizeof(ProcessedSize));

    /* Get a SB Buffer, as a place to put the decoded data */
    IngestBufPtr = CFE_SB_AllocateMessageBuffer(sizeof(EdsNativeBuffer_CFE_HDR_CommandHeader_t));
    if (IngestBufPtr == NULL)
    {
        CFE_EVS_SendEvent(CI_LAB_INGEST_ALLOC_ERR_EID, CFE_EVS_EventType_ERROR, "CI_LAB: buffer allocation failed\n");

        ResultStatus = CFE_SB_BUF_ALOC_ERR;
    }
    else
    {
        MaxSize.Bits  = EdsLib_OCTETS_TO_BITS(SourceSize);
        MaxSize.Bytes = sizeof(EdsNativeBuffer_CFE_HDR_CommandHeader_t);
        ResultStatus  = CI_LAB_DecodeInputHeader(IngestBufPtr, SourceBuffer, &MaxSize, &ProcessedSize);
    }

    if (ResultStatus == CFE_SUCCESS)
    {
        /* Header decoded successfully - Now need to determine the type for the rest of the payload */
        CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &IngestBufPtr->Msg.BaseMsg);
        CFE_MissionLib_UnmapListenerComponent(&ListenerParams, &PubSubParams);

        /* This determines if there is an EDS mapping for the messge topic */
        EdsStatus =
            CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, ListenerParams.Telecommand.TopicId, &TopicInfo);
        if (EdsStatus != CFE_MISSIONLIB_SUCCESS)
        {
            OS_printf("CFE_MissionLib_GetTopicInfo(): %d\n", (int)EdsStatus);
            ResultStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }
        else if (EdsLib_Is_Valid(TopicInfo.ParentIntfId))
        {
            /* Decode the payload based on EDS */
            ResultStatus =
                CI_LAB_DecodeInputPayload(IngestBufPtr, TopicInfo.ParentIntfId, SourceBuffer, &MaxSize, &ProcessedSize);
        }
        else if (CI_LAB_Global.AllowPassthrough)
        {
            /* No EDS mapping, Just copy the bits */
            ResultStatus = CI_LAB_PassthruInputPayload(IngestBufPtr, SourceBuffer, &MaxSize, &ProcessedSize);
        }
        else
        {
            OS_printf("CI_LAB_DecodeInputMessage(): No EDS definition for TopicId=%u\n",
                      (unsigned int)ListenerParams.Telecommand.TopicId);
            ResultStatus = CFE_STATUS_VALIDATION_FAILURE;
        }
    }

    if (ResultStatus != CFE_SUCCESS && IngestBufPtr != NULL)
    {
        CFE_SB_ReleaseMessageBuffer(IngestBufPtr);
        IngestBufPtr = NULL;
    }

    *DestBufferOut = IngestBufPtr;

    return ResultStatus;
}
