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
#include <stdarg.h>
#include <limits.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>

#include "cmd_send.h"
#include "send_udp.h"

#define MAX_HOSTNAME_SIZE 128
#define MAX_PORT_SIZE     32

/* Destination default values */
#define DEFAULT_HOSTNAME "127.0.0.1" /* Local host */
#define DEFAULT_PORT     "1234"      /* Default cFS cpu1 port */

/* Protocol names - for interpreting protocol argument */
#define PROTOCOL_CCSDS_PRI "ccsdspri" /* CCSDS Primary header only */
#define PROTOCOL_CCSDS_EXT "ccsdsext" /* CCSDS Primary and Extended header only */
#define PROTOCOL_CFS_V1    "cfsv1"    /* CCSDS Primary header and cFS command secondary header */
#define PROTOCOL_CFS_V2    "cfsv2"    /* CCSDS Primary, Extended header, and cFS command secondary header */
#define PROTOCOL_RAW       "raw"      /* No predefined header */

const struct option CMDSEND_OPTION_LONGINFO[CmdSend_OptionId_MAX + 1] = {
    [CmdSend_OptionId_help]      = { "help",      no_argument,       NULL, '?' },
    [CmdSend_OptionId_verbose]   = { "verbose",   no_argument,       NULL, 'v' },
    [CmdSend_OptionId_pktapid]   = { "pktapid",   required_argument, NULL, 'A' },
    [CmdSend_OptionId_pktpb]     = { "pktpb",     required_argument, NULL, 'B' },
    [CmdSend_OptionId_cmdcode]   = { "cmdcode",   required_argument, NULL, 'C' },
    [CmdSend_OptionId_pktfc]     = { "pktfc",     required_argument, NULL, 'C' },
    [CmdSend_OptionId_pktedsver] = { "pktedsver", required_argument, NULL, 'D' },
    [CmdSend_OptionId_endian]    = { "endian",    required_argument, NULL, 'E' },
    [CmdSend_OptionId_pktseqflg] = { "pktseqflg", required_argument, NULL, 'F' },
    [CmdSend_OptionId_pktname]   = { "pktname",   required_argument, NULL, 'G' },
    [CmdSend_OptionId_pktseqcnt] = { "pktseqcnt", required_argument, NULL, 'G' },
    [CmdSend_OptionId_host]      = { "host",      required_argument, NULL, 'H' },
    [CmdSend_OptionId_pktid]     = { "pktid",     required_argument, NULL, 'I' },
    [CmdSend_OptionId_pktendian] = { "pktendian", required_argument, NULL, 'J' },
    [CmdSend_OptionId_pktlen]    = { "pktlen",    required_argument, NULL, 'L' },
    [CmdSend_OptionId_port]      = { "port",      required_argument, NULL, 'P' },
    [CmdSend_OptionId_protocol]  = { "protocol",  required_argument, NULL, 'Q' },
    [CmdSend_OptionId_pktcksum]  = { "pktcksum",  required_argument, NULL, 'R' },
    [CmdSend_OptionId_pktsec]    = { "pktsec",    required_argument, NULL, 'S' },
    [CmdSend_OptionId_pkttype]   = { "pkttype",   required_argument, NULL, 'T' },
    [CmdSend_OptionId_pktsubsys] = { "pktsubsys", required_argument, NULL, 'U' },
    [CmdSend_OptionId_pktver]    = { "pktver",    required_argument, NULL, 'V' },
    [CmdSend_OptionId_pktsys]    = { "pktsys",    required_argument, NULL, 'Y' },
    [CmdSend_OptionId_byte]      = { "byte",      required_argument, NULL, 'b' },
    [CmdSend_OptionId_int8]      = { "int8",      required_argument, NULL, 'b' },
    [CmdSend_OptionId_double]    = { "double",    required_argument, NULL, 'd' },
    [CmdSend_OptionId_float]     = { "float",     required_argument, NULL, 'f' },
    [CmdSend_OptionId_half]      = { "half",      required_argument, NULL, 'h' },
    [CmdSend_OptionId_int16]     = { "int16",     required_argument, NULL, 'h' },
    [CmdSend_OptionId_int16b]    = { "int16b",    required_argument, NULL, 'i' },
    [CmdSend_OptionId_int32b]    = { "int32b",    required_argument, NULL, 'j' },
    [CmdSend_OptionId_int64b]    = { "int64b",    required_argument, NULL, 'k' },
    [CmdSend_OptionId_long]      = { "long",      required_argument, NULL, 'l' },
    [CmdSend_OptionId_word]      = { "word",      required_argument, NULL, 'l' },
    [CmdSend_OptionId_int32]     = { "int32",     required_argument, NULL, 'l' },
    [CmdSend_OptionId_uint8]     = { "uint8",     required_argument, NULL, 'm' },
    [CmdSend_OptionId_uint16]    = { "uint16",    required_argument, NULL, 'n' },
    [CmdSend_OptionId_uint32]    = { "uint32",    required_argument, NULL, 'o' },
    [CmdSend_OptionId_uint64]    = { "uint64",    required_argument, NULL, 'p' },
    [CmdSend_OptionId_int64]     = { "int64",     required_argument, NULL, 'q' },
    [CmdSend_OptionId_string]    = { "string",    required_argument, NULL, 's' },
    [CmdSend_OptionId_uint16b]   = { "uint16b",   required_argument, NULL, 'w' },
    [CmdSend_OptionId_uint32b]   = { "uint32b",   required_argument, NULL, 'x' },
    [CmdSend_OptionId_uint64b]   = { "uint64b",   required_argument, NULL, 'y' },
};

