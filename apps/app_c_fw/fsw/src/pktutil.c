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


/*
** Includes
*/

#include <string.h>

#include "cfe.h"
#include "pktutil.h"

/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int HexChar2Bin(char *ValOut, const char HexIn);


/******************************************************************************
** Function: PktUtil_HexDecode
**
**   1. InBuf must be formatted identically to the algorithm used by 
**      PktUtil_HexEncode() which also means in must contain an even number
**      of bytes.
*/
size_t PktUtil_HexDecode(uint8 *OutBuf, const char *InBuf, size_t BufLen)
{

   size_t Len = BufLen;
   size_t i;
   char   HiNibble, LoNibble;

   if (InBuf == NULL || *InBuf == '\0' || OutBuf == NULL)
      return 0;
   
   if (BufLen == 0)
   {
      Len = strlen(InBuf);
   }
   if (Len % 2 != 0)
      return 0;
   Len /= 2;
   memset(OutBuf, 'A', Len);
   for (i=0; i < Len; i++)
   {
      if (!HexChar2Bin(&HiNibble, InBuf[i*2]) || !HexChar2Bin(&LoNibble, InBuf[i*2+1]))
      {
         return 0;
      }
      (OutBuf)[i] = (HiNibble << 4) | LoNibble;
   }

   return Len;

} /* End PktUtil_HexDecode() */


/******************************************************************************
** Function: PktUtil_HexEncode
**
** Notes:
**   1. Each binary numeric value is encoded using 2 hex digits regardless of 
**      whether the numeric value could be represented by one digit. Each byte
**      has a value between 0-255 and is represented by 0x00-0xFF. As a result,
**      encoded buffer will always be twice the size of binary.
**   2. The caller is responsible for ensuring the output buffer is big enough
**      to hold the encoded binary.  
*/
void PktUtil_HexEncode(char *OutBuf, const uint8 *InBuf, size_t BufLen)
{
   size_t  i;

   for (i=0; i < BufLen; i++)
   {
      OutBuf[i*2]   = "0123456789ABCDEF"[InBuf[i] >> 4];
      OutBuf[i*2+1] = "0123456789ABCDEF"[InBuf[i] & 0x0F];
   }
   OutBuf[BufLen*2] = '\0';

} /* End PktUtil_HexEncode() */


/******************************************************************************
** Function: PktUtil_IsFilterTypeValid
**
** Notes:
**   1. Intended for for parameter validation. It uses uint16 becaue command
**      packet definitions typically don't use enumerated types so they can 
**      control the storage size (prior to C++11).
*/
bool PktUtil_IsFilterTypeValid(uint16 FilterType)
{

   return ((FilterType >= PKTUTIL_FILTER_ALWAYS) &&
           (FilterType <= PKTUTIL_FILTER_NEVER));


} /* End PktUtil_IsFilterTypeValid() */


/******************************************************************************
** Function: PktUtil_IsPacketFiltered
**
** Algorithm
**   N = The filter will pass this many packets
**   X = out of every group of this many packets
**   O = starting at this offset within the group
*/
bool PktUtil_IsPacketFiltered(const CFE_MSG_Message_t *MsgPtr, const PktUtil_Filter_t *Filter)
{        

   bool PacketIsFiltered = true;
   CFE_TIME_SysTime_t PacketTime;
   CFE_MSG_SequenceCount_t SeqCnt;
   uint16 FilterValue;
   uint16 Seconds;
   uint16 Subsecs;

   if (Filter->Type == PKTUTIL_FILTER_ALWAYS) return true;
   if (Filter->Type == PKTUTIL_FILTER_NEVER)  return false;
   
   /* 
   ** Default to packet being filtered so only need to check for valid algorithm
   ** parameters. Any invalid algorithm parameter or undefined filter type results
   ** in packet being filtered.
   **  X: Group size of zero will result in divide by zero
   **  N: Pass count of zero will result in zero packets, so must be non-zero
   **  N <= X: Pass count cannot exceed group size
   **  O <  X: Group offset must be less than group size
   */ 
   if ((Filter->Param.X != 0) && 
       (Filter->Param.N != 0) && 
       (Filter->Param.N <= Filter->Param.X) &&
       (Filter->Param.O <  Filter->Param.X))
   {

      if (Filter->Type == PKTUTIL_FILTER_BY_SEQ_CNT) {
      
         CFE_MSG_GetSequenceCount(MsgPtr, &SeqCnt);
         FilterValue = (uint16)SeqCnt; 

      }
      else if (Filter->Type == PKTUTIL_FILTER_BY_TIME)
      {
         
         CFE_MSG_GetMsgTime(MsgPtr, &PacketTime);  
   
         Seconds = ((uint16)PacketTime.Seconds) & PKTUTIL_11_LSB_SECONDS_MASK;

         Subsecs = (((uint16)PacketTime.Subseconds) >> PKTUTIL_16_MSB_SUBSECS_SHIFT) & PKTUTIL_4_MSB_SUBSECS_MASK;

         /* Merge seconds and subseconds into a packet filter value */
         Seconds = Seconds << PKTUTIL_11_LSB_SECONDS_SHIFT;
         Subsecs = Subsecs >> PKTUTIL_4_MSB_SUBSECS_SHIFT;

         FilterValue = Seconds | Subsecs;
            
      } /* End if filter by time */

      if (FilterValue >= Filter->Param.O)
      {

         if (((FilterValue - Filter->Param.O) % Filter->Param.X) < Filter->Param.N)
         {

            PacketIsFiltered = false;
      
         }
      }
        
   } /* End if valid algorithm parameters */
    
   return PacketIsFiltered;

} /* End of PktUtil_IsPacketFiltered() */


/******************************************************************************
** Function: HexChar2Bin
**
*/
static int HexChar2Bin(char *ValOut, const char HexIn)
{

   if (HexIn >= '0' && HexIn <= '9') 
   {
      *ValOut = HexIn - '0';
   }
   else if (HexIn >= 'A' && HexIn <= 'F')
   {
      *ValOut = HexIn - 'A' + 10;
   }
   else if (HexIn >= 'a' && HexIn <= 'f')
   {
      *ValOut = HexIn - 'a' + 10;
   }
   else
   {
      return 0;
   }

   return 1;
	
} /* End HexChar2Bin() */

