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

/*
 * Command utility. This program will build a Command packet
 * with variable parameters and send it on a UDP network socket.
 * this program is primarily used to command a cFS flight software system.
 */

#ifdef jphfix
/* System define for endian functions */
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#endif

/*
 * System includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <endian.h>

#include "cmd_send.h"

/*
 * Defines
 */

/* Max sizes */
#define MAX_ENDIAN_SIZE 3 /* Maximum endian string size */

/* Endian strings - related to MAX_ENDIAN_SIZE, don't exceed */
#define ENDIAN_BIG    "BE"
#define ENDIAN_LITTLE "LE"

/*
 * Packet default values
 * NOTE: Defaulting for backwards compatibility, make sure values match protocol or override in cmd line
 */
#define DEFAULT_PROTOCOL  CmdSend_Protocol_cfsv1 /* cFS version 1 default */
#define DEFAULT_BIGENDIAN true                   /* Default to big endian */

/*
 * Parameter datatype structure
 */
typedef struct
{
    uint16_t CCSDS_Pri[3];  /* CCSDS Primary Header (always big endian)
                             * index  mask    ------------ description ----------------
                             *   0   0xE000 : Packet Version number: CCSDS Version 1 = b000
                             *   0   0x1000 : Packet Type:           0 = TLM, 1 = CMD
                             *   0   0x0800 : Sec Hdr Flag:          0 = absent, 1 = present
                             *   0   0x07FF : Application Process Identifier
                             *   1   0xC000 : Sequence Flags:        b00 = continuation
                             *                                       b01 = first segment
                             *                                       b10 = last segment
                             *                                       b11 = unsegmented
                             *   1   0x3FFF : Packet Sequence Count or Packet Name
                             *   2   0xFFFF : Packet Data Length: (total packet length) - 7
                             */
    uint16_t CCSDS_Ext[2];  /* CCSDS Extended Header (always big endian)
                             * index  mask    ------------ description ----------------
                             *   0   0xF800 : EDS Version for packet definition used
                             *   0   0x0400 : Endian:        Big = 0, Little (Intel) = 1
                             *   0   0x0200 : Playback flag: 0 = original, 1 = playback
                             *   0   0x01FF : APID Qualifier (Subsystem Identifier)
                             *   1   0xFFFF : APID Qualifier (System Identifier)
                             */
    uint16_t CFS_CmdSecHdr; /* cFS standard secondary command header (always big endian)
                             * index  mask    ------------ description ----------------
                             *   0   0x8000 : Reserved
                             *   0   0x7F00 : Command Function Code
                             *   1   0x00FF : Command Checksum
                             */

    bool PayloadError;
    bool BigEndian;         /* Endian default, false means little endian */
    bool Verbose;           /* Verbose option */
    bool IncludeCCSDSPri;   /* Include CCSDS Primary Header */
    bool IncludeCCSDSExt;   /* Include CCSDS Secondary Header */
    bool IncludeCFSSec;     /* Include cFS Command Secondary Header */
    bool OverridePktType;   /* Override packet type field */
    bool OverridePktSec;    /* Override packet secondary header exists field */
    bool OverridePktSeqFlg; /* Override packet sequence flags */
    bool OverridePktLen;    /* Override packet length field */
    bool OverridePktEndian; /* Override packet endian field */
    bool OverridePktCksum;  /* Override packet checksum */

    char LastErrorText[160];

    unsigned int payload_bytes;

    unsigned char Payload[CMDSEND_MAX_PACKET_SIZE]; /* Data packet to send */
} CommandData_t;

