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
**    Provide utilities that compute CRCs.
**
**  Notes:
**    1. See https://stackoverflow.com/questions/27939882/fast-crc-algorithm
**    2. Th efollowing algorithm was suggessted in a comment as potentially more
**       efficient:  crc = (crc >> 1) ^ (POLY & (0 - (crc & 1)));
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/


/*
** Includes
*/

#include <stddef.h>
#include <stdint.h>

#include "crc.h"


/***********************/
/** Macro Definitions **/
/***********************/


/* CRC-32C (iSCSI) polynomial in reversed bit order. */
#define POLY 0x82f63b78

/* CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order. */
/* #define POLY 0xedb88320 */


/******************************************************************************
** Function: CRC_32c
**
** Compute 32-bit CRC using iSCSI polynomial
*/
uint32 CRC_32c(uint32 Crc, const uint8 *Buf, size_t BufLen)
{        

   int k;

   Crc = ~Crc;
   
   while (BufLen--)
   {
   
      Crc ^= *Buf++;
      for (k = 0; k < 8; k++)
         Crc = Crc & 1 ? (Crc >> 1) ^ POLY : Crc >> 1;
   }
   
   return ~Crc;


} /* End CRC_32c() */


