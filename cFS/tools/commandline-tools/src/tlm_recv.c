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

#include "tlm_recv.h"

#define BASE_SERVER_PORT 2234

static const TlmRecv_Display_API_t *TLMRECV_DISPLAY_API[] = {
#ifdef CFE_EDS_ENABLED
    &EDS_API,
#endif
    &PassThru_API
};

#define TLMRECV_MAX_NUM_DISPLAYS (sizeof(TLMRECV_DISPLAY_API) / sizeof(TLMRECV_DISPLAY_API[0]))

static const char *optString = "c:p:v?";

/*
** getopts_long long form argument table
*/
static struct option longOpts[] = {
    { "cpu",     required_argument, NULL, 'c' },
    { "port",    required_argument, NULL, 'p' },
    { "verbose", no_argument,       NULL, 'v' },
    { "help",    no_argument,       NULL, '?' },
    { NULL,      no_argument,       NULL, 0   }
};

typedef struct TlmRecv_Display
{
    void *Obj;
} TlmRecv_Display_t;

/*
 * Parameter datatype structure
 */
typedef struct
{
    bool ShowHelp;

    int               NumDisplays;
    TlmRecv_Display_t Display[TLMRECV_MAX_NUM_DISPLAYS];

    uint8_t NetBuf[TLMRECV_MAX_PACKET_SIZE];

} TlmData_t;

TlmData_t TlmData;

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Instantiates the display/decoder objects
 *
 *-----------------------------------------------------------------*/
void InstantiateDisplays(void)
{
    TlmRecv_Display_t           *disp;
    const TlmRecv_Display_API_t *API;
    int                          j;

    for (j = 0; j < TLMRECV_MAX_NUM_DISPLAYS; ++j)
    {
        API = TLMRECV_DISPLAY_API[j];
        if (API == NULL)
        {
            break;
        }

        disp      = &TlmData.Display[j];
        disp->Obj = API->Instantiate();
    }
    TlmData.NumDisplays = j;
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Destroys the display/decoder objects
 *
 *-----------------------------------------------------------------*/
void DestroyDisplays(void)
{
    TlmRecv_Display_t *disp;
    int                j;

    for (j = 0; j < TlmData.NumDisplays; ++j)
    {
        disp = &TlmData.Display[j];
        if (disp->Obj != NULL)
        {
            free(disp->Obj);
            disp->Obj = NULL;
        }
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Destroys the display/decoder objects
 *
 *-----------------------------------------------------------------*/
void DisplayObject(const void *Data, size_t Size)
{
    TlmRecv_Display_t           *disp;
    const TlmRecv_Display_API_t *API;
    int                          j;

    for (j = 0; j < TlmData.NumDisplays; ++j)
    {
        disp = &TlmData.Display[j];
        API  = TLMRECV_DISPLAY_API[j];

        if (disp->Obj != NULL && API != NULL)
        {
            CONSOLEUTILS_INFO("Invoking %s decoder\n", API->Name);
            if (API->DisplayObject(disp->Obj, Data, Size))
            {
                break;
            }
        }
    }
}

/*----------------------------------------------------------------
 *
 * MAIN ROUTINE
 *
 *-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int                opt       = 0;
    int                longIndex = 0;
    int                sd, rc, n, cliLen;
    struct sockaddr_in cliAddr, servAddr;
    unsigned short     Port;

    memset(&TlmData, 0, sizeof(TlmData));

    Port = BASE_SERVER_PORT;
    opt  = getopt_long(argc, argv, optString, longOpts, &longIndex);
    while (opt != -1)
    {
        switch (opt)
        {
            case 'c':
                Port = BASE_SERVER_PORT + atoi(optarg) - 1;
                break;

            case 'p':
                Port = atoi(optarg);
                break;

            case 'v':
                ++ConsoleUtils_Verbosity;
                break;

            case '?':
                break;

            default:
                break;
        }

        opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    }

    InstantiateDisplays();

    /*
    ** socket creation
    */
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        CONSOLEUTILS_ERROR("%s: cannot open socket \n", argv[0]);
        exit(1);
    }

    /*
    ** bind local server port
    */
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(Port);
    rc                       = bind(sd, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if (rc < 0)
    {
        CONSOLEUTILS_ERROR("%s: cannot bind port number %d \n", argv[0], Port);
        exit(1);
    }

    CONSOLEUTILS_INFO("%s: waiting for data on port UDP %u\n", argv[0], Port);

    /* server infinite loop */
    while (1)
    {
        /*
        ** receive message
        */
        cliLen = sizeof(cliAddr);
        n = recvfrom(sd, TlmData.NetBuf, sizeof(TlmData.NetBuf), 0, (struct sockaddr *)&cliAddr, (socklen_t *)&cliLen);

        if (n < 0)
        {
            CONSOLEUTILS_ERROR("%s: cannot receive data \n", argv[0]);
            perror("recvfrom");
            break;
        }

        /*
        ** print received message
        */

        CONSOLEUTILS_INFO("Telemetry Packet From: %s:UDP%u, %u bytes\n",
                          inet_ntoa(cliAddr.sin_addr),
                          ntohs(cliAddr.sin_port),
                          n);

        DisplayObject(TlmData.NetBuf, n);

    } /* end of server infinite loop */

    DestroyDisplays();

    return 0;
}
