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
 * \file     testctrl.c
 * \ingroup  testexecutive
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of test cpu controller
 */

#include <stdlib.h>
#include <string.h> /* memset() */
#include <ctype.h>
#include <getopt.h>

#include "osapi.h"

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_displaydb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_lua_softwarebus.h"
#include "ccsds_spacepacket_eds_typedefs.h"


/*
**  volume table.
*/
OS_VolumeInfo_t OS_VolumeTable [OS_MAX_FILE_SYSTEMS] =
{
        /*
         ** The following entry is a "pre-mounted" path to a non-volatile device
         ** This is intended to contain the functional test scripts
         */
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  }
};

uint32_t TargetArgCount;
char **TargetArgs;

#ifdef JPHFIX_SERVER
void TestCtrl_ConnHandler(void)
{
    uint32 SocketID;
    int32 Status;
    OS_SockAddr_t SocketAddress;
    char DataBuf[128];

    Status = OS_SocketOpen(&SocketID, OS_SocketDomain_INET, OS_SocketType_STREAM);
    if (Status != OS_SUCCESS)
    {
        fprintf(stderr,"OS_SocketOpen() failed: %d\n", (int)Status);
        return;
    }

   OS_SocketAddrInit(&SocketAddress, OS_SocketDomain_INET);
   OS_SocketAddrSetPort(&SocketAddress, 2345);


   uint32 ConnID;
   OS_SockAddr_t ConnAddress;

   Status = OS_SocketBind(SocketID, &SocketAddress);
   if (Status != OS_SUCCESS)
   {
       fprintf(stderr,"OS_SocketBind() failed: %d\n", (int)Status);
       return;
   }

   while(1)
   {
       Status = OS_SocketAccept(SocketID, &ConnID, &ConnAddress, OS_PEND);
       if (Status != OS_SUCCESS)
       {
           fprintf(stderr,"OS_SocketAccept() failed: %d\n", (int)Status);
           break;
       }

       while (1)
       {
           Status = OS_StreamRead(ConnID, DataBuf, sizeof(DataBuf)-1, OS_PEND);
           if (Status < OS_SUCCESS)
           {
               fprintf(stderr,"OS_StreamRead() failed: %d\n", (int)Status);
               break;
           }

           if (Status == 0)
           {
               printf("OS_StreamRead(): disconnect\n");
               break;
           }

           DataBuf[Status] = 0;
           printf("OS_StreamRead(): %s\n", DataBuf);
       }

       OS_SocketClose(ConnID);
   }

   OS_SocketClose(SocketID);

}
#endif

#define COMMAND_MAX_ARGS        8
#define NETBUFFER_MAX_SIZE      (64 + ((sizeof(CCSDS_SpacePacket_Buffer_t) * 4) / 3))
#define OBJBUFFER_MAX_SIZE      (64 + sizeof(CCSDS_SpacePacket_Buffer_t))

struct
{
    char *ArgPtr[COMMAND_MAX_ARGS];
    char NetBuffer[NETBUFFER_MAX_SIZE];
    uint8_t ObjBuffer[OBJBUFFER_MAX_SIZE];
    uint32 ArgCount;
    uint32 NetDataLen;
    uint32 ObjBitSize;
} TestCtrl;

void TestCtrl_TokenizeCommand(char *CommandData)
{
    char *Ptr = CommandData;

    TestCtrl.ArgCount = 0;

    /* skip flag char and all leading whitespace */
    do
    {
        ++Ptr;
    }
    while(isspace((int)*Ptr));

    while(TestCtrl.ArgCount < COMMAND_MAX_ARGS && *Ptr != 0)
    {
        TestCtrl.ArgPtr[TestCtrl.ArgCount] = Ptr;
        ++TestCtrl.ArgCount;

        while(*Ptr != 0 && !isspace((int)*Ptr))
        {
            ++Ptr;
        }

        if (*Ptr == 0)
        {
            break;
        }

        *Ptr = 0;
        do
        {
            ++Ptr;
        }
        while(isspace((int)*Ptr));
    }
}

