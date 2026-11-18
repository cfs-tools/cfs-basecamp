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
 * \file     cmdUtil.c
 * \ingroup  cfecfs
 * \author   joseph.p.hickey@nasa.gov
 *
 ** cmdUtil -- A CCSDS Command utility. This program will build a CCSDS Command packet
 **               with variable parameters and send it on a UDP network socket.
 **               this program is primarily used to command a cFE flight software system.
 */

/*
** System includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "cmd_send.h"

#include <cfe_mission_cfg.h>

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_global.h"
#include "edslib_displaydb.h"
#include "edslib_intfdb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_hdr_eds_datatypes.h"

const char DEFAULT_COMPONENT[] = "Application";

static const EdsLib_Id_t CFE_SB_TELECOMMAND_CMD_ID =
    EDSLIB_INTF_ID(EDS_INDEX(CFE_SB), EdsCommand_CFE_SB_Telecommand_indication_DECLARATION);

#define OPTARG_SIZE        64
#define CONSTRAINT_BUFSIZE 32

typedef union CmdSend_NativeBuffer
{
    EdsDataType_CCSDS_SpacePacketBasic_t    Basic;
    EdsDataType_CCSDS_SpacePacketApidQ_t    ApidQ;
    EdsNativeBuffer_CFE_HDR_CommandHeader_t Cmd;
    uint8_t                                 Byte[16];
} CmdSend_NativeBuffer_t;

typedef enum
{
    EDS_LookupState_FAILED  = -1,
    EDS_LookupState_UNKNOWN = 0,
    EDS_LookupState_SUCCESS = 1
} EDS_LookupState_t;

/*
** Parameter datatype structure
*/
typedef struct
{
    char DestIntf[OPTARG_SIZE];
    char CmdName[OPTARG_SIZE];

    EDS_LookupState_t  IntfState;
    EDS_LookupState_t  PayloadState;
    bool               IsDerived;
    bool               PayloadError;
    CmdSend_Protocol_t SelectedProto;

    EdsLib_Id_t IntfEdsId;
    EdsLib_Id_t CmdEdsId;
    EdsLib_Id_t IntfArg;
    EdsLib_Id_t ActualArg;

    EdsComponent_CFE_SB_Listener_t           Params;
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSub;
    uint16_t                                 CommandCodeConstrIdx;
    EdsLib_DataTypeDB_EntityInfo_t           CommandCodeInfo;
    EdsLib_DataTypeDB_TypeInfo_t             EdsHeaderInfo;
    EdsLib_DataTypeDB_TypeInfo_t             EdsTypeInfo;
    EdsLib_DataTypeDB_EntityInfo_t           EdsPayloadInfo;

    CmdSend_NativeBuffer_t Buf;

    char LastErrorText[160];

} CommandData_t;

