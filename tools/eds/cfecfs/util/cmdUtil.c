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

#ifdef WIN32
#pragma warning (disable:4786)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef int socklen_t;
#else
#include "getopt.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#define SOCKET int
#define closesocket(fd) close(fd)
#endif

#include <cfe_mission_cfg.h>

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_displaydb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_hdr_eds_typedefs.h"

const char DEFAULT_COMPONENT[] = "Application";

/*
** SendUdp
*/
int SendUdp(char *hostname, unsigned short port, unsigned char *packetData, int packetSize) {
    SOCKET              sd;
    int                 rc;
    unsigned int        i;
    struct sockaddr_in  cliAddr;
    struct sockaddr_in  remoteServAddr;
    struct hostent     *hostID;

    #ifdef WIN32
        WSADATA  wsaData;
        WSAStartup(WINSOCK_VERSION, &wsaData);
    #endif

    if (hostname == NULL) {
        return -1;
    }

    /*
    ** get server IP address (no check if input is IP address or DNS name
    */
    hostID = gethostbyname(hostname);
    if (hostID == NULL) {
        return -2;
    }

    printf("sending data to '%s' (IP : %s); port %d\n", hostID->h_name,
        inet_ntoa(*(struct in_addr *)hostID->h_addr_list[0]), port);

    /*
    ** Setup socket structures
    */
    remoteServAddr.sin_family = hostID->h_addrtype;
    memcpy((char *) &remoteServAddr.sin_addr.s_addr,
        hostID->h_addr_list[0], hostID->h_length);
    remoteServAddr.sin_port = htons(port);

    /*
    ** Create Socket
    */
    sd = socket(AF_INET,SOCK_DGRAM,0);
    if (sd < 0) {
        return -4;
    }

    /*
    ** bind any port
    */
    cliAddr.sin_family = AF_INET;
    cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliAddr.sin_port = htons(0);

    rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
    if (rc < 0) {
        printf("%u: cannot bind port\n", port);
        return -5;
    }

    printf("Data to send:\n");
    i = 0;
    while (i < packetSize) {
        printf("0x%02X ", packetData[i] & 0xFF);
        if (++i % 8 == 0) {
            puts("");
        }
    }
    puts("");

    /*
    ** send the event
    */
    rc = sendto(sd, (char*)packetData, packetSize, 0,
        (struct sockaddr*)&remoteServAddr,
        sizeof(remoteServAddr));

    if (rc < 0) {
        closesocket(sd);
        return -6;
    }

    closesocket(sd);
    return 0;
}

/*
** Defines
*/
#define DEFAULT_HOSTNAME "127.0.0.1"
#define OPTARG_SIZE       128

#define DEFAULT_PORTNUM   1234
#define CONSTRAINT_BUFSIZE 20

/*
** Parameter datatype structure
*/
typedef struct {
    char HostName[OPTARG_SIZE]; /* Hostname like "localhost" or "192.168.0.121" */
    char DestIntf[OPTARG_SIZE];
    char CmdName[OPTARG_SIZE];
    uint16_t PortNum;           /* Portnum: Default "1234" */

    int  Verbose;               /* Verbose option set? */
    int  GotDestInfo;
    int  GotUsageReq;

    EdsLib_Id_t IntfArg;
    EdsLib_Id_t ActualArg;
    CFE_SB_Listener_Component_t Params;
    CFE_MissionLib_InterfaceInfo_t IntfInfo;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSub;
    uint16_t CommandCodeConstrIdx;
    EdsLib_DataTypeDB_EntityInfo_t CommandCodeInfo;
    EdsLib_DataTypeDB_TypeInfo_t EdsHeaderInfo;
    EdsLib_DataTypeDB_TypeInfo_t EdsTypeInfo;
    EdsLib_DataTypeDB_EntityInfo_t EdsPayloadInfo;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
} CommandData_t;


CFE_HDR_CommandHeader_Buffer_t CommandBuffer;
static CFE_HDR_CommandHeader_PackedBuffer_t PackedCommand;

/*
** Declare the global command data
*/
CommandData_t CommandData;