/*
 * getopts parameter passing options string
 */
static const char *CMDSEND_OPTSTRING = "A:B:C:D:E:F:G:H:I:J:L:P:Q:R:S:T:U:V:Y:b:d:f:h:i:j:k:l:m:n:o:p:q:s:vw:x:y:?";

static const CmdSend_Parser_API_t *CMDSEND_PARSER_API[] = {
#ifdef CFE_EDS_ENABLED
    &EDS_API,
#endif
    &PassThru_API
};

#define CMDSEND_MAX_NUM_PARSERS (sizeof(CMDSEND_PARSER_API) / sizeof(CMDSEND_PARSER_API[0]))

typedef struct CmdSend_OptParser
{
    void *Obj;
    bool  IsOK;
} CmdSend_OptParser_t;

/*
 * Parameter datatype structure
 */
typedef struct
{
    bool ShowHelp;

    char HostName[MAX_HOSTNAME_SIZE]; /* Host name like "localhost" or "192.168.0.121" */
    char PortNum[MAX_PORT_SIZE];      /* Port number */

    int             OptionCount;
    int             NonOptionCount;
    CmdSend_ArgV_t *ArgV;

    int                 NumParsers;
    int                 GoodParsers;
    CmdSend_OptParser_t Parser[CMDSEND_MAX_NUM_PARSERS];

    size_t  ContentSize;
    uint8_t NetBuf[CMDSEND_MAX_PACKET_SIZE];

} CommandData_t;

CommandData_t CommandData;

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Display program usage and exits
 *
 *-----------------------------------------------------------------*/