typedef struct
{
    EdsLib_Generic_UnsignedInt_t ValueCompare;
    bool                         IsMatch;

} EDS_ConstraintMatch_t;

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
static void Enumerate_Topics_Usage_Callback(void *Arg, uint16_t TopicId, EdsLib_Id_t IntfEdsId)
{
    char    NameBuffer[128];
    int32_t Status;

    Status = EdsLib_IntfDB_GetFullName(&EDS_DATABASE, IntfEdsId, NameBuffer, sizeof(NameBuffer));
    if (Status == EDSLIB_SUCCESS)
    {
        printf("   %s\n", NameBuffer);
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
static void Enumerate_Constraint_Callback(const EdsLib_DatabaseObject_t        *GD,
                                          const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
                                          EdsLib_GenericValueBuffer_t          *ConstraintValue,
                                          void                                 *Arg)
{
    char *Buffer = Arg;

    switch (ConstraintValue->ValueType)
    {
        case EDSLIB_BASICTYPE_SIGNED_INT:
            snprintf(Buffer, CONSTRAINT_BUFSIZE, "%lld", (long long)ConstraintValue->Value.SignedInteger);
            break;
        case EDSLIB_BASICTYPE_UNSIGNED_INT:
            snprintf(Buffer, CONSTRAINT_BUFSIZE, "%llu", (unsigned long long)ConstraintValue->Value.UnsignedInteger);
            break;
        case EDSLIB_BASICTYPE_BINARY:
            strncpy(Buffer, ConstraintValue->Value.StringData, CONSTRAINT_BUFSIZE - 1);
            Buffer[CONSTRAINT_BUFSIZE - 1] = 0;
            break;
        default:
            break;
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
static void Match_Constraint_Callback(const EdsLib_DatabaseObject_t        *GD,
                                      const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo,
                                      EdsLib_GenericValueBuffer_t          *ConstraintValue,
                                      void                                 *Arg)
{
    EDS_ConstraintMatch_t *Result = Arg;

    if (!Result->IsMatch)
    {
        switch (ConstraintValue->ValueType)
        {
            case EDSLIB_BASICTYPE_SIGNED_INT:
                Result->IsMatch = (ConstraintValue->Value.SignedInteger == Result->ValueCompare);
                break;
            case EDSLIB_BASICTYPE_UNSIGNED_INT:
                Result->IsMatch = (ConstraintValue->Value.UnsignedInteger == Result->ValueCompare);
                break;
            default:
                break;
        }
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
static void Enumerate_Members_Usage_Callback(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc)
{
    const char                  *Type;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    EdsLib_DisplayHint_t         DisplayHint;
    if (ParamDesc->FullName != NULL)
    {
        EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, ParamDesc->EntityInfo.EdsId, &TypeInfo);
        DisplayHint = EdsLib_DisplayDB_GetDisplayHint(&EDS_DATABASE, ParamDesc->EntityInfo.EdsId);

        switch (TypeInfo.ElemType)
        {
            case EDSLIB_BASICTYPE_SIGNED_INT:
                Type = "int";
                break;
            case EDSLIB_BASICTYPE_UNSIGNED_INT:
                Type = "uint";
                break;
            case EDSLIB_BASICTYPE_FLOAT:
                Type = "ieee";
                break;
            case EDSLIB_BASICTYPE_BINARY:
                Type = "binary";
                break;
            default:
                Type = "other";
                break;
        }
        switch (DisplayHint)
        {
            case EDSLIB_DISPLAYHINT_ENUM_SYMTABLE:
                Type = "enum";
                break;
            case EDSLIB_DISPLAYHINT_STRING:
                Type = "string";
                break;
            default:
                break;
        }
        printf("   %10s/%-4u  %s\n", Type, TypeInfo.Size.Bits, ParamDesc->FullName);
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
bool EDS_ProcessDestIntf(CommandData_t *cmd)
{
    char                         *Separator;
    size_t                        FullLen;
    int32_t                       EdsRc;
    EdsLib_Generic_UnsignedInt_t  MidVal;
    bool                          IsNumeric;
    char                         *tail;
    CFE_MissionLib_TopicInfo_t    TopicInfo;
    EdsLib_IntfDB_InterfaceInfo_t IntfInfo;

    MidVal    = strtoul(cmd->DestIntf, &tail, 0);
    IsNumeric = (tail != cmd->DestIntf && *tail == 0);

    if (IsNumeric)
    {
        cmd->PubSub.MsgId.Value = MidVal;
        CFE_MissionLib_UnmapListenerComponent(&cmd->Params, &cmd->PubSub);

        EdsRc = CFE_MissionLib_GetTopicInfo(&CFE_SOFTWAREBUS_INTERFACE, cmd->Params.Telecommand.TopicId, &TopicInfo);
        if (EdsRc != CFE_MISSIONLIB_SUCCESS)
        {
            if (cmd->DestIntf[0] != 0)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "Dest Interface Argument: '%s' rejected. Interface not known.\n",
                         cmd->DestIntf);
            }
            return false;
        }

        cmd->IntfEdsId = TopicInfo.ParentIntfId;
    }
    else
    {
        EdsRc = EdsLib_IntfDB_FindComponentInterfaceByFullName(&EDS_DATABASE, cmd->DestIntf, &cmd->IntfEdsId);
        if (EdsRc != EDSLIB_SUCCESS)
        {
            /*
             * Try adding an implicit "Application" component designator --
             * This is to be more similar to the previous version of this tool which did not
             * have named components at all, and it also reduces the amount that the user has
             * to type since all top-level SB components are called "Application".
             */
            Separator = strrchr(cmd->DestIntf, '/');
            FullLen   = strlen(cmd->DestIntf);
            if (Separator != NULL && (FullLen + sizeof(DEFAULT_COMPONENT)) < sizeof(cmd->DestIntf))
            {
                memmove(Separator + sizeof(DEFAULT_COMPONENT), Separator, 1 + FullLen - (Separator - cmd->DestIntf));
                memcpy(Separator + 1, DEFAULT_COMPONENT, sizeof(DEFAULT_COMPONENT) - 1);
                Separator[0] = '/';
                EdsRc = EdsLib_IntfDB_FindComponentInterfaceByFullName(&EDS_DATABASE, cmd->DestIntf, &cmd->IntfEdsId);
            }
        }
        if (EdsRc == EDSLIB_SUCCESS)
        {
            EdsRc = CFE_MissionLib_FindTopicIdFromIntfId(&CFE_SOFTWAREBUS_INTERFACE,
                                                         cmd->IntfEdsId,
                                                         &cmd->Params.Telecommand.TopicId);
        }

        if (EdsRc != CFE_MISSIONLIB_SUCCESS)
        {
            if (cmd->DestIntf[0] != 0)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "Dest Interface Argument: '%s' rejected. Interface not known.\n",
                         cmd->DestIntf);
            }
            return false;
        }

        EdsRc = EdsLib_IntfDB_GetComponentInterfaceInfo(&EDS_DATABASE, cmd->IntfEdsId, &IntfInfo);
        if (EdsRc != EDSLIB_SUCCESS)
        {
            snprintf(cmd->LastErrorText,
                     sizeof(cmd->LastErrorText),
                     "Cannot lookup interface info for: '%s'\n",
                     cmd->DestIntf);
            return false;
        }

        CFE_MissionLib_MapListenerComponent(&cmd->PubSub, &cmd->Params);
    }

    CFE_MissionLib_Set_PubSub_Parameters(&cmd->Buf.Cmd.BaseObject.Message, &cmd->PubSub);

    EdsRc =
        EdsLib_IntfDB_FindAllArgumentTypes(&EDS_DATABASE, CFE_SB_TELECOMMAND_CMD_ID, cmd->IntfEdsId, &cmd->IntfArg, 1);
    if (EdsRc != EDSLIB_SUCCESS)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "Cannot lookup argument type for: '%s', rc=%d\n",
                 cmd->DestIntf,
                 (int)EdsRc);
        return false;
    }

    CONSOLEUTILS_INFO("Base Indication Argument EdsId=%x / %s\n",
                      (unsigned int)cmd->IntfArg,
                      EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, cmd->IntfArg));

    cmd->ActualArg = cmd->IntfArg;

    return true;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
bool EDS_ProcessCmdCode(CommandData_t *cmd)
{
    int32_t                             EdsRc;
    uint16_t                            Idx;
    EdsLib_Id_t                         PossibleId;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    EDS_ConstraintMatch_t               ConstraintMatch;
    bool                                IsNumeric;
    char                               *tail;

    /*
     * Find the constraint on the "CommandCode" entity
     * This is the only constraint entity used by CFE commands so it is hardcoded to look for this
     */
    EdsRc = EdsLib_DataTypeDB_GetDerivedInfo(&EDS_DATABASE, cmd->IntfArg, &DerivInfo);
    if (EdsRc == EDSLIB_SUCCESS)
    {
        cmd->IsDerived = true;

        ConstraintMatch.IsMatch      = false;
        ConstraintMatch.ValueCompare = strtoul(cmd->CmdName, &tail, 0);
        IsNumeric                    = (tail != cmd->CmdName && *tail == 0);

        Idx = 0;
        while (EdsLib_DataTypeDB_GetDerivedTypeById(&EDS_DATABASE, cmd->IntfArg, Idx, &PossibleId) == EDSLIB_SUCCESS)
        {
            if (IsNumeric)
            {
                EdsLib_DataTypeDB_ConstraintIterator(&EDS_DATABASE,
                                                     cmd->IntfArg,
                                                     PossibleId,
                                                     Match_Constraint_Callback,
                                                     &ConstraintMatch);
            }
            else
            {
                ConstraintMatch.IsMatch =
                    (strcmp(EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, PossibleId), cmd->CmdName) == 0);
            }

            if (ConstraintMatch.IsMatch)
            {
                cmd->ActualArg = PossibleId;
                break;
            }
            ++Idx;
        }

        if (cmd->ActualArg == cmd->IntfArg)
        {
            if (cmd->CmdName[0] == 0)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "Dest Interface requires a derivative specifier / command code: \'%s\'\n",
                         cmd->DestIntf);
            }
            else
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "Command \'%s\' not found within interface \'%s\'\n",
                         cmd->CmdName,
                         cmd->DestIntf);
            }
            return false;
        }
    }
    else if (cmd->CmdName[0] != 0)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "Dest Interface does not have command codes: \'%s\'\n",
                 cmd->DestIntf);
        return false;
    }

    return true;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