void TestCtrl_ProcessDataBlock(void)
{
    char *EndPtr;
    uint32_t BitSize;

    if (TestCtrl.NetDataLen < sizeof(TestCtrl.NetBuffer))
    {
        TestCtrl.NetBuffer[TestCtrl.NetDataLen] = 0;
    }
    else
    {
        TestCtrl.NetBuffer[sizeof(TestCtrl.NetBuffer) - 1] = 0;
    }

    if (TestCtrl.ArgCount > 1)
    {
        BitSize = strtoul(TestCtrl.ArgPtr[1], &EndPtr, 0);
        if (*EndPtr != 0)
        {
            BitSize = 0;
        }
    }
    else
    {
        BitSize = 0;
    }

    if (BitSize > (8 * sizeof(TestCtrl.ObjBuffer)))
    {
        BitSize = (8 * sizeof(TestCtrl.ObjBuffer));
    }

    EdsLib_DisplayDB_Base64Decode(TestCtrl.ObjBuffer, BitSize, TestCtrl.NetBuffer);
    TestCtrl.ObjBitSize = BitSize;

    EdsLib_Generate_Hexdump(stderr, TestCtrl.ObjBuffer, 0, (BitSize + 7) / 8);
}

void TestCtrl_RunCommandLoop(void)
{
    char NetworkBuffer[256];
    uint32 ContentLen;

    while (fgets(NetworkBuffer, sizeof(NetworkBuffer), stdin) != NULL)
    {
        /*
         * All directives should begin with a consistent character for easy identification.
         * This character should _NOT_ be part of the standard base64 charset, since base64
         * may be used to transfer binary objects across the pipe.  Using a non-base64 char
         * as a flag ensures that it is not possible to confuse directives with data.
         *
         * (This means A-Z, a-z, 0-9 and symbols '+', '/', and '=' should be avoided.  Other
         * chars are fair game)
         */
        if (NetworkBuffer[0] == '@')
        {
            /* it is a directive */
            TestCtrl_TokenizeCommand(NetworkBuffer);

            if (TestCtrl.ArgCount > 0)
            {
                if (strcmp(TestCtrl.ArgPtr[0], "BLOCK") == 0)
                {
                    TestCtrl_ProcessDataBlock();
                    TestCtrl.NetDataLen = 0;
                }
                else
                {
                    /* Call user command handler */
                }
            }
        }
        else if (TestCtrl.NetDataLen < sizeof(TestCtrl.NetBuffer))
        {
            ContentLen = strlen(NetworkBuffer);
            if ((TestCtrl.NetDataLen + ContentLen) > sizeof(TestCtrl.NetBuffer))
            {
                ContentLen = sizeof(TestCtrl.NetBuffer) - TestCtrl.NetDataLen;
            }
            memcpy(&TestCtrl.NetBuffer[TestCtrl.NetDataLen], NetworkBuffer, ContentLen);
            TestCtrl.NetDataLen += ContentLen;
        }
    }
}

int main(int argc, char *argv[])
{
    int opt;

    /* Initialize the OSAL */
    if (OS_API_Init() != OS_SUCCESS)
    {
        fprintf(stderr,"OS_API_Init() failed");
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "c:i:")) >= 0)
    {
        switch (opt)
        {
        case 'c':
        {
            CFE_MissionLib_GetInstanceNumber(&CFE_SOFTWAREBUS_INTERFACE, optarg);
            break;
        }
        case 'i':
        {
            break;
        }
        case 'l':
        {
            break;
        }
        default: /* '?' */
        {
            fprintf(stderr, "Usage: %s [-c cpu_name]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        }
    }

    if (optind < argc)
    {
        TargetArgCount = argc - optind;
        TargetArgs = &argv[optind];
    }

    TestCtrl_RunCommandLoop();

    return EXIT_SUCCESS;
}


