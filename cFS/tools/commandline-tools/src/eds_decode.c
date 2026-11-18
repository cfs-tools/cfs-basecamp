/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 *
 * Copyright (c) 2020 United States Government as represented by
 * the Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file     tlm_decode.c
 * \ingroup  cfecfs
 * \author   joseph.p.hickey@nasa.gov
 *
 * Read and display UDP telemetry packets
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */

#include <cfe_mission_cfg.h>
#include "cfe_sb_eds_datatypes.h"
#include "cfe_hdr_eds_datatypes.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_global.h"
#include "edslib_displaydb.h"
#include "edslib_intfdb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"

#include "tlm_recv.h"

typedef struct EDS_Decoder
{
    EdsNativeBuffer_CFE_HDR_TelemetryHeader_t LocalBuffer;

} EDS_Decoder_t;

static const EdsLib_Id_t CFE_SB_TELEMETRY_CMD_ID =
    EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsCommand_CFE_SB_Telemetry_indication_DECLARATION);

/*----------------------------------------------------------------
 *
 * Helper function
 *
 *-----------------------------------------------------------------*/
void EDS_DecodeDisplay(void *Arg, const EdsLib_EntityDescriptor_t *Param)
{
    uint8_t *BasePtr;
    char     OutputBuffer[256];

    BasePtr  = (uint8_t *)Arg;
    BasePtr += Param->EntityInfo.Offset.Bytes;
    EdsLib_Scalar_ToString(&EDS_DATABASE, Param->EntityInfo.EdsId, OutputBuffer, sizeof(OutputBuffer), BasePtr);
    printf("%s(): Bit=%-4d %35s = %s\n", __func__, Param->EntityInfo.Offset.Bits, Param->FullName, OutputBuffer);
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
void *EDS_Instantiate(void)
{
    EDS_Decoder_t *obj;

    obj = malloc(sizeof(*obj));
    memset(obj, 0, sizeof(*obj));

    return obj;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
bool EDS_DisplayObject(void *arg, const void *Data, size_t Size)
{
    EDS_Decoder_t                           *tlm = arg;
    EdsLib_Id_t                              EdsId;
    EdsLib_DataTypeDB_TypeInfo_t             TypeInfo;
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSubParams;
    EdsComponent_CFE_SB_Publisher_t          PublisherParams;
    CFE_MissionLib_TopicInfo_t               TopicInfo;
    char                                     TempBuffer[64];
    int32_t                                  Status;

    EdsLib_SizeInfo_t ProcessedSize;
    EdsLib_SizeInfo_t MaxSize;

    memset(&ProcessedSize, 0, sizeof(ProcessedSize));
    memset(&MaxSize, 0, sizeof(MaxSize));

    ConsoleUtils_Hexdump(Data, Size);

    EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), EdsContainer_CFE_HDR_TelemetryHeader_DATADICTIONARY);
    Status = EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, EdsId, &TypeInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return false;
    }

    MaxSize.Bits  = EdsLib_OCTETS_TO_BITS(Size);
    MaxSize.Bytes = sizeof(tlm->LocalBuffer);

    Status = EdsLib_DataTypeDB_UnpackPartialObjectVarSize(&EDS_DATABASE,
                                                          &EdsId,
                                                          tlm->LocalBuffer.Byte,
                                                          Data,
                                                          &MaxSize,
                                                          &ProcessedSize);
    if (Status != EDSLIB_SUCCESS)
    {
        return false;
    }

    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &tlm->LocalBuffer.BaseObject.Message);
    CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);

    Status = CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, PublisherParams.Telemetry.TopicId, &TopicInfo);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return false;
    }

    Status =
        EdsLib_IntfDB_FindAllArgumentTypes(&EDS_DATABASE, CFE_SB_TELEMETRY_CMD_ID, TopicInfo.ParentIntfId, &EdsId, 1);
    if (Status != EDSLIB_SUCCESS)
    {
        return false;
    }

    Status = EdsLib_DataTypeDB_UnpackPartialObjectVarSize(&EDS_DATABASE,
                                                          &EdsId,
                                                          tlm->LocalBuffer.Byte,
                                                          Data,
                                                          &MaxSize,
                                                          &ProcessedSize);
    if (Status != EDSLIB_SUCCESS)
    {
        return false;
    }

    CONSOLEUTILS_INFO("Formatcode=%08lx / %s\n",
                      (unsigned long)EdsId,
                      EdsLib_DisplayDB_GetTypeName(&EDS_DATABASE, EdsId, TempBuffer, sizeof(TempBuffer)));

    Status = EdsLib_DataTypeDB_VerifyUnpackedObject(&EDS_DATABASE,
                                                    EdsId,
                                                    tlm->LocalBuffer.Byte,
                                                    Data,
                                                    EDSLIB_DATATYPEDB_RECOMPUTE_NONE);
    if (Status != EDSLIB_SUCCESS)
    {
        CONSOLEUTILS_ERROR("NOTE - EDS VERIFICATION FAILED: code=%d\n", (int)Status);
    }

    EdsLib_DisplayDB_IterateAllEntities(&EDS_DATABASE, EdsId, EDS_DecodeDisplay, tlm->LocalBuffer.Byte);

    return true;
}

/*----------------------------------------------------------------
 *
 * API structure
 *
 *-----------------------------------------------------------------*/
/* clang-format off */
const TlmRecv_Display_API_t EDS_API =
{
    .Name = "EDS",
    .Instantiate = EDS_Instantiate,
    .DisplayObject = EDS_DisplayObject
};