void DisplayUsage(char *Name)
{
#ifdef jphfix
    char endian[] = ENDIAN_LITTLE;

    if (DEFAULT_BIGENDIAN)
        strcpy(endian, ENDIAN_BIG);

    printf("%s -- Command Client.\n", Name);
    printf("  - General options:\n");
    printf("    -v, --verbose: Extra output\n");
    printf("    -?, --help: print options and exit\n");
    printf("  - Destination options:\n");
    printf("    -H, --host: Destination hostname or IP address (Default = %s)\n", DEFAULT_HOSTNAME);
    printf("    -P, --port: Destination port (default = %s)\n", DEFAULT_PORT);
    printf("  - Packet format options:\n");
    printf("    -E, --endian: Default endian for unnamed fields/payload: [%s|%s] (default = %s)\n",
           ENDIAN_BIG,
           ENDIAN_LITTLE,
           endian);
    printf("    -Q, --protocol: Sets allowed named fields and header layout (default = %s)\n", DEFAULT_PROTOCOL);
    printf("        %8s = no predefined fields/layout\n", PROTOCOL_RAW);
    printf("        %8s = CCSDS Pri header only\n", PROTOCOL_CCSDS_PRI);
    printf("        %8s = CCSDS Pri and Ext headers\n", PROTOCOL_CCSDS_EXT);
    printf("        %8s = CCSDS Pri and cFS Cmd Sec headers\n", PROTOCOL_CFS_V1);
    printf("        %8s = CCSDS Pri, Ext, and cFS Cmd Sec headers\n", PROTOCOL_CFS_V2);
    printf("  - CCSDS Primary Header named fields (protocol=[%s|%s|%s|%s])\n",
           PROTOCOL_CCSDS_PRI,
           PROTOCOL_CCSDS_EXT,
           PROTOCOL_CFS_V1,
           PROTOCOL_CFS_V2);
    printf("    -I, --pktid: macro for setting first 16 bits of CCSDS Primary header\n");
    printf("    -V, --pktver: Packet version number (range=0-0x7)\n");
    printf("    -T, --pkttype: !OVERRIDE! Packet type (default is cmd, 0=tlm, 1=cmd)\n");
    printf("    -S, --pktsec: !OVERRIDE! Secondary header flag (default set from protocol, 0=absent, 1=present)\n");
    printf("    -A, --pktapid: Application Process Identifier (range=0-0x7FF)\n");
    printf("    -F, --pktseqflg: !OVERRIDE! Sequence Flags (default unsegmented, 0=continuation, 1=first, 2=last, "
           "3=unsegmented)\n");
    printf("    -G, --pktseqcnt, --pktname: Packet sequence count or Packet name (range=0-0x3FFF)\n");
    printf("    -L, --pktlen: !OVERRIDE! Packet data length (default will calculate value, range=0-0xFFFF)\n");
    printf("  - CCSDS Extended Header named fields (protocol=[%s|%s])\n", PROTOCOL_CCSDS_EXT, PROTOCOL_CFS_V2);
    printf("    -D, --pktedsver: EDS version (range=0-0x1F)\n");
    printf("    -J, --pktendian: !OVERRIDE! Endian (default uses endian, 0=big, 1=little(INTEL))\n");
    printf("    -B, --pktpb: Playback field (0=original, 1=playback)\n");
    printf("    -U, --pktsubsys: APID Qualifier Subsystem (range=0-0x1FF)\n");
    printf("    -Y, --pktsys: APID Qualifier System (range=0-0xFFFF)\n");
    printf("  - cFS Command Secondary Header named fields (protocol=[%s|%s])\n", PROTOCOL_CFS_V1, PROTOCOL_CFS_V2);
    printf("    -C, --pktfc, --cmdcode: Command function code (range=0-0x7F)\n");
    printf("    -R, --pktcksum: !OVERRIDE! Packet checksum (default will calculate value, range=0-0xFF)\n");
    printf("  - Raw values converted to selected endian where applicable\n");
    printf("    -b, --int8, --byte: Signed 8 bit int (range=-127-127)\n");
    printf("    -m, --uint8: Unsigned 8 bit int (range=0-0xFF)\n");
    printf("    -h, --int16, --half: Signed 16 bit int (range=-32767-32767)\n");
    printf("    -n, --uint16: Unsigned 16 bit int (range=0-0xFFFF)\n");
    printf("    -l, --int32, --long, --word: Signed 32 bit int (range=-2147483647-2147483647)\n");
    printf("    -o, --uint32: Unsigned 32 bit int (range=0-0xFFFFFFFF)\n");
    printf("    -q, --int64: Signed 64 bit int (range=-9223372036854775807-9223372036854775807)\n");
    printf("    -p, --uint64: Unsigned 64 bit int (range=0-0xFFFFFFFFFFFFFFFF)\n");
    printf("    -d, --double: Double format (caution - host format w/ converted endian, may not match target)\n");
    printf("    -f, --float: Float format (caution - host format w/ converted endian, may not match target)\n");
    printf("    -s, --string: Fixed length string, NNN:String where NNN is fixed length (0 padded)\n");
    printf("  - Raw values always big endian (even if endian=%s)\n", ENDIAN_LITTLE);
    printf("    -i, --int16b: Big endian signed 16 bit int (range=-32767-32767)\n");
    printf("    -j, --int32b: Big endian signed 32 bit int (range=-2147483647-2147483647)\n");
    printf("    -k, --int64b: Big endian signed 64 bit int (range=-9223372036854775807-9223372036854775807)\n");
    printf("    -w, --uint16b: Big endian unsigned 16 bit int (range=0-0xFFFF)\n");
    printf("    -x, --uint32b: Big endian unsigned 32 bit int (range=0-0xFFFFFFFF)\n");
    printf("    -y, --uint64b: Big endian unsigned 64 bit int (range=0-0xFFFFFFFFFFFFFFFF)\n");
    printf(" \n");
#endif
    printf("Examples:\n");
    printf("  ./cmdUtil --host=localhost --port=1234 --pktid=0x1803 --pktfc=3 --int16=100 --string=16:ES_APP\n");
    printf("  ./cmdUtil -ELE -C2 -A6 -n2 # Processor reset on little endian, using defaults\n");
    printf("  ./cmdUtil --endian=LE --protocol=raw --uint64b=0x1806C000000302DD --uint16=2\n");
    printf("  ./cmdUtil --pktver=1 --pkttype=0 --pktsec=0 --pktseqflg=2 --pktlen=0xABC --pktcksum=0\n");
    printf(
        "  ./cmdUtil -Qcfsv2 --pktedsver=0xA --pktendian=1 --pktpb=1 --pktsubsys=0x123 --pktsys=0x4321 --pktfc=0xB\n");
    printf(" \n");
    exit(EXIT_SUCCESS);
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Processes command line options.  This builds a local list of options
 * along with the text/argument associated with them.
 *
 *-----------------------------------------------------------------*/
void CollectOptions(int argc, char *argv[])
{
    int                  option_argc;
    int                  nonoption_argc;
    size_t               size_req;
    int                  opt;
    CmdSend_ArgV_t      *argv_ptr;
    const struct option *opt_ptr;
    char               **nonoption_argv;
    uint8_t              reverse_opt_lookup[0x80];

    /* build a reverse lookup table */
    memset(reverse_opt_lookup, 0, sizeof(reverse_opt_lookup));
    for (opt = 1; opt < CmdSend_OptionId_MAX; ++opt)
    {
        reverse_opt_lookup[CMDSEND_OPTION_LONGINFO[opt].val] = opt;
    }

    /* Make 1 pass to count the options */
    option_argc = 0;
    do
    {
        opt = getopt_long(argc, argv, CMDSEND_OPTSTRING, &CMDSEND_OPTION_LONGINFO[1], NULL);
        if (opt < 0)
        {
            break;
        }
        ++option_argc;
    } while (true);

    nonoption_argc = argc - optind;
    nonoption_argv = &argv[optind];

    size_req = sizeof(CmdSend_ArgV_t) * (nonoption_argc + option_argc);

    if (size_req > 0)
    {
        CommandData.ArgV = malloc(size_req);
        if (CommandData.ArgV == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        memset(CommandData.ArgV, 0, size_req);
    }

    /* Make second pass to categorize the options */
    optind = 1;
    while (CommandData.OptionCount < option_argc)
    {
        opt = getopt_long(argc, argv, CMDSEND_OPTSTRING, &CMDSEND_OPTION_LONGINFO[1], NULL);
        if (opt < 0)
        {
            break;
        }

        argv_ptr = &CommandData.ArgV[CommandData.OptionCount];

        argv_ptr->OptionId = reverse_opt_lookup[opt];
        opt_ptr            = &CMDSEND_OPTION_LONGINFO[argv_ptr->OptionId];
        if (opt_ptr->val != opt || argv_ptr->OptionId == CmdSend_OptionId_help)
        {
            CommandData.ShowHelp = true;
        }
        else if (argv_ptr->OptionId == CmdSend_OptionId_verbose)
        {
            ++ConsoleUtils_Verbosity;
        }
        else if (argv_ptr->OptionId == CmdSend_OptionId_host)
        {
            strncpy(CommandData.HostName, optarg, sizeof(CommandData.HostName) - 1);
        }
        else if (argv_ptr->OptionId == CmdSend_OptionId_port)
        {
            strncpy(CommandData.PortNum, optarg, sizeof(CommandData.PortNum) - 1);
        }
        else if (opt_ptr->has_arg != no_argument && optarg != NULL)
        {
            argv_ptr->Text = optarg;
        }

        ++CommandData.OptionCount;
    }

    while (CommandData.NonOptionCount < nonoption_argc)
    {
        argv_ptr       = &CommandData.ArgV[CommandData.OptionCount + CommandData.NonOptionCount];
        argv_ptr->Text = nonoption_argv[CommandData.NonOptionCount];
        ++CommandData.NonOptionCount;
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Convert a protocol string to the corresponding enum
 *
 *-----------------------------------------------------------------*/
CmdSend_Protocol_t CmdSend_GetProtocolFromString(const char *protocol)
{
    CmdSend_Protocol_t Selected;

    Selected = CmdSend_Protocol_UNDEFINED;

    if (strcmp(protocol, PROTOCOL_CCSDS_PRI) == 0)
    {
        Selected = CmdSend_Protocol_ccsdspri;
    }
    else if (strcmp(protocol, PROTOCOL_CCSDS_EXT) == 0)
    {
        Selected = CmdSend_Protocol_ccsdsext;
    }
    else if (strcmp(protocol, PROTOCOL_CFS_V1) == 0)
    {
        Selected = CmdSend_Protocol_cfsv1;
    }
    else if (strcmp(protocol, PROTOCOL_CFS_V2) == 0)
    {
        Selected = CmdSend_Protocol_cfsv2;
    }
    else if (strcmp(protocol, PROTOCOL_RAW) != 0)
    {
        CONSOLEUTILS_ERROR("ERROR: Invalid protocol: %s\n", protocol);
    }

    return Selected;
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Instantiates the parser/encoder objects
 *
 *-----------------------------------------------------------------*/
void InstantiateParsers(void)
{
    CmdSend_OptParser_t        *parser;
    const CmdSend_Parser_API_t *API;
    int                         j;

    for (j = 0; j < CMDSEND_MAX_NUM_PARSERS; ++j)
    {
        API = CMDSEND_PARSER_API[j];
        if (API == NULL)
        {
            break;
        }

        parser      = &CommandData.Parser[j];
        parser->Obj = API->Instantiate();
        if (parser->Obj != NULL)
        {
            parser->IsOK = true;
            ++CommandData.GoodParsers;
        }
    }
    CommandData.NumParsers = j;
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Destroys the parser/encoder objects
 *
 *-----------------------------------------------------------------*/
void DestroyParsers(void)
{
    CmdSend_OptParser_t *parser;
    int                  j;

    for (j = 0; j < CommandData.NumParsers; ++j)
    {
        parser = &CommandData.Parser[j];
        if (parser->Obj != NULL)
        {
            free(parser->Obj);
            parser->Obj = NULL;
        }
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Calls the given function for every parser/encoder
 *
 *-----------------------------------------------------------------*/
void ForeachParser(void (*Func)(const CmdSend_Parser_API_t *, CmdSend_OptParser_t *, void *), void *Arg)
{
    int j;

    for (j = 0; j < CommandData.NumParsers; ++j)
    {
        Func(CMDSEND_PARSER_API[j], &CommandData.Parser[j], Arg);
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Used with ForeachParser to do command line option processing
 *
 *-----------------------------------------------------------------*/
void DoParseOption(const CmdSend_Parser_API_t *API, CmdSend_OptParser_t *parser, void *argv_ptr)
{
    if (parser->IsOK)
    {
        if (API->ParseOption(parser->Obj, argv_ptr) == CmdSend_OptParse_INVALID)
        {
            parser->IsOK = false;
            --CommandData.GoodParsers;
        }
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Used with ForeachParser to show error messages
 *
 *-----------------------------------------------------------------*/
void DoShowError(const CmdSend_Parser_API_t *API, CmdSend_OptParser_t *parser, void *arg)
{
    if (parser->Obj != NULL && API->GetErrorText != NULL)
    {
        CONSOLEUTILS_ERROR("%s: %s\n", API->Name, API->GetErrorText(parser->Obj));
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Used with ForeachParser to show help information
 *
 *-----------------------------------------------------------------*/
void DoShowHelp(const CmdSend_Parser_API_t *API, CmdSend_OptParser_t *parser, void *arg)
{
    if (parser->Obj != NULL && API->ShowHelp != NULL)
    {
        API->ShowHelp(parser->Obj);
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Used with ForeachParser to do encoding of output message
 *
 *-----------------------------------------------------------------*/
void DoEncode(const CmdSend_Parser_API_t *API, CmdSend_OptParser_t *parser, void *arg)
{
    size_t ContentSize;
    bool   Result;

    Result      = false;
    ContentSize = sizeof(CommandData.NetBuf);
    if (CommandData.ContentSize == 0 && parser->IsOK)
    {
        Result = API->GetPackedObject(parser->Obj, CommandData.NetBuf, &ContentSize);
    }

    if (Result)
    {
        CONSOLEUTILS_INFO("Using result from %s encoder\n", API->Name);
        CommandData.ContentSize = ContentSize;
    }
}

/*----------------------------------------------------------------
 *
 * MAIN ROUTINE
 *
 *-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int             i;
    CmdSend_ArgV_t *argv_ptr;

    memset(&CommandData, 0, sizeof(CommandData));

    /* Defaults */
    strncpy(CommandData.HostName, DEFAULT_HOSTNAME, sizeof(CommandData.HostName) - 1);
    strncpy(CommandData.PortNum, DEFAULT_PORT, sizeof(CommandData.PortNum) - 1);

    CollectOptions(argc, argv);

    for (i = 0; i < CommandData.OptionCount; ++i)
    {
        argv_ptr = &CommandData.ArgV[i];
    }

    for (i = 0; i < CommandData.NonOptionCount; ++i)
    {
        argv_ptr = &CommandData.ArgV[CommandData.OptionCount + i];
    }

    InstantiateParsers();

    for (i = 0; CommandData.GoodParsers > 0 && i < (CommandData.OptionCount + CommandData.NonOptionCount); ++i)
    {
        argv_ptr = &CommandData.ArgV[i];
        if (argv_ptr->Text)
        {
            ForeachParser(DoParseOption, argv_ptr);
        }
    }

    if (CommandData.ShowHelp)
    {
        ForeachParser(DoShowHelp, NULL);
    }
    else
    {
        ForeachParser(DoEncode, NULL);

        if (CommandData.ContentSize == 0)
        {
            printf("Option parsing failed:\n");
            ForeachParser(DoShowError, NULL);
        }
        else
        {
            ConsoleUtils_Hexdump(CommandData.NetBuf, CommandData.ContentSize);
            SendUdp(CommandData.HostName, CommandData.PortNum, CommandData.NetBuf, CommandData.ContentSize);
        }
    }

    DestroyParsers();

    if (CommandData.ShowHelp)
    {
        DisplayUsage(argv[0]);
    }

    return EXIT_SUCCESS;
}
