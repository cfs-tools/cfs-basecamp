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


// Standard CRC-32 polynomial in reversed bit order. (Ethernet, ZIP, etc.)
#define POLY 0xEDB88320


/******************************************************************************
** Function: CRC_32Bitwise
**
*/
uint32 CRC_32Bitwise(const uint8 *Buf, size_t BufLen, uint32 PrevCrc)
{
   
   uint32 Crc = ~PrevCrc; // Initialize with all 1s
    
   for (size_t i = 0; i < BufLen; i++)
   {
      Crc ^= Buf[i];
      for (int j = 0; j < 8; j++)
      {
         // Standard CRC32 polynomial 0xEDB88320 (reversed)
         Crc = (Crc >> 1) ^ (-(int32_t)(Crc & 1) & POLY);
      }
   }
   return ~Crc; // Final XOR (all 1s)
    
} /* End CRC_32Bitwise() */


/*
#include <stdio.h>
#include <stdint.h>

uint32_t crc32_bitwise(const uint8_t *data, size_t length, uint32_t previous_crc) {
    uint32_t crc = ~previous_crc; // Initialize with all 1s
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            // Standard CRC32 polynomial 0xEDB88320 (reversed)
            crc = (crc >> 1) ^ (-(int32_t)(crc & 1) & 0xEDB88320);
        }
    }
    return ~crc; // Final XOR (all 1s)
}

int main() {
    uint8_t buffer[] = "hello world";
    uint32_t result = crc32_bitwise(buffer, sizeof(buffer) - 1, 0);
    printf("CRC32: %08x\n", result);
    return 0;
}



import zlib

def calculate_file_crc(filename):
    crc = 0
    with open(filename, 'rb') as f:
        # Read file in chunks to handle large files efficiently
        for chunk in iter(lambda: f.read(4096), b""):
            crc = zlib.crc32(chunk, crc)
    return crc & 0xFFFFFFFF  # Ensure unsigned 32-bit result

# Usage
file_path = "example.txt"
print(f"CRC32: {calculate_file_crc(file_path):08x}")

*/