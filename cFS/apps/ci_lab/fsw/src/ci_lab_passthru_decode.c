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
#include "cfe.h"

#include "ci_lab_app.h"
#include "ci_lab_perfids.h"
#include "ci_lab_msgids.h"
#include "ci_lab_decode.h"

/*
 * ---------------------------------------
 * In a "passthrough" configuration - the data from the network
 * is expected to be a direct instance of the CFE_MSG_Message_t base,
 * and thus something can be sent directly to SB.
 *
 * Instead of using an intermediate buffer, just get a buffer from
 * SB and put it directly in there.  This reduces copying.
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_GetInputBuffer(void **BufferOut, size_t *SizeOut)
{
    CFE_SB_Buffer_t *IngestBuffer;
    const size_t     IngestSize = CI_LAB_PLATFORM_MAX_INGEST;

    IngestBuffer = CFE_SB_AllocateMessageBuffer(IngestSize);
    if (IngestBuffer == NULL)
    {
        *BufferOut = NULL;
        *SizeOut   = 0;

        CFE_EVS_SendEvent(CI_LAB_INGEST_ALLOC_ERR_EID, CFE_EVS_EventType_ERROR, "CI_LAB: buffer allocation failed\n");

        return CFE_SB_BUF_ALOC_ERR;
    }

    *BufferOut = IngestBuffer;
    *SizeOut   = IngestSize;

    return CFE_SUCCESS;
}

/*
 * ---------------------------------------
 * In a "passthrough" configuration - the data from the network
 * is expected to be a direct instance of the CFE_MSG_Message_t base,
 * and thus something can be sent directly to SB.
 *
 * This just does a simple sanity check on the message size.  But
 * otherwise, the source buffer is used directly as the output buffer.
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_DecodeInputMessage(void *SourceBuffer, size_t SourceSize, CFE_SB_Buffer_t **DestBufferOut)
{
    CFE_SB_Buffer_t *MsgBufPtr;
    CFE_MSG_Size_t   MsgSize;
    CFE_Status_t     Status;

    if (SourceSize < sizeof(CFE_MSG_CommandHeader_t))
    {
        MsgBufPtr = NULL;
        Status    = CFE_STATUS_WRONG_MSG_LENGTH;

        CFE_EVS_SendEvent(CI_LAB_INGEST_LEN_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "CI: cmd dropped, bad packet length=%lu\n",
                          (unsigned long)SourceSize);
    }
    else
    {
        MsgBufPtr = SourceBuffer;
        MsgSize   = 0;

        /* Check the size from within the header itself, compare against network buffer size */
        Status = CFE_MSG_GetSize(&MsgBufPtr->Msg, &MsgSize);

        if ((Status == CFE_SUCCESS) && (MsgSize > SourceSize))
        {
            Status = CFE_STATUS_WRONG_MSG_LENGTH;

            CFE_EVS_SendEvent(CI_LAB_INGEST_LEN_ERR_EID,
                              CFE_EVS_EventType_ERROR,
                              "CI: cmd dropped - length mismatch, %lu (hdr) / %lu (packet)\n",
                              (unsigned long)MsgSize,
                              (unsigned long)SourceSize);
        }
    }

    *DestBufferOut = MsgBufPtr;

    return Status;
}