bool EDS_LookupPayloadDetail(CommandData_t *cmd)
{
    uint16_t Idx;

    CONSOLEUTILS_INFO("Actual Indication Argument EdsId=%x / %s\n",
                      (unsigned int)cmd->ActualArg,
                      EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, cmd->ActualArg));

    if (EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, cmd->ActualArg, &cmd->EdsTypeInfo) != EDSLIB_SUCCESS)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "Error retrieving info for code %x\n",
                 (unsigned int)cmd->ActualArg);
        return false;
    }

    if (EdsLib_DisplayDB_GetIndexByName(&EDS_DATABASE, cmd->ActualArg, "Payload", &Idx) == EDSLIB_SUCCESS)
    {
        EdsLib_DataTypeDB_GetMemberByIndex(&EDS_DATABASE, cmd->ActualArg, Idx, &cmd->EdsPayloadInfo);
    }

    return true;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
bool EDS_CheckValidIntf(CommandData_t *cmd)
{
    if (cmd->IntfState == EDS_LookupState_UNKNOWN)
    {
        if (!EDS_ProcessDestIntf(cmd))
        {
            cmd->IntfState = EDS_LookupState_FAILED;
        }
        else if (!EDS_ProcessCmdCode(cmd))
        {
            cmd->IntfState = EDS_LookupState_FAILED;
        }
        else
        {
            cmd->IntfState = EDS_LookupState_SUCCESS;
        }
    }

    return (cmd->IntfState == EDS_LookupState_SUCCESS);
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
bool EDS_CheckValidPayload(CommandData_t *cmd)
{
    int32_t EdsRc;

    if (cmd->PayloadState == EDS_LookupState_UNKNOWN)
    {
        if (!EDS_CheckValidIntf(cmd))
        {
            cmd->PayloadState = EDS_LookupState_FAILED;
        }
        else if (!EDS_LookupPayloadDetail(cmd))
        {
            cmd->PayloadState = EDS_LookupState_FAILED;
        }
        else
        {
            EdsRc = EdsLib_DataTypeDB_InitializeNativeObject(&EDS_DATABASE, cmd->ActualArg, &cmd->Buf);
            if (EdsRc != EDSLIB_SUCCESS)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "EdsLib_DataTypeDB_InitializeNativeObject(): %d\n",
                         (int)EdsRc);
                cmd->PayloadState = EDS_LookupState_FAILED;
            }
            else
            {
                cmd->PayloadState = EDS_LookupState_SUCCESS;
            }
        }
    }

    return (cmd->PayloadState == EDS_LookupState_SUCCESS);
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
void EDS_ProcessParameterArgument(CommandData_t *cmd, const char *optarg)
{
    char                     *value;
    int32_t                   Result;
    EdsLib_EntityDescriptor_t Desc;

    value = strchr(optarg, '=');
    if (value == NULL)
    {
        snprintf(
            cmd->LastErrorText,
            sizeof(cmd->LastErrorText),
            "Parameter Argument: '%s' rejected. Must be in the form: 'x=y' where x is the name and y is the value\n",
            optarg);
        cmd->PayloadError = true;
        return;
    }

    *value = 0;
    ++value;

    /* First determine the type of the given argument */
    memset(&Desc, 0, sizeof(Desc));
    Desc.FullName = optarg;
    Result =
        EdsLib_DisplayDB_LocateSubEntity(&EDS_DATABASE, cmd->EdsPayloadInfo.EdsId, Desc.FullName, &Desc.EntityInfo);
    if (Result != EDSLIB_SUCCESS)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "Dest App Argument: '%s' rejected. Parameter not known.\n",
                 optarg);
        cmd->PayloadError = true;
        return;
    }

    CONSOLEUTILS_INFO("Parameter \'%s\' Located at payload offset %d\n", optarg, Desc.EntityInfo.Offset.Bytes);

    Result = EdsLib_Scalar_FromString(&EDS_DATABASE,
                                      Desc.EntityInfo.EdsId,
                                      &cmd->Buf.Byte[cmd->EdsPayloadInfo.Offset.Bytes + Desc.EntityInfo.Offset.Bytes],
                                      value);
    if (Result != 0)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "Parameter Argument: Value '%s' rejected. Unable to parse.\n",
                 value);
        cmd->PayloadError = true;
        return;
    }
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t EDS_ParseDestFullImpl(CommandData_t *cmd, const char *Text, bool AllowInst, bool AllowCmd)
{
    const char *Sep1;
    const char *Sep2;
    size_t      Len;
    char        TempString[OPTARG_SIZE];

    Sep1 = strchr(cmd->DestIntf, ':');
    if (Sep1 != NULL)
    {
        Len = Sep1 - Text;
        if (Len >= sizeof(TempString))
        {
            Len = sizeof(TempString) - 1;
        }
        memcpy(TempString, Text, Len);
        TempString[Len] = 0;

        if (AllowInst)
        {
            cmd->Params.Telecommand.InstanceNumber = 0;
        }
        else
        {
            cmd->Params.Telecommand.InstanceNumber =
                CFE_MissionLib_GetInstanceNumber(&CFE_SOFTWAREBUS_INTERFACE, TempString);
        }
        if (cmd->Params.Telecommand.InstanceNumber == 0)
        {
            CONSOLEUTILS_ERROR("Instance specifier \'%s\' invalid.\n", TempString);
            return CmdSend_OptParse_INVALID;
        }

        ++Sep1;
    }
    else
    {
        Sep1 = Text;
    }

    Sep2 = strchr(Sep1, '.');
    if (Sep2 == NULL)
    {
        Sep2 = Sep1 + strlen(Sep1);
    }

    Len = Sep2 - Sep1;
    if (Len >= sizeof(cmd->DestIntf))
    {
        Len = sizeof(cmd->DestIntf) - 1;
    }
    memcpy(cmd->DestIntf, Sep1, Len);
    cmd->DestIntf[Len] = 0;

    if (*Sep2 == '.')
    {
        ++Sep2;
    }
    Len = strlen(Sep2);
    if (Len > 0)
    {
        if (Len >= sizeof(cmd->CmdName))
        {
            Len = sizeof(cmd->CmdName) - 1;
        }
        if (AllowCmd)
        {
            memcpy(cmd->CmdName, Sep2, Len);
            cmd->CmdName[Len] = 0;
        }
        else
        {
            CONSOLEUTILS_ERROR("Instance specifier \'%s\' invalid.\n", Sep2);
            return CmdSend_OptParse_INVALID;
        }
    }

    return CmdSend_OptParse_ACCEPTED;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t EDS_ParseDestFullCmd(CommandData_t *cmd, const char *Text)
{
    return EDS_ParseDestFullImpl(cmd, Text, true, true);
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t EDS_ParseDestApidOnly(CommandData_t *cmd, const char *Text)
{
    return EDS_ParseDestFullImpl(cmd, Text, true, false);
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t EDS_ParseDestCmdOnly(CommandData_t *cmd, const char *Text)
{
    strncpy(cmd->CmdName, Text, sizeof(cmd->CmdName) - 1);
    return CmdSend_OptParse_ACCEPTED;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
void *EDS_Instantiate(void)
{
    CommandData_t *obj;
    int32_t        EdsRc;

    obj = malloc(sizeof(*obj));
    memset(obj, 0, sizeof(*obj));

    obj->Params.Telecommand.InstanceNumber = 1;

    EdsRc = EdsLib_DataTypeDB_GetTypeInfo(
        &EDS_DATABASE,
        EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), EdsContainer_CFE_HDR_CommandHeader_DATADICTIONARY),
        &obj->EdsHeaderInfo);
    if (EdsRc != EDSLIB_SUCCESS)
    {
        CONSOLEUTILS_ERROR("CCSDS Primary Header lookup failed.\n");
        free(obj);
        obj = NULL;
    }

    obj->Buf.Basic.CommonHdr.SeqFlag = 0x3;
    return obj;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
void EDS_HelpUsage(void *arg)
{
    CommandData_t *cmd = arg;
    uint16_t       Idx;
    EdsLib_Id_t    PossibleId;
    char           ConstraintBuffer[CONSTRAINT_BUFSIZE];

    EDS_CheckValidIntf(cmd);
    EDS_CheckValidPayload(cmd);

    if (!EdsLib_Is_Valid(cmd->IntfEdsId))
    {
        printf("EDS-defined Telecommand Interfaces:\n");
        CFE_MissionLib_EnumerateTopics(&CFE_SOFTWAREBUS_INTERFACE, Enumerate_Topics_Usage_Callback, NULL);
        printf("\n");
    }
    else if (EdsLib_Is_Valid(cmd->EdsPayloadInfo.EdsId))
    {
        printf("\nDefined Payload Fields (sizes in bits):\n");
        EdsLib_DisplayDB_IterateAllEntities(&EDS_DATABASE,
                                            cmd->EdsPayloadInfo.EdsId,
                                            Enumerate_Members_Usage_Callback,
                                            NULL);
    }
    else if (cmd->CmdName[0] == 0)
    {
        printf("\nAvailable Command Codes:\n");
        Idx = 0;
        while (EdsLib_DataTypeDB_GetDerivedTypeById(&EDS_DATABASE, cmd->IntfArg, Idx, &PossibleId) == EDSLIB_SUCCESS)
        {
            strcpy(ConstraintBuffer, "N/A");
            EdsLib_DataTypeDB_ConstraintIterator(&EDS_DATABASE,
                                                 cmd->IntfArg,
                                                 PossibleId,
                                                 Enumerate_Constraint_Callback,
                                                 ConstraintBuffer);

            printf("   %-40s (%s)\n", EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, PossibleId), ConstraintBuffer);
            ++Idx;
        }
    }
    else
    {
        printf("\nSelected command has no payload fields\n");
    }
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t EDS_ParseOption(void *obj, const CmdSend_ArgV_t *ArgV)
{
    CmdSend_OptParse_t retcode;
    int32_t            status;
    EdsLib_Id_t        EdsId;
    CommandData_t     *cmd = obj;

    /* Set defaults */
    retcode = CmdSend_OptParse_UNDEFINED;

    switch (ArgV->OptionId)
    {
        case CmdSend_OptionId_endian:
            if (strcmp(ArgV->Text, "EDS") == 0)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            else
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "endian selection: \'%s\' incompatible for EDS mode\n",
                         ArgV->Text);
                retcode = CmdSend_OptParse_INVALID;
            }
            break;
        case CmdSend_OptionId_protocol:
            cmd->SelectedProto = CmdSend_GetProtocolFromString(ArgV->Text);
            if (cmd->SelectedProto == CmdSend_Protocol_UNDEFINED)
            {
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;
        case CmdSend_OptionId_pktedsver:
        case CmdSend_OptionId_pktid:
            retcode = EDS_ParseDestFullCmd(cmd, ArgV->Text);
            break;
        case CmdSend_OptionId_pktapid:
            retcode = EDS_ParseDestApidOnly(cmd, ArgV->Text);
            break;
        case CmdSend_OptionId_pktpb:
            EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), EdsBoolean_CCSDS_SingleBitFlag_DATADICTIONARY);
            status = EdsLib_Scalar_FromString(&EDS_DATABASE, EdsId, &cmd->Buf.ApidQ.ApidQ.Playback, ArgV->Text);
            if (status == EDSLIB_SUCCESS)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;
        case CmdSend_OptionId_cmdcode:
        case CmdSend_OptionId_pktfc:
            retcode = EDS_ParseDestCmdOnly(cmd, ArgV->Text);
            break;
        case CmdSend_OptionId_pktseqflg:
            EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), EdsEnum_CCSDS_SecHdrFlags_DATADICTIONARY);
            status = EdsLib_Scalar_FromString(&EDS_DATABASE, EdsId, &cmd->Buf.Basic.CommonHdr.SeqFlag, ArgV->Text);
            if (status == EDSLIB_SUCCESS)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;
        case CmdSend_OptionId_pktseqcnt:
            EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), EdsInteger_CCSDS_SeqCount_DATADICTIONARY);
            status = EdsLib_Scalar_FromString(&EDS_DATABASE, EdsId, &cmd->Buf.Basic.CommonHdr.Sequence, ArgV->Text);
            if (status == EDSLIB_SUCCESS)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;
        case CmdSend_OptionId_pktendian:
            EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), EdsBoolean_CCSDS_SingleBitFlag_DATADICTIONARY);
            status = EdsLib_Scalar_FromString(&EDS_DATABASE, EdsId, &cmd->Buf.ApidQ.ApidQ.Endian, ArgV->Text);
            if (status == EDSLIB_SUCCESS)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;
        case CmdSend_OptionId_pktsubsys:
            EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), EdsInteger_CCSDS_SubsystemId_DATADICTIONARY);
            status = EdsLib_Scalar_FromString(&EDS_DATABASE, EdsId, &cmd->Buf.ApidQ.ApidQ.SubsystemId, ArgV->Text);
            if (status == EDSLIB_SUCCESS)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;
        case CmdSend_OptionId_pktver:
            EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), EdsInteger_CCSDS_VersionId_DATADICTIONARY);
            status = EdsLib_Scalar_FromString(&EDS_DATABASE, EdsId, &cmd->Buf.Basic.CommonHdr.VersionId, ArgV->Text);
            if (status == EDSLIB_SUCCESS)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;
        case CmdSend_OptionId_pktsys:
            EdsId  = EDSLIB_MAKE_ID(EDS_INDEX(CCSDS_SPACEPACKET), EdsInteger_CCSDS_SubsystemId_DATADICTIONARY);
            status = EdsLib_Scalar_FromString(&EDS_DATABASE, EdsId, &cmd->Buf.ApidQ.ApidQ.SystemId, ArgV->Text);
            if (status == EDSLIB_SUCCESS)
            {
                retcode = CmdSend_OptParse_ACCEPTED;
            }
            break;

        case CmdSend_OptionId_NONE:
            if (EDS_CheckValidPayload(cmd))
            {
                EDS_ProcessParameterArgument(cmd, ArgV->Text);
            }
            break;

        default:
            break;
    }

    return retcode;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
