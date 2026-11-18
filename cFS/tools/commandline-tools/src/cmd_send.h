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
#ifndef CMD_SEND_H
#define CMD_SEND_H

#include <stdbool.h>
#include <stddef.h>

#include "console_utils.h"

#define CMDSEND_MAX_PACKET_SIZE 2048 /* Max packet size, ref: CCSDS max = 65542, IPv4 UDP max = 65507 */

/* Protocol names - for interpreting protocol argument */
typedef enum
{
    CmdSend_Protocol_UNDEFINED,
    CmdSend_Protocol_raw,
    CmdSend_Protocol_ccsdspri,
    CmdSend_Protocol_ccsdsext,
    CmdSend_Protocol_cfsv1,
    CmdSend_Protocol_cfsv2,
    CmdSend_Protocol_MAX
} CmdSend_Protocol_t;

typedef enum
{
    CmdSend_OptParse_INVALID = -1,
    CmdSend_OptParse_UNDEFINED,
    CmdSend_OptParse_ACCEPTED,
} CmdSend_OptParse_t;

typedef enum
{
    CmdSend_OptionId_NONE, /* keep first */
    CmdSend_OptionId_help,
    CmdSend_OptionId_verbose,
    CmdSend_OptionId_pktapid,
    CmdSend_OptionId_pktpb,
    CmdSend_OptionId_cmdcode,
    CmdSend_OptionId_pktfc,
    CmdSend_OptionId_pktedsver,
    CmdSend_OptionId_endian,
    CmdSend_OptionId_pktseqflg,
    CmdSend_OptionId_pktname,
    CmdSend_OptionId_pktseqcnt,
    CmdSend_OptionId_host,
    CmdSend_OptionId_pktid,
    CmdSend_OptionId_pktendian,
    CmdSend_OptionId_pktlen,
    CmdSend_OptionId_port,
    CmdSend_OptionId_protocol,
    CmdSend_OptionId_pktcksum,
    CmdSend_OptionId_pktsec,
    CmdSend_OptionId_pkttype,
    CmdSend_OptionId_pktsubsys,
    CmdSend_OptionId_pktver,
    CmdSend_OptionId_pktsys,
    CmdSend_OptionId_byte,
    CmdSend_OptionId_int8,
    CmdSend_OptionId_double,
    CmdSend_OptionId_float,
    CmdSend_OptionId_half,
    CmdSend_OptionId_int16,
    CmdSend_OptionId_int16b,
    CmdSend_OptionId_int32b,
    CmdSend_OptionId_int64b,
    CmdSend_OptionId_long,
    CmdSend_OptionId_word,
    CmdSend_OptionId_int32,
    CmdSend_OptionId_uint8,
    CmdSend_OptionId_uint16,
    CmdSend_OptionId_uint32,
    CmdSend_OptionId_uint64,
    CmdSend_OptionId_int64,
    CmdSend_OptionId_string,
    CmdSend_OptionId_uint16b,
    CmdSend_OptionId_uint32b,
    CmdSend_OptionId_uint64b,
    CmdSend_OptionId_MAX /* keep last */

} CmdSend_OptionId_t;

typedef struct CmdSend_ArgV
{
    CmdSend_OptionId_t OptionId;
    const char        *Text;
} CmdSend_ArgV_t;

#ifdef jphfix
typedef struct CmdSend_OptInfo
{
    int NumOccurs;
    int FirstOccurs;
} CmdSend_OptInfo_t;
#endif

typedef struct CmdSend_Parser_API
{
    const char *Name;
    void *(*Instantiate)(void);
    CmdSend_OptParse_t (*ParseOption)(void *, const CmdSend_ArgV_t *);

    void (*ShowHelp)(void *);
    const char *(*GetErrorText)(void *);

    bool (*GetPackedObject)(void *, void *, size_t *);

} CmdSend_Parser_API_t;

extern const CmdSend_Parser_API_t PassThru_API;
extern const CmdSend_Parser_API_t EDS_API;

CmdSend_Protocol_t CmdSend_GetProtocolFromString(const char *protocol);

#endif
