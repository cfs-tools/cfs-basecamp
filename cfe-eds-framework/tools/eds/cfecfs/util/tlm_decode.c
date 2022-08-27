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
#include "cfe_sb_eds_typedefs.h"
#include "cfe_hdr_eds_typedefs.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_displaydb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"


#define BASE_SERVER_PORT 1235

CFE_HDR_TelemetryHeader_Buffer_t       LocalBuffer;
CFE_HDR_TelemetryHeader_PackedBuffer_t NetworkBuffer;

static const char *optString = "c:?";

/*
** getopts_long long form argument table
*/
static struct option longOpts[] = {
    { "cpu",       required_argument, NULL, 'c' },
    { "help",      no_argument,       NULL, '?' },
    { NULL,        no_argument,       NULL, 0   }
};



void TlmUtilDisplay(void *Arg, const EdsLib_EntityDescriptor_t *Param)
{
   uint8_t *BasePtr;
   char OutputBuffer[256];

   BasePtr = (uint8_t *)Arg;
   BasePtr += Param->EntityInfo.Offset.Bytes;
   EdsLib_Scalar_ToString(&EDS_DATABASE, Param->EntityInfo.EdsId, OutputBuffer, sizeof(OutputBuffer), BasePtr);
   printf("%s(): Bit=%-4d %35s = %s\n",__func__,
           Param->EntityInfo.Offset.Bits, Param->FullName, OutputBuffer);
}

int main(int argc, char *argv[])
{
  int   opt = 0;
  int   longIndex = 0;
  int                 sd, rc, n, cliLen;
  struct sockaddr_in  cliAddr, servAddr;
  unsigned short      Port;
  EdsLib_Id_t      EdsId;
  EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
  CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
  CFE_SB_Publisher_Component_t PublisherParams;
  char TempBuffer[64];
  int32_t Status;

  Port = BASE_SERVER_PORT;
  opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
  while( opt != -1 )
  {
      switch( opt )
      {
      case 'c':
          Port += atoi(optarg) - 1;
          break;

      case '?':
          break;

      default:
          break;
      }

      opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
  }


  /*
  ** socket creation
  */
  sd=socket(AF_INET, SOCK_DGRAM, 0);
  if(sd < 0)
  {
    printf("%s: cannot open socket \n",argv[0]);
    exit(1);
  }

  /*
  ** bind local server port
  */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(Port);
  rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));
  if(rc<0)
  {
    printf("%s: cannot bind port number %d \n",
	   argv[0], Port);
    exit(1);
  }

  printf("%s: waiting for data on port UDP %u\n",
	   argv[0],Port);

  /* server infinite loop */
  while(1)
  {

    /*
    ** receive message
    */
    cliLen = sizeof(cliAddr);
    n = recvfrom(sd, NetworkBuffer, sizeof(NetworkBuffer), 0,
		 (struct sockaddr *) &cliAddr, (socklen_t *) &cliLen);

    if(n<0)
    {
      printf("%s: cannot receive data \n",argv[0]);
      continue;
    }

    /*
    ** print received message
    */

    printf("Telemetry Packet From: %s:UDP%u, %u bits : \n",
      inet_ntoa(cliAddr.sin_addr),
      ntohs(cliAddr.sin_port),
      8 * n);

    if (n > 0)
    {
        EdsLib_Generate_Hexdump(stdout, NetworkBuffer, 0, n);
    }

    EdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), CFE_HDR_TelemetryHeader_DATADICTIONARY);
    Status = EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, EdsId, &TypeInfo);
    if (Status != EDSLIB_SUCCESS)
    {
        return Status;
    }

    Status = EdsLib_DataTypeDB_UnpackPartialObject(&EDS_DATABASE, &EdsId,
            LocalBuffer.Byte, NetworkBuffer, sizeof(LocalBuffer), 8 * n, 0);
    if (Status != EDSLIB_SUCCESS)
    {
        return Status;
    }

    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &LocalBuffer.BaseObject.Message);
    CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);

    Status = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, CFE_SB_Telemetry_Interface_ID,
            PublisherParams.Telemetry.TopicId, 1, 1, &EdsId);
    if (Status != CFE_MISSIONLIB_SUCCESS)
    {
        return Status;
    }

    Status = EdsLib_DataTypeDB_UnpackPartialObject(&EDS_DATABASE, &EdsId, LocalBuffer.Byte, NetworkBuffer,
            sizeof(LocalBuffer), 8 * n, TypeInfo.Size.Bytes);
    if (Status != EDSLIB_SUCCESS)
    {
        return Status;
    }

    printf("Formatcode=%08lx / %s\n",(unsigned long)EdsId,
            EdsLib_DisplayDB_GetTypeName(&EDS_DATABASE, EdsId, TempBuffer, sizeof(TempBuffer)));

    Status = EdsLib_DataTypeDB_VerifyUnpackedObject(&EDS_DATABASE, EdsId, LocalBuffer.Byte,
            NetworkBuffer, EDSLIB_DATATYPEDB_RECOMPUTE_NONE);
    if (Status != EDSLIB_SUCCESS)
    {
        printf("NOTE - EDS VERIFICATION FAILED: code=%d\n", (int)Status);
    }

    EdsLib_DisplayDB_IterateAllEntities(&EDS_DATABASE, EdsId, TlmUtilDisplay, LocalBuffer.Byte);
    printf("\n");

  }/* end of server infinite loop */

  return 0;

}
