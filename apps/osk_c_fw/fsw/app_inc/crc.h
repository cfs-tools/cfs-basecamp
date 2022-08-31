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
**    None
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _crc_
#define _crc_


/*
** Includes
*/

#include "osk_c_fw_cfg.h"


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: CRC_32c
**
**   Compute 32-bit CRC using iSCSI polynomial
**
**   Notes:
**      1. Crc should be et to zero for the initial call in a running
**         computational scenario.
** 
*/
uint32 CRC_32c(uint32 Crc, const uint8 *Buf, size_t BufLen);


#endif /* _crc_ */