/*
** getopts parameter passing options string
*/
static const char *optString = "H:P:D:v?";

/*
** getopts_long long form argument table
*/
static struct option longOpts[] = {
    { "host",      required_argument, NULL, 'H' },
    { "port",      required_argument, NULL, 'P' },
    { "dest",      required_argument, NULL, 'D' },
    { "help",      no_argument,       NULL, '?' },
    { "verbose",   no_argument,       NULL, 'v' },
    { NULL,        no_argument,       NULL, 0   }
};

/*
** Display program usage, and exit.
*/
void DisplayUsage(char *Name, CommandData_t *CommandData )
{
    printf("%s -- A CCSDS Command Client.\n",Name);
    printf("      The parameters are:\n");
    printf("      --host / -H : The hostname or IP address to send the command to ( default = localhost )\n");
    printf("      --port / -P : The UDP port to send the command to ( default = 1234 )\n");
    printf("      --dest / -D : The EDS interface name to send the command to\n");
    printf("      parameter=value : Set the interface parameter name to value (repeat as necessary)\n");

    printf(" \n");
    printf("       An example of using this is:\n");
    printf(" \n");
    printf("  %s --host=localhost --port=1234 -D 1:CFE_ES/Application/CMD.QueryAllCmd Filename=MyFile.txt\n",Name);
    printf(" \n");
}

void ProcessParameterArgument(char *optarg, CommandData_t *CommandData)
{
   char *value;
   int32_t Result;
   EdsLib_EntityDescriptor_t Desc;

   value = strchr(optarg, '=');
   if (value == NULL)
   {
      fprintf(stderr,"Parameter Argument: '%s' rejected. Must be in the form: 'x=y' where x is the name and y is the value\n",optarg);
      return;
   }

   *value = 0;
   ++value;


   /* First determine the type of the given argument */
   memset(&Desc,0,sizeof(Desc));
   Desc.FullName = optarg;
   Result = EdsLib_DisplayDB_LocateSubEntity(&EDS_DATABASE, CommandData->EdsPayloadInfo.EdsId, Desc.FullName, &Desc.EntityInfo);
   if (Result != EDSLIB_SUCCESS)
   {
      fprintf(stderr,"Dest App Argument: '%s' rejected. Parameter not known.\n",optarg);
      return;
   }

   if (CommandData->Verbose) {
       printf("Parameter \'%s\' Located at payload offset %d\n",optarg,
               Desc.EntityInfo.Offset.Bytes);
   }

   Result = EdsLib_Scalar_FromString(&EDS_DATABASE, Desc.EntityInfo.EdsId,
           &CommandBuffer.Byte[CommandData->EdsPayloadInfo.Offset.Bytes + Desc.EntityInfo.Offset.Bytes],
           value);
   if (Result != 0)
   {
      fprintf(stderr,"Parameter Argument: Value '%s' rejected. Unable to parse.\n",value);
      return;
   }
}

static void Enumerate_Topics_Usage_Callback(void *Arg, uint16_t TopicId, const char *TopicName)
{
    if (TopicName != NULL)
    {
        printf("   %s\n", TopicName);
    }
}

static void Enumerate_Constraint_Callback(const EdsLib_DatabaseObject_t *GD, const EdsLib_DataTypeDB_EntityInfo_t *MemberInfo, EdsLib_GenericValueBuffer_t *ConstraintValue, void *Arg)
{
    char *Buffer = Arg;

    switch(ConstraintValue->ValueType)
    {
    case EDSLIB_BASICTYPE_SIGNED_INT:
        snprintf(Buffer, CONSTRAINT_BUFSIZE, "%lld",
                (long long)ConstraintValue->Value.SignedInteger);
        break;
    case EDSLIB_BASICTYPE_UNSIGNED_INT:
        snprintf(Buffer, CONSTRAINT_BUFSIZE, "%llu",
                (unsigned long long)ConstraintValue->Value.SignedInteger);
        break;
    case EDSLIB_BASICTYPE_BINARY:
        strncpy(Buffer, ConstraintValue->Value.StringData, CONSTRAINT_BUFSIZE-1);
        Buffer[CONSTRAINT_BUFSIZE-1] = 0;
        break;
    default:
        break;
    }
}

