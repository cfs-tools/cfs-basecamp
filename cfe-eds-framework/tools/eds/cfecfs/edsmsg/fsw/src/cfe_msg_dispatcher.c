/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/*
** Include Files
*/
#include <stdlib.h>
#include <string.h>

#include "edslib_datatypedb.h"
#include "common_types.h"
#include "cfe_error.h"
#include "cfe_sb_api_typedefs.h"
#include "cfe_msg.h"
#include "cfe_config.h"

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"

CFE_Status_t CFE_MSG_EdsDispatch(uint16 InterfaceID, uint16 IndicationIndex, uint16 DispatchTableID,
                                 const CFE_SB_Buffer_t *Buffer, const void *DispatchTable)
{
    const EdsLib_DatabaseObject_t *       GD;
    CFE_MissionLib_TopicInfo_t            TopicInfo;
    CFE_MissionLib_IndicationInfo_t       IndicationInfo;
    EdsLib_DataTypeDB_TypeInfo_t          TypeInfo;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    EdsLib_Id_t                           ArgumentType;
    int32_t                               Status;
    uint16_t                              TopicId;
    uint16_t                              DispatchOffset;
    CFE_MSG_Size_t                        BufferSize;
    union
    {
        cpuaddr     MemAddr;
        const void *GenericPtr;
        int32 (**DispatchFunc)(const CFE_SB_Buffer_t *);
    } HandlerPtr;

    GD = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    CFE_MSG_GetSize(&Buffer->Msg, &BufferSize);

    HandlerPtr.GenericPtr = DispatchTable;
    DispatchOffset        = 0;
    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &Buffer->Msg.BaseMsg);

    switch (InterfaceID)
    {
        case CFE_SB_Telemetry_Interface_ID:
        {
            CFE_SB_Publisher_Component_t PublisherParams;
            CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);
            TopicId = PublisherParams.Telemetry.TopicId;
            break;
        }
        case CFE_SB_Telecommand_Interface_ID:
        {
            CFE_SB_Listener_Component_t ListenerParams;
            CFE_MissionLib_UnmapListenerComponent(&ListenerParams, &PubSubParams);
            TopicId = ListenerParams.Telecommand.TopicId;
            break;
        }
        default:
            TopicId = 0;
            break;
    }

    Status = CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, InterfaceID, TopicId, &TopicInfo);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_STATUS_UNKNOWN_MSG_ID;
    }

    if (TopicInfo.DispatchTableId != DispatchTableID)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    Status = CFE_MissionLib_GetIndicationInfo(&CFE_SOFTWAREBUS_INTERFACE, InterfaceID, TopicId, IndicationIndex,
                                              &IndicationInfo);
    if (Status != CFE_MISSIONLIB_SUCCESS || IndicationInfo.NumArguments != 1)
    {
        /*
         * This dispatch function only handles single-argument commands defined in EDS.
         * This is really a programmer/EDS error and not a runtime error if this occurs
         */
        return CFE_SB_NOT_IMPLEMENTED;
    }

    Status = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, InterfaceID, TopicId, IndicationIndex, 1,
                                            &ArgumentType);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    if (IndicationInfo.SubcommandArgumentId == 1 && IndicationInfo.NumSubcommands > 0)
    {
        /* Derived command case -- the indication corresponds to a multiple entries in the dispatch table.
         * The actual type of the argument must be determined to figure out which one to invoke. */
        EdsLib_DataTypeDB_DerivativeObjectInfo_t DerivObjInfo;

        Status = EdsLib_DataTypeDB_IdentifyBuffer(GD, ArgumentType, Buffer->Msg.Byte, &DerivObjInfo);
        if (Status != EDSLIB_SUCCESS)
        {
            return CFE_STATUS_UNKNOWN_MSG_ID;
        }

        /* NOTE: The "IdentifyBuffer" outputs a 0-based index, but the Subcommand is a 1-based index */
        Status = CFE_MissionLib_GetSubcommandOffset(&CFE_SOFTWAREBUS_INTERFACE, InterfaceID, TopicId, IndicationIndex,
                                                    1 + DerivObjInfo.DerivativeTableIndex, &DispatchOffset);
        if (Status != EDSLIB_SUCCESS)
        {
            return CFE_STATUS_BAD_COMMAND_CODE;
        }

        ArgumentType = DerivObjInfo.EdsId;
    }

    Status = EdsLib_DataTypeDB_GetTypeInfo(GD, ArgumentType, &TypeInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    if (TypeInfo.Size.Bytes > BufferSize)
    {
        return CFE_STATUS_WRONG_MSG_LENGTH;
    }

    HandlerPtr.MemAddr += TopicInfo.DispatchStartOffset;
    HandlerPtr.MemAddr += DispatchOffset;
    if (*HandlerPtr.DispatchFunc == NULL)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    return (*HandlerPtr.DispatchFunc)(Buffer);
}
