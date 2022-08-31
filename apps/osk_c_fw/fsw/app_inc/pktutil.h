/*
**  Copyright 2022 bitValence, Inc.
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it
**  under the terms of the GNU Affero General Public License
**  as published by the Free Software Foundation; version 3 with
**  attribution addendums as found in the LICENSE.txt
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Affero General Public License for more details.
**
**  Purpose:
**    Provide general utilities used with command and telemetry packets.
**
**  Notes:
**    1. PktUtil_IsPacketFiltered originated from cfs_utils and the preserves
**       'N of X with offset O' algorithm.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _pkt_util_
#define _pkt_util_


/*
** Includes
*/

#include "osk_c_fw_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define PKTUTIL_MAX_APP_ID    (0x0800)  /* Maximum CCSDS v1 ApId */

#define PKTUTIL_CMD_HDR_BYTES (sizeof(CFE_MSG_CommandHeader_t))
#define PKTUTIL_CMD_HDR_WORDS (sizeof(CFE_MSG_CommandHeader_t)/2)

#define PKTUTIL_TLM_HDR_BYTES (sizeof(CFE_MSG_TelemetryHeader_t))
#define PKTUTIL_TLM_HDR_WORDS (sizeof(CFE_MSG_TelemetryHeader_t)/2)

#define PKTUTIL_16_MSB_SUBSECS_SHIFT  16
#define PKTUTIL_11_LSB_SECONDS_MASK   0x07FF
#define PKTUTIL_11_LSB_SECONDS_SHIFT  4
#define PKTUTIL_4_MSB_SUBSECS_MASK    0xF000
#define PKTUTIL_4_MSB_SUBSECS_SHIFT   12

/*
** cFE Bootes (6.8.*) removed the following macros from ccsds.h and did not define them anywhere else.
** cFE Caelum (7.0.*) added them back in a new file called cfe_endian.h. They are temporarily defined
** here just for the OSK Bootes release.
**
*/

/* 
** Macros to convert 16/32 bit types from platform "endianness" to Big Endian
** - cFE 7.0: cfe_endian.h defines these 
**
#ifdef SOFTWARE_BIG_BIT_ORDER
  #define CFE_MAKE_BIG16(n) (n)
  #define CFE_MAKE_BIG32(n) (n)
#else
  #define CFE_MAKE_BIG16(n) ( (((n) << 8) & 0xFF00) | (((n) >> 8) & 0x00FF) )
  #define CFE_MAKE_BIG32(n) ( (((n) << 24) & 0xFF000000) | (((n) << 8) & 0x00FF0000) | (((n) >> 8) & 0x0000FF00) | (((n) >> 24) & 0x000000FF) )
#endif
*/

/**********************/
/** Type Definitions **/
/**********************/

/*
** PktUtil_IsPacketFiltered() should be used by ground commands
** or tables that accept a filter type.
*/

typedef enum
{
   
   PKTUTIL_FILTER_ALWAYS     = 1,
   PKTUTIL_FILTER_BY_SEQ_CNT = 2,
   PKTUTIL_FILTER_BY_TIME    = 3,
   PKTUTIL_FILTER_NEVER      = 4
   
} PktUtil_FilterType_t;


/* 
** N of X packets ("group size") will be sent starting at offset O
*/ 
typedef struct
{
   
   uint16 N;
   uint16 X;
   uint16 O;
   
} PktUtil_FilterParam_t;

typedef struct
{
   
   PktUtil_FilterType_t   Type;
   PktUtil_FilterParam_t  Param;

} PktUtil_Filter_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: PktUtil_IsFilterTypeValid
**
** Notes:
**   1. Intended for for parameter validation. It uses uint16 because command
**      packet definitions typically don't use enumerated types so they can 
**      control the storage size (prior to C++11).
*/
bool PktUtil_IsFilterTypeValid(uint16 FilterType);


/******************************************************************************
** Function: PktUtil_IsPacketFiltered
**
*/
bool PktUtil_IsPacketFiltered(const CFE_MSG_Message_t *MsgPtr, const PktUtil_Filter_t *Filter);


#endif /* _pkt_util_ */