static void Enumerate_Members_Usage_Callback(void *Arg, const EdsLib_EntityDescriptor_t *ParamDesc)
{
    const char *Type;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    EdsLib_DisplayHint_t DisplayHint;
    if (ParamDesc->FullName != NULL)
    {
        EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, ParamDesc->EntityInfo.EdsId, &TypeInfo);
        DisplayHint = EdsLib_DisplayDB_GetDisplayHint(&EDS_DATABASE, ParamDesc->EntityInfo.EdsId);

        switch(TypeInfo.ElemType)
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
        switch(DisplayHint)
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

/*
** main function
*/
int main(int argc, char *argv[]) {
    int   opt = 0;
    int   longIndex = 0;
    int   retStat;
    uint16_t Idx;
    int32_t EdsRc;
    char *Separator;
    char ConstraintBuffer[CONSTRAINT_BUFSIZE];

    /*
    ** Initialize the CommandData struct
    */
    memset(&(CommandData), 0, sizeof(CommandData_t));

    strncpy(CommandData.HostName, DEFAULT_HOSTNAME, OPTARG_SIZE-1);
    CommandData.PortNum = 0;
    CommandData.Params.Telecommand.InstanceNumber = 1;

    /*
    ** Process the arguments with getopt_long(), then
    ** Build the packet.
    */
    opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
    while( opt != -1 )
    {
        switch( opt )
        {
        case 'H':
            printf("Host: %s\n",(char *)optarg);
            strncpy(CommandData.HostName, optarg, OPTARG_SIZE-1);
            break;

        case 'P':
            CommandData.PortNum = atoi(optarg);
            break;

        case 'v':
            printf("Verbose messages on.\n");
            CommandData.Verbose = 1;
            break;

        case 'D':
            strncpy(CommandData.DestIntf, optarg, OPTARG_SIZE-1);
            break;

        case '?':
            CommandData.GotUsageReq = 1;
            break;

        default:
            break;
        }

        opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
    }

    if (CommandData.GotUsageReq)
    {
        DisplayUsage(argv[0], &CommandData);
    }

    EdsRc = EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE,
            EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), CFE_HDR_CommandHeader_DATADICTIONARY),
            &CommandData.EdsHeaderInfo);
    if (EdsRc != EDSLIB_SUCCESS)
    {
        fprintf(stderr,"CCSDS Primary Header lookup failed.\n");
        return EXIT_FAILURE;
    }

    Separator = strchr(CommandData.DestIntf, ':');
    if (Separator != NULL)
    {
        *Separator = 0;
        CommandData.Params.Telecommand.InstanceNumber =
                CFE_MissionLib_GetInstanceNumber(&CFE_SOFTWAREBUS_INTERFACE, CommandData.DestIntf);
        if (CommandData.Params.Telecommand.InstanceNumber == 0)
        {
            fprintf(stderr,"Instance specifier \'%s\' invalid.\n", CommandData.DestIntf);
            return EXIT_FAILURE;
        }
        ++Separator;
        memmove(CommandData.DestIntf, Separator, 1 + strlen(Separator));
    }

    if (CommandData.PortNum == 0)
    {
        CommandData.PortNum = DEFAULT_PORTNUM;
        if (CommandData.Params.Telecommand.InstanceNumber > 0)
        {
            CommandData.PortNum += CommandData.Params.Telecommand.InstanceNumber - 1;
        }
    }

    Separator = strchr(CommandData.DestIntf, '.');
    if (Separator != NULL)
    {
        *Separator = 0;
        strcpy(CommandData.CmdName, Separator + 1);
    }

    EdsRc = CFE_MissionLib_FindTopicByName(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telecommand_Interface_ID,
            CommandData.DestIntf, &CommandData.Params.Telecommand.TopicId);
    if (EdsRc != CFE_MISSIONLIB_SUCCESS)
    {
        /*
         * Try adding an implicit "Application" component designator --
         * This is to be more similar to the previous version of this tool which did not
         * have named components at all, and it also reduces the amount that the user has
         * to type since all top-level SB components are called "Application".
         */
        Separator = strrchr(CommandData.DestIntf, '/');
        size_t FullLen = strlen(CommandData.DestIntf);
        if (Separator != NULL && (FullLen + sizeof(DEFAULT_COMPONENT)) < sizeof(CommandData.DestIntf))
        {
            memmove(Separator + sizeof(DEFAULT_COMPONENT), Separator, 1 + FullLen - (Separator - CommandData.DestIntf));
            memcpy(Separator+1, DEFAULT_COMPONENT, sizeof(DEFAULT_COMPONENT) - 1);
            Separator[0] = '/';
            EdsRc = CFE_MissionLib_FindTopicByName(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telecommand_Interface_ID,
                    CommandData.DestIntf, &CommandData.Params.Telecommand.TopicId);
        }
    }
    if (EdsRc != CFE_MISSIONLIB_SUCCESS)
    {
        if (CommandData.DestIntf[0] != 0)
        {
            fprintf(stderr,"Dest Interface Argument: '%s' rejected. Interface not known.\n", CommandData.DestIntf);
        }
        if (CommandData.GotUsageReq)
        {
            printf("EDS-defined Telecommand Interfaces:\n");
            CFE_MissionLib_EnumerateTopics(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telecommand_Interface_ID, Enumerate_Topics_Usage_Callback, NULL);
            printf("\n");
        }
        return EXIT_FAILURE;
    }

    EdsRc = CFE_MissionLib_GetInterfaceInfo(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telecommand_Interface_ID, &CommandData.IntfInfo);
    if (EdsRc != CFE_MISSIONLIB_SUCCESS)
    {
        fprintf(stderr,"Cannot lookup interface info for: '%s'\n", CommandData.DestIntf);
        return EXIT_FAILURE;
    }

    if (CommandData.IntfInfo.NumCommands != 1)
    {
        fprintf(stderr,"Interface \'%s\' has multiple commands: %u\n", CommandData.DestIntf, (unsigned int)CommandData.IntfInfo.NumCommands);
        return EXIT_FAILURE;
    }

    CFE_MissionLib_MapListenerComponent(&CommandData.PubSub, &CommandData.Params);

    CFE_MissionLib_Set_PubSub_Parameters(&CommandBuffer.BaseObject.Message, &CommandData.PubSub);

    EdsRc = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telecommand_Interface_ID,
            CommandData.Params.Telecommand.TopicId, 1, 1, &CommandData.IntfArg);
    if (EdsRc != CFE_MISSIONLIB_SUCCESS)
    {
        fprintf(stderr,"Cannot lookup argument type for: '%s'\n", CommandData.DestIntf);
        return EXIT_FAILURE;
    }

    if (CommandData.Verbose)
    {
        printf("Base Indication Argument EdsId=%x / %s\n", (unsigned int)CommandData.IntfArg,
                EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, CommandData.IntfArg));
    }

    CommandData.ActualArg = CommandData.IntfArg;

    /*
     * Find the constraint on the "CommandCode" entity
     * This is the only constraint entity used by CFE commands so it is hardcoded to look for this
     */
    EdsRc = EdsLib_DataTypeDB_GetDerivedInfo(&EDS_DATABASE, CommandData.IntfArg, &CommandData.DerivInfo);
    if (EdsRc == EDSLIB_SUCCESS)
    {
        EdsLib_Id_t PossibleId;

        Idx = 0;
        while (EdsLib_DataTypeDB_GetDerivedTypeById(&EDS_DATABASE, CommandData.IntfArg, Idx, &PossibleId) == EDSLIB_SUCCESS)
        {
            if (strcmp(EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, PossibleId), CommandData.CmdName) == 0)
            {
                CommandData.ActualArg = PossibleId;
                break;
            }
            ++Idx;
        }

        if (CommandData.ActualArg == CommandData.IntfArg)
        {
            if (CommandData.CmdName[0] == 0)
            {
                fprintf(stderr,"Dest Interface requires a derivative specifier / command code: \'%s\'\n", CommandData.DestIntf);
            }
            else
            {
                fprintf(stderr,"Command \'%s\' not found within interface \'%s\'\n", CommandData.CmdName, CommandData.DestIntf);
            }
            if (CommandData.GotUsageReq)
            {
                printf("\nAvailable Command Codes:\n");
                Idx = 0;
                while (EdsLib_DataTypeDB_GetDerivedTypeById(&EDS_DATABASE, CommandData.IntfArg, Idx, &PossibleId) == EDSLIB_SUCCESS)
                {
                    strcpy(ConstraintBuffer, "N/A");
                    EdsLib_DataTypeDB_ConstraintIterator(&EDS_DATABASE, CommandData.IntfArg, PossibleId, Enumerate_Constraint_Callback, ConstraintBuffer);

                    printf("   %-40s (%s)\n", EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, PossibleId), ConstraintBuffer);
                    ++Idx;
                }
            }
            return EXIT_FAILURE;
        }
    }
    else if (CommandData.CmdName[0] != 0)
    {
        fprintf(stderr,"Dest Interface does not have command codes: \'%s\'\n", CommandData.DestIntf);
        return EXIT_FAILURE;
    }

    if (CommandData.Verbose)
    {
        printf("Actual Indication Argument EdsId=%x / %s\n", (unsigned int)CommandData.ActualArg,
                EdsLib_DisplayDB_GetBaseName(&EDS_DATABASE, CommandData.ActualArg));
    }

    if (EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, CommandData.ActualArg, &CommandData.EdsTypeInfo) != EDSLIB_SUCCESS)
    {
        fprintf(stderr,"Error retrieving info for code %x\n", (unsigned int)CommandData.ActualArg);
        return EXIT_FAILURE;
    }

    if (EdsLib_DisplayDB_GetIndexByName(&EDS_DATABASE, CommandData.ActualArg, "Payload", &Idx) == EDSLIB_SUCCESS)
    {
        EdsLib_DataTypeDB_GetMemberByIndex(&EDS_DATABASE, CommandData.ActualArg, Idx, &CommandData.EdsPayloadInfo);
    }

    if (CommandData.GotUsageReq)
    {
        if (EdsLib_Is_Valid(CommandData.EdsPayloadInfo.EdsId))
        {
            printf("\nDefined Payload Fields (sizes in bits):\n");
            EdsLib_DisplayDB_IterateAllEntities(&EDS_DATABASE, CommandData.EdsPayloadInfo.EdsId, Enumerate_Members_Usage_Callback, NULL);
        }
        else
        {
            printf("\nSelected command has no payload fields\n");
        }
        return EXIT_FAILURE;
    }

    EdsRc = EdsLib_DataTypeDB_InitializeNativeObject(&EDS_DATABASE, CommandData.ActualArg, CommandBuffer.Byte);
    if (EdsRc != EDSLIB_SUCCESS)
    {
        printf("Error in EdsLib_DataTypeDB_InitializeNativeObject(): %d\n", (int)EdsRc);
        return EXIT_FAILURE;
    }

    /*
     * Process parameter values on the command line
     */
    while (optind < argc)
    {
       ProcessParameterArgument(argv[optind], &CommandData);
       ++optind;
    }

    /*
     * Do final encoding of packet
     * Note for the length field, the size of the primary header must be subtracted.
     * Hardcoding the sequence number / flags field for now.
     */
    CommandBuffer.BaseObject.Message.CCSDS.CommonHdr.SeqFlag = 0x3;
    EdsLib_DataTypeDB_PackCompleteObject(&EDS_DATABASE, &CommandData.ActualArg,
            PackedCommand, CommandBuffer.Byte,
            sizeof(PackedCommand) * 8, CommandData.EdsTypeInfo.Size.Bytes);

    /*
    ** Send the packet
    */
    retStat = SendUdp(CommandData.HostName, CommandData.PortNum, PackedCommand, (CommandData.EdsTypeInfo.Size.Bits + 7) / 8);

    if (retStat < 0) {
        fprintf(stderr,"Problem sending UDP packet: %d\n", retStat);
        return ( retStat );
    } else if (CommandData.Verbose) {
        printf("Command packet sent OK.\n");
    }

    return EXIT_SUCCESS;
}