/*----------------------------------------------------------------
 *
 * Local Helper function
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t PassThru_SetProtocol(CommandData_t *cmd, CmdSend_Protocol_t Selected)
{
    /* Clear any default setting */
    cmd->IncludeCCSDSPri = false;
    cmd->IncludeCCSDSExt = false;
    cmd->IncludeCFSSec   = false;

    switch (Selected)
    {
        case CmdSend_Protocol_ccsdspri:
            cmd->IncludeCCSDSPri = true;
            break;
        case CmdSend_Protocol_ccsdsext:
            cmd->IncludeCCSDSPri = true;
            cmd->IncludeCCSDSExt = true;
            break;
        case CmdSend_Protocol_cfsv1:
            cmd->IncludeCCSDSPri = true;
            cmd->IncludeCFSSec   = true;
            break;
        case CmdSend_Protocol_cfsv2:
            cmd->IncludeCCSDSPri = true;
            cmd->IncludeCCSDSExt = true;
            cmd->IncludeCFSSec   = true;
            break;
        case CmdSend_Protocol_raw:
            break;

        default:
            return CmdSend_OptParse_INVALID;
            break;
    }

    return CmdSend_OptParse_ACCEPTED;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 * Process string protocol field
 *  - updates orig masked bits with big endian in masked/shifted
 *  - in = input string(unshifted)
 *  - mask = value mask, also used to calculate shift
 *  - fieldexists = process helper, returns if false
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t
PassThru_ProcessField(CommandData_t *cmd, uint16_t *orig, const char *in, const uint16_t mask, bool fieldexists)
{
    long int     templong;
    unsigned int shift = 0;
    char        *tail  = NULL;

    /* Check if protocol includes field */
    if (!fieldexists)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "ERROR: %s:%u - Field does not exist for selected protocol: %s\n",
                 __func__,
                 __LINE__,
                 in);
        return CmdSend_OptParse_INVALID;
    }

    /* Find shift from mask (already checked for non-zero) */
    while (((mask >> shift) & 0x1) == 0)
    {
        shift++;
    }

    errno    = 0;
    templong = strtoul(in, &tail, 0);
    if (errno != 0 || tail == in)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "ERROR: %s:%u - String conversion (%s): %s\n",
                 __func__,
                 __LINE__,
                 in,
                 strerror(errno));
        return CmdSend_OptParse_INVALID;
    }

    if (*tail != 0)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "ERROR: %s:%u - Trailing characters (%s) in parameter %s\n",
                 __func__,
                 __LINE__,
                 tail,
                 in);
        return CmdSend_OptParse_INVALID;
    }

    templong <<= shift;
    if ((templong & ~mask) != 0)
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "ERROR: %s:%u - Parameter 0x%lX (%s<<%u) exceeds mask 0x%X\n",
                 __func__,
                 __LINE__,
                 templong,
                 in,
                 shift,
                 mask);
        return CmdSend_OptParse_INVALID;
    }

    *orig = htobe16((be16toh(*orig) & ~mask) | (templong & mask));
    return CmdSend_OptParse_ACCEPTED;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 * Copy data into packet buffer
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t PassThru_CopyData(CommandData_t *cmd, char *in, unsigned int nbytes)
{
    /* Ensure space */
    if ((cmd->payload_bytes + nbytes) > sizeof(cmd->Payload))
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "ERROR %s:%u - Exceeded packet size, startbyte = %u, nbytes = %u, max = %u\n",
                 __func__,
                 __LINE__,
                 cmd->payload_bytes,
                 nbytes,
                 (unsigned)sizeof(cmd->Payload));
        return CmdSend_OptParse_INVALID;
    }

    /* Copy data into packet buffer and move start byte */
    memcpy(&cmd->Payload[cmd->payload_bytes], in, nbytes);
    cmd->payload_bytes += nbytes;

    return CmdSend_OptParse_ACCEPTED;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 * Pads the local buffer with zero fill
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t PassThru_PadData(CommandData_t *cmd, int val, unsigned int nbytes)
{
    /* Ensure space */
    if ((cmd->payload_bytes + nbytes) > sizeof(cmd->Payload))
    {
        snprintf(cmd->LastErrorText,
                 sizeof(cmd->LastErrorText),
                 "ERROR %s:%u - Exceeded packet size, startbyte = %u, nbytes = %u, max = %u\n",
                 __func__,
                 __LINE__,
                 cmd->payload_bytes,
                 nbytes,
                 (unsigned)sizeof(cmd->Payload));
        return CmdSend_OptParse_INVALID;
    }

    /* Copy data into packet buffer and move start byte */
    memset(&cmd->Payload[cmd->payload_bytes], val, nbytes);
    cmd->payload_bytes += nbytes;

    return CmdSend_OptParse_ACCEPTED;
}

