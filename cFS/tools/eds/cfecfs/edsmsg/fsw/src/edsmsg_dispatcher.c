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
#include "edslib_intfdb.h"
#include "common_types.h"
#include "cfe_error.h"
#include "cfe_sb_api_typedefs.h"
#include "cfe_msg.h"
#include "edsmsg_dispatcher.h"
#include "cfe_config.h"

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"

/*
 * A generic typedef for a dispatch function implementation
 * Note the actual implementation is going to have a more specific data type
 * for the argument, but it will always be a single const pointer.
 */
typedef int32 (*CFE_EDSMSG_DispatchFunc_t)(const CFE_SB_Buffer_t *);

/*----------------------------------------------------------------
 *
 * Local helper function
 * Finds topicinfo for a command message
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_EDSMSG_FindCmdTopicInfo(const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *PubSubParams,
                                         CFE_MissionLib_TopicInfo_t                     *TopicInfo)
{
    EdsComponent_CFE_SB_Listener_t ListenerParams;
    int32_t                        Status;

    CFE_MissionLib_UnmapListenerComponent(&ListenerParams, PubSubParams);

    Status = CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, ListenerParams.Telecommand.TopicId, TopicInfo);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_STATUS_UNKNOWN_MSG_ID;
    }
    else
    {
        return CFE_SUCCESS;
    }
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Finds topicinfo for a telemetry message
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_EDSMSG_FindTlmTopicInfo(const EdsInterface_CFE_SB_SoftwareBus_PubSub_t *PubSubParams,
                                         CFE_MissionLib_TopicInfo_t                     *TopicInfo)
{
    EdsComponent_CFE_SB_Publisher_t PublisherParams;
    int32_t                         Status;

    CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, PubSubParams);

    Status = CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, PublisherParams.Telemetry.TopicId, TopicInfo);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_STATUS_UNKNOWN_MSG_ID;
    }
    else
    {
        return CFE_SUCCESS;
    }
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Finds topicinfo for a message.  First find the type of message (TLM or CMD)
 * and then call the apprpriate sub-function to get the topic info
 *
 * If this fails, zero-fill the output because topicID 0 is never valid
 *
 *-----------------------------------------------------------------*/
void CFE_EDSMSG_Dispatch_GetTopicInfo(const CFE_SB_Buffer_t *Buffer, CFE_MissionLib_TopicInfo_t *TopicInfo)
{
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSubParams;
    const EdsDataType_CFE_HDR_Message_t     *Hdr;

    Hdr = (const EdsDataType_CFE_HDR_Message_t *)(const void *)&Buffer->Msg;

    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, Hdr);

    /* In general FSW will call this only on "Listener" interfaces (e.g. SB pipes where
     * cmds come in) as opposed to publisher interfaces (e.g. telemetry output), however
     * it is still theoretically possible to dispatch TLM */
    if (CFE_MissionLib_PubSub_IsListenerComponent(&PubSubParams))
    {
        CFE_EDSMSG_FindCmdTopicInfo(&PubSubParams, TopicInfo);
    }
    else if (CFE_MissionLib_PubSub_IsPublisherComponent(&PubSubParams))
    {
        CFE_EDSMSG_FindTlmTopicInfo(&PubSubParams, TopicInfo);
    }
    else
    {
        memset(&TopicInfo, 0, sizeof(TopicInfo));
    }
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Confirms that the topic matches the expected component interface
 *
 *-----------------------------------------------------------------*/
