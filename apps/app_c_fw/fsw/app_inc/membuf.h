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
**    Define a global memory buffer that can be used for memory
**    maintenance demonstrations and exercises.
**
**  Notes:
**    1. This memory buffer was orginally created to support operational
**       scenarios for apps like Memory Manager and Memory Dwell. This header
**       was created to support direct app access if teh need arises.
**
*/

#ifndef _mem_buf_
#define _mem_buf_

/*
** Include Files
*/

#include "app_c_fw.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define MEMBUF_BYTE_LEN  512


/**********************/
/** Type Definitions **/
/**********************/

typedef uint8 MEM_BUF_ByteArray_t[MEMBUF_BYTE_LEN];

#endif /* _mem_buf_ */