/*----------------------------------------------------------------
 *
 * Local Helper function
 * Calculate cFS Secondary Header Checksum
 * Note - this matches cFS checksum calc in framework
 *
 *-----------------------------------------------------------------*/
unsigned char PassThru_CalcChecksum(unsigned char *bbuf, unsigned int nbytes)
{
    unsigned char checksum = 0xFF;

    for (unsigned int i = 0; i < nbytes; i++)
        checksum ^= bbuf[i];

    return checksum;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
void *PassThru_Instantiate(void)
{
    CommandData_t *obj;

    obj = malloc(sizeof(*obj));
    memset(obj, 0, sizeof(*obj));

    /* Set defaults */
    obj->BigEndian = DEFAULT_BIGENDIAN;
    PassThru_SetProtocol(obj, DEFAULT_PROTOCOL);

    return obj;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
CmdSend_OptParse_t PassThru_ParseOption(void *obj, const CmdSend_ArgV_t *ArgV)
{
    CmdSend_OptParse_t     retcode;
    CommandData_t         *cmd  = obj;
    char                  *tail = NULL;
    long long int          templl;
    long long unsigned int tempull;
    bool                   forcebigendian;

    /* Set defaults */
    retcode = CmdSend_OptParse_UNDEFINED;

    /* check if it is one of the forced big endian options */
    switch (ArgV->OptionId)
    {
        case CmdSend_OptionId_int16b:
        case CmdSend_OptionId_uint16b:
        case CmdSend_OptionId_int32b:
        case CmdSend_OptionId_uint32b:
        case CmdSend_OptionId_int64b:
        case CmdSend_OptionId_uint64b:
            forcebigendian = true;
            break;
        default:
            forcebigendian = false;
            break;
    }

    switch (ArgV->OptionId)
    {
        case CmdSend_OptionId_NONE:
            cmd->PayloadError = true;
            break;
        case CmdSend_OptionId_pktapid:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[0], ArgV->Text, 0x07FF, cmd->IncludeCCSDSPri);
            break;
        case CmdSend_OptionId_pktpb:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Ext[0], ArgV->Text, 0x0200, cmd->IncludeCCSDSExt);
            break;
        case CmdSend_OptionId_cmdcode:
        case CmdSend_OptionId_pktfc:
            retcode = PassThru_ProcessField(cmd, &cmd->CFS_CmdSecHdr, ArgV->Text, 0x7F00, cmd->IncludeCFSSec);
            break;
        case CmdSend_OptionId_pktedsver:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Ext[0], ArgV->Text, 0xF800, cmd->IncludeCCSDSExt);
            break;
        case CmdSend_OptionId_endian:
            if (strcmp(ArgV->Text, ENDIAN_LITTLE) == 0)
            {
                cmd->BigEndian = false;
                retcode        = CmdSend_OptParse_ACCEPTED;
            }
            else if (strcmp(ArgV->Text, ENDIAN_BIG) == 0)
            {
                cmd->BigEndian = true;
                retcode        = CmdSend_OptParse_ACCEPTED;
            }
            else
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "endian selection: \'%s\' incompatible for passthrough mode\n",
                         ArgV->Text);
                retcode = CmdSend_OptParse_INVALID;
            }
            break;
        case CmdSend_OptionId_pktseqflg:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[1], ArgV->Text, 0xC000, cmd->IncludeCCSDSPri);
            cmd->OverridePktSeqFlg = true;
            break;
        case CmdSend_OptionId_pktseqcnt:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[1], ArgV->Text, 0x3FFF, cmd->IncludeCCSDSPri);
            break;
        case CmdSend_OptionId_pktid:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[0], ArgV->Text, 0xFFFF, cmd->IncludeCCSDSPri);
            break;
        case CmdSend_OptionId_pktendian:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Ext[0], ArgV->Text, 0x0400, cmd->IncludeCCSDSExt);
            cmd->OverridePktEndian = true;
            break;
        case CmdSend_OptionId_pktlen:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[2], ArgV->Text, 0xFFFF, cmd->IncludeCCSDSPri);
            cmd->OverridePktLen = true;
            break;
        case CmdSend_OptionId_protocol:
            retcode = PassThru_SetProtocol(cmd, CmdSend_GetProtocolFromString(ArgV->Text));
            break;
        case CmdSend_OptionId_pktcksum:
            retcode = PassThru_ProcessField(cmd, &cmd->CFS_CmdSecHdr, ArgV->Text, 0x00FF, cmd->IncludeCFSSec);
            cmd->OverridePktCksum = true;
            break;
        case CmdSend_OptionId_pktsec:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[0], ArgV->Text, 0x0800, cmd->IncludeCCSDSPri);
            cmd->OverridePktSec = true;
            break;
        case CmdSend_OptionId_pkttype:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[0], ArgV->Text, 0x1000, cmd->IncludeCCSDSPri);
            cmd->OverridePktType = true;
            break;
        case CmdSend_OptionId_pktsubsys:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Ext[0], ArgV->Text, 0x01FF, cmd->IncludeCCSDSExt);
            break;
        case CmdSend_OptionId_pktver:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Pri[0], ArgV->Text, 0xE000, cmd->IncludeCCSDSPri);
            break;
        case CmdSend_OptionId_pktsys:
            retcode = PassThru_ProcessField(cmd, &cmd->CCSDS_Ext[1], ArgV->Text, 0xFFFF, cmd->IncludeCCSDSExt);
            break;
        case CmdSend_OptionId_byte:
        case CmdSend_OptionId_int8:
        {
            int8_t tempint8;

            templl   = strtoll(ArgV->Text, &tail, 0);
            tempint8 = templl;
            if (tail == ArgV->Text || tempint8 != templl)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not int8: \'%s\' -> %d\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (int)tempint8);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                retcode = PassThru_CopyData(cmd, (char *)&tempint8, sizeof(tempint8));
            }
            break;
        }
        case CmdSend_OptionId_double:
        {
            double   tempd;
            uint64_t tempuint64;

            tempd = strtod(ArgV->Text, &tail);
            if (tail == ArgV->Text)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not double: %s\n",
                         __func__,
                         __LINE__,
                         ArgV->Text);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                memcpy(&tempuint64, &tempd, sizeof(tempuint64));

                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempuint64 = htobe64(tempuint64);
                else
                    tempuint64 = htole64(tempuint64);

                retcode = PassThru_CopyData(cmd, (char *)&tempuint64, sizeof(tempuint64));
            }
            break;
        }
        case CmdSend_OptionId_float:
        {
            float    tempf;
            uint32_t tempuint32;

            tempf = strtof(ArgV->Text, &tail);
            if (tail == ArgV->Text)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not float: %s\n",
                         __func__,
                         __LINE__,
                         ArgV->Text);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                memcpy(&tempuint32, &tempf, sizeof(tempuint32));

                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempuint32 = htobe32(tempuint32);
                else
                    tempuint32 = htole32(tempuint32);

                retcode = PassThru_CopyData(cmd, (char *)&tempuint32, sizeof(tempuint32));
            }
            break;
        }
        case CmdSend_OptionId_int16b:
        case CmdSend_OptionId_half:
        case CmdSend_OptionId_int16:
        {
            int16_t tempint16;

            templl    = strtoll(ArgV->Text, &tail, 0);
            tempint16 = templl;
            if (tail == ArgV->Text || tempint16 != templl)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not int16: \'%s\' -> %d\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (int)tempint16);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempint16 = htobe16(tempint16);
                else
                    tempint16 = htole16(tempint16);

                retcode = PassThru_CopyData(cmd, (char *)&tempint16, sizeof(tempint16));
            }
            break;
        }
        case CmdSend_OptionId_long:
        case CmdSend_OptionId_word:
        case CmdSend_OptionId_int32b:
        case CmdSend_OptionId_int32:
        {
            int32_t tempint32;

            templl    = strtoll(ArgV->Text, &tail, 0);
            tempint32 = templl;
            if (tail == ArgV->Text || tempint32 != templl)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not int32: \'%s\' -> %ld\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (long)tempint32);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempint32 = htobe32(tempint32);
                else
                    tempint32 = htole32(tempint32);

                retcode = PassThru_CopyData(cmd, (char *)&tempint32, sizeof(tempint32));
            }
            break;
        }
        case CmdSend_OptionId_uint8:
        {
            uint8_t tempuint8;

            tempull   = strtoull(ArgV->Text, &tail, 0);
            tempuint8 = tempull;
            if (tail == ArgV->Text || tempuint8 != tempull)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not uint8: \'%s\' -> %u\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (unsigned int)tempuint8);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                retcode = PassThru_CopyData(cmd, (char *)&tempuint8, sizeof(tempuint8));
            }
            break;
        }
        case CmdSend_OptionId_uint16b:
        case CmdSend_OptionId_uint16:
        {
            uint16_t tempuint16;

            tempull    = strtoull(ArgV->Text, &tail, 0);
            tempuint16 = tempull;
            if (tail == ArgV->Text || tempuint16 != tempull)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not uint16: \'%s\' -> %u\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (unsigned int)tempuint16);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempuint16 = htobe16(tempuint16);
                else
                    tempuint16 = htole16(tempuint16);

                retcode = PassThru_CopyData(cmd, (char *)&tempuint16, sizeof(tempuint16));
            }
            break;
        }
        case CmdSend_OptionId_uint32b:
        case CmdSend_OptionId_uint32:
        {
            uint32_t tempuint32;

            tempull    = strtoull(ArgV->Text, &tail, 0);
            tempuint32 = tempull;
            if (tail == ArgV->Text || tempuint32 != tempull)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not uint32: \'%s\' -> %lu\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (unsigned long)tempuint32);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempuint32 = htobe32(tempuint32);
                else
                    tempuint32 = htole32(tempuint32);

                retcode = PassThru_CopyData(cmd, (char *)&tempuint32, sizeof(tempuint32));
            }
            break;
        }
        case CmdSend_OptionId_uint64b:
        case CmdSend_OptionId_uint64:
        {
            uint64_t tempuint64;

            tempull    = strtoull(ArgV->Text, &tail, 0);
            tempuint64 = tempull;
            if (tail == ArgV->Text || tempuint64 != tempull)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not uint64: \'%s\' -> %llu\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (unsigned long long)tempuint64);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempuint64 = htobe64(tempuint64);
                else
                    tempuint64 = htole64(tempuint64);

                retcode = PassThru_CopyData(cmd, (char *)&tempuint64, sizeof(tempuint64));
            }
            break;
        }
        case CmdSend_OptionId_int64b:
        case CmdSend_OptionId_int64:
        {
            int64_t tempint64;

            templl    = strtoll(ArgV->Text, &tail, 0);
            tempint64 = templl;
            if (tail == ArgV->Text || tempint64 != templl)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR %s:%u - Parameter not int64: \'%s\' -> %lld\n",
                         __func__,
                         __LINE__,
                         ArgV->Text,
                         (long long)tempint64);
                retcode = CmdSend_OptParse_INVALID;
            }
            if (tail != ArgV->Text && tempint64 == templl)
            {
                /* Endian conversion */
                if (cmd->BigEndian || forcebigendian)
                    tempint64 = htobe64(tempint64);
                else
                    tempint64 = htole64(tempint64);

                retcode = PassThru_CopyData(cmd, (char *)&tempint64, sizeof(tempint64));
            }
            break;
        }
        case CmdSend_OptionId_string:
        {
            size_t len;

            tempull = strtoull(ArgV->Text, &tail, 0);

            if (*tail != ':' || tempull == 0)
            {
                snprintf(cmd->LastErrorText,
                         sizeof(cmd->LastErrorText),
                         "ERROR: %s:%u - String format is NNN:string, not: \'%s\'\n",
                         __func__,
                         __LINE__,
                         ArgV->Text);
                retcode = CmdSend_OptParse_INVALID;
            }
            else
            {
                ++tail;
                len = strlen(tail);
                if (len > tempull)
                {
                    snprintf(cmd->LastErrorText,
                             sizeof(cmd->LastErrorText),
                             "ERROR: %s:%u - Trailing characters (%s) in argument: \'%s\'\n",
                             __func__,
                             __LINE__,
                             tail,
                             ArgV->Text);
                    retcode = CmdSend_OptParse_INVALID;
                }
                else
                {
                    /* Copy the data over */
                    retcode = PassThru_CopyData(cmd, tail, len);

                    /* Zero-pad any unused portion */
                    if (retcode == CmdSend_OptParse_ACCEPTED && len != tempull)
                    {
                        retcode = PassThru_PadData(cmd, 0, tempull - len);
                    }
                }
            }
            break;
        }
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
const char *PassThru_GetErrorText(void *obj)
{
    CommandData_t *cmd = obj;

    return cmd->LastErrorText;
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
bool PassThru_GetPackedObject(void *obj, void *buf, size_t *sz)
{
    CommandData_t *cmd = obj;
    uint8_t       *out = buf;
    size_t         nbytes;

    nbytes = cmd->payload_bytes;
    if (cmd->IncludeCCSDSPri)
    {
        nbytes += sizeof(cmd->CCSDS_Pri);
    }
    if (cmd->IncludeCCSDSExt)
    {
        nbytes += sizeof(cmd->CCSDS_Ext);
    }
    if (cmd->IncludeCFSSec)
    {
        nbytes += sizeof(cmd->CFS_CmdSecHdr);
    }

    if (nbytes > *sz)
    {
        return false;
    }

    /* Set non-overridden fields - PktType, PktSec, PktSeqFlg, PktLen, PktEndian */
    if (!cmd->OverridePktType)
    {
        cmd->CCSDS_Pri[0] |= htobe16(0x1000);
    }
    if (!cmd->OverridePktSec && cmd->IncludeCFSSec)
    {
        cmd->CCSDS_Pri[0] |= htobe16(0x0800);
    }
    if (!cmd->OverridePktSeqFlg)
    {
        cmd->CCSDS_Pri[1] |= htobe16(0xC000);
    }
    if (!cmd->OverridePktLen)
    {
        cmd->CCSDS_Pri[2] = htobe16(nbytes - 7);
    }
    if (!cmd->OverridePktEndian && !cmd->BigEndian)
    {
        cmd->CCSDS_Ext[0] |= htobe16(0x0400);
    }

    /* Copy selected header data (pre-checksum) */
    nbytes = 0;
    if (cmd->IncludeCCSDSPri)
    {
        memcpy(&out[nbytes], &cmd->CCSDS_Pri, sizeof(cmd->CCSDS_Pri));
        nbytes += sizeof(cmd->CCSDS_Pri);
    }
    if (cmd->IncludeCCSDSExt)
    {
        memcpy(&out[nbytes], &cmd->CCSDS_Ext, sizeof(cmd->CCSDS_Ext));
        nbytes += sizeof(cmd->CCSDS_Ext);
    }
    if (cmd->IncludeCFSSec)
    {
        memcpy(&out[nbytes], &cmd->CFS_CmdSecHdr, sizeof(cmd->CFS_CmdSecHdr));
        nbytes += sizeof(cmd->CFS_CmdSecHdr);

        if (!cmd->OverridePktCksum)
        {
            out[nbytes - 1] = PassThru_CalcChecksum(out, nbytes);
        }
    }

    memcpy(&out[nbytes], cmd->Payload, cmd->payload_bytes);
    nbytes += cmd->payload_bytes;

    *sz = nbytes;

    return true;
}

/*----------------------------------------------------------------
 *
 * API structure
 *
 *-----------------------------------------------------------------*/
/* clang-format off */
const CmdSend_Parser_API_t PassThru_API =
{
    .Name = "PassThrough",
    .Instantiate = PassThru_Instantiate,
    .ParseOption = PassThru_ParseOption,
    .GetErrorText = PassThru_GetErrorText,
    .GetPackedObject = PassThru_GetPackedObject
};