bool CFE_EDSMSG_Dispatch_CheckComponentMatch(EdsLib_Id_t ComponentId, CFE_MissionLib_TopicInfo_t *TopicInfo)
{
    const EdsLib_DatabaseObject_t *GD;
    EdsLib_IntfDB_InterfaceInfo_t  IntfInfo;
    int32_t                        Status;

    GD = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    Status = EdsLib_IntfDB_GetComponentInterfaceInfo(GD, TopicInfo->ParentIntfId, &IntfInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return false;
    }

    /* The interfaces must match - if they do not match it means this dispatch table cannot be used */
    return (EdsLib_Is_Similar(ComponentId, IntfInfo.ParentCompEdsId));
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Finds the EdsId for the argument (message data type)
 * This just finds the base type using the information in the EDS DB only
 *
 *-----------------------------------------------------------------*/
CFE_Status_t
CFE_EDSMSG_Dispatch_FindArgBaseType(EdsLib_Id_t DeclIntfId, EdsLib_Id_t CompIntfId, EdsLib_Id_t *ArgTypeBuf)
{
    const EdsLib_DatabaseObject_t *GD;
    EdsLib_Id_t                    CommandEdsId;
    int32_t                        Status;

    GD = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    /* The SB interfaces are split between "publisher" and "listener" such that
     * there is expected to be just a singluar command associated with the I/F */
    Status = EdsLib_IntfDB_FindAllCommands(GD, DeclIntfId, &CommandEdsId, 1);
    if (Status != EDSLIB_SUCCESS)
    {
        return CFE_STATUS_VALIDATION_FAILURE;
    }

    /* Likewise, the single command should have a singular argument */
    Status = EdsLib_IntfDB_FindAllArgumentTypes(GD, CommandEdsId, CompIntfId, ArgTypeBuf, 1);
    if (Status != EDSLIB_SUCCESS)
    {
        return CFE_STATUS_VALIDATION_FAILURE;
    }

    return CFE_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Finds the EdsId for the argument (message data type)
 *
 * This uses the constraints and values within the message (e.g. command code)
 * to identify a derived data type.  This is the final identification step and
 * once the type is fully identified, the size of the message should match its
 * expected/defined size in EDS
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_EDSMSG_Dispatch_CheckActualBufferType(const CFE_SB_Buffer_t *Buffer,
                                                       EdsLib_Id_t           *EdsId,
                                                       uint32_t              *DispatchTblPosition)
{
    const EdsLib_DatabaseObject_t           *GD;
    EdsLib_DataTypeDB_TypeInfo_t             TypeInfo;
    EdsLib_DataTypeDB_DerivativeObjectInfo_t DerivObjInfo;
    int32_t                                  Status;
    CFE_MSG_Size_t                           MessageSize;
    CFE_Status_t                             ReturnCode;

    GD = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    CFE_MSG_GetSize(&Buffer->Msg, &MessageSize);

    /* Check if the argument type has derivatives.  This is typical for CMD interfaces where there
     * are many possible command codes, and in this case each command code will have its own
     * entry in the dispatch table. If this fails, it does not fail the overall process, it just
     * means that the argument is not derived.  */
    Status = EdsLib_DataTypeDB_IdentifyBufferWithSize(GD, *EdsId, Buffer, MessageSize, &DerivObjInfo);
    if (Status == EDSLIB_SUCCESS)
    {
        *EdsId               = DerivObjInfo.EdsId;
        *DispatchTblPosition = DerivObjInfo.DerivativeTableIndex;
    }
    else
    {
        /* Non derived, there is just one entry, it is always first */
        *DispatchTblPosition = 0;
    }

    /* This lookup should not fail */
    Status = EdsLib_DataTypeDB_GetTypeInfo(GD, *EdsId, &TypeInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        ReturnCode = CFE_SB_INTERNAL_ERR;
    }
    else if (TypeInfo.Size.Bytes != MessageSize)
    {
        ReturnCode = CFE_STATUS_WRONG_MSG_LENGTH;
    }
    else
    {
        /* Everything checked out */
        ReturnCode = CFE_SUCCESS;
    }

    return ReturnCode;
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Completely look up the data type from the supplied interface IDs
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_EDSMSG_Dispatch_FindArgType(EdsLib_Id_t            DeclIntfId,
                                             EdsLib_Id_t            ParentIntfId,
                                             const CFE_SB_Buffer_t *Buffer,
                                             uint32_t              *DispatchPosition)
{
    CFE_Status_t ReturnCode;
    EdsLib_Id_t  ArgType;

    *DispatchPosition = 0;

    /* First lookup is to simply identify the base type for this intf */
    /* This is independent of the message content, strictly based on the I/F */
    ReturnCode = CFE_EDSMSG_Dispatch_FindArgBaseType(DeclIntfId, ParentIntfId, &ArgType);
    if (ReturnCode == CFE_SUCCESS)
    {
        /* Second lookup is to check if its a derived type, and also confirm the size matches.
         * This requires looking at fields within the actual msg */
        ReturnCode = CFE_EDSMSG_Dispatch_CheckActualBufferType(Buffer, &ArgType, DispatchPosition);
    }

    return ReturnCode;
}

/*----------------------------------------------------------------
 *
 * Local helper function
 * Finds the matching handler function from the dispatch table
 *
 *-----------------------------------------------------------------*/
CFE_EDSMSG_DispatchFunc_t
CFE_EDSMSG_Dispatch_LookupHandler(const void *DispatchTable, size_t DispatchStartOffset, uint32_t DispatchPosition)
{
    const uint8                     *MemAddr;
    const CFE_EDSMSG_DispatchFunc_t *DispatchTbl;

    /* calculate the address of the first handler routine for this intf
     * Note the START offsets are in bytes, so this must be done in uint8* logic */
    MemAddr  = (const uint8 *)DispatchTable;
    MemAddr += DispatchStartOffset;

    /* now shift to the correct type (function pointer) and apply the position offset */
    DispatchTbl = (const CFE_EDSMSG_DispatchFunc_t *)MemAddr;

    return (DispatchTbl[DispatchPosition]);
}

/*----------------------------------------------------------------
 *
 * Public API call
 * Dispatch the message based on the IDs and dispatch table
 *
 *-----------------------------------------------------------------*/
CFE_Status_t CFE_EDSMSG_Dispatch(EdsLib_Id_t            DeclIntfId,
                                 EdsLib_Id_t            ComponentId,
                                 const CFE_SB_Buffer_t *Buffer,
                                 const void            *DispatchTable)
{
    CFE_Status_t               ReturnCode;
    CFE_MissionLib_TopicInfo_t TopicInfo;
    uint32_t                   DispatchPosition;
    CFE_EDSMSG_DispatchFunc_t  Handler;

    DispatchPosition = 0;
    Handler          = NULL;
    CFE_EDSMSG_Dispatch_GetTopicInfo(Buffer, &TopicInfo);

    /* Check that the dispatch table matches this buffer (they map to the same EDS component)
     * Otherwise it means there is a mismatch between the message and the
     * dispatch table type and this message cannot be dispatched using this table */
    if (CFE_EDSMSG_Dispatch_CheckComponentMatch(ComponentId, &TopicInfo))
    {
        ReturnCode = CFE_EDSMSG_Dispatch_FindArgType(DeclIntfId, TopicInfo.ParentIntfId, Buffer, &DispatchPosition);
    }
    else
    {
        ReturnCode = CFE_STATUS_VALIDATION_FAILURE;
    }

    if (ReturnCode == CFE_SUCCESS)
    {
        Handler = CFE_EDSMSG_Dispatch_LookupHandler(DispatchTable, TopicInfo.DispatchStartOffset, DispatchPosition);
    }

    if (ReturnCode == CFE_SUCCESS)
    {
        /* Now actually call the handler, if its defined */
        if (Handler != NULL)
        {
            ReturnCode = Handler(Buffer);
        }
        else
        {
            /* this means there was no entry in the table for this cmd */
            ReturnCode = CFE_STATUS_NOT_IMPLEMENTED;
        }
    }

    return ReturnCode;
}
