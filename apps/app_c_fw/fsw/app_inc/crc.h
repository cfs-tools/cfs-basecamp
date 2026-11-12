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
*/

#ifndef _crc_
#define _crc_


/*
** Includes
*/

#include "app_c_fw_cfg.h"


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: CRC_32Bitwise
**
**   Compute standard 32-bit CRC (Ethernet, ZIP, etc.)
**
**   Notes:
**      1. Crc should be set to zero for the initial call in a running
**         computational scenario.
**      2. Returns a new CRC that should be passed in as th.e PrevCrc when
**         computing a CRC on a large buffer that is done in multiple 
**         segments
** 
*/
uint32 CRC_32Bitwise(const uint8 *Buf, size_t BufLen, uint32 PrevCrc);

#endif /* _crc_ */