const char *EDS_GetErrorText(void *obj)
{
    CommandData_t *cmd = obj;

    return cmd->LastErrorText;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
bool EDS_GetPackedObject(void *obj, void *buf, size_t *sz)
{
    CommandData_t    *cmd = obj;
    EdsLib_SizeInfo_t PackedSize;
    int32_t           EdsRc;

    if (!EDS_CheckValidPayload(cmd))
    {
        return false;
    }

    /*
     * Do final encoding of packet
     */
    PackedSize.Bits  = EdsLib_OCTETS_TO_BITS(*sz);
    PackedSize.Bytes = sizeof(cmd->Buf);
    EdsRc = EdsLib_DataTypeDB_PackCompleteObjectVarSize(&EDS_DATABASE, &cmd->ActualArg, buf, &cmd->Buf, &PackedSize);
    if (EdsRc != EDSLIB_SUCCESS)
    {
        CONSOLEUTILS_ERROR("EdsLib_DataTypeDB_PackCompleteObjectVarSize(): %d\n", (int)EdsRc);
        return false;
    }

    *sz = EdsLib_BITS_TO_OCTETS(PackedSize.Bits);
    return true;
}

/*----------------------------------------------------------------
 *
 * API structure
 *
 *-----------------------------------------------------------------*/
/* clang-format off */
const CmdSend_Parser_API_t EDS_API =
{
    .Name         = "EDS",
    .Instantiate  = EDS_Instantiate,
    .ParseOption  = EDS_ParseOption,
    .ShowHelp     = EDS_HelpUsage,
    .GetErrorText = EDS_GetErrorText,
    .GetPackedObject = EDS_GetPackedObject
};
