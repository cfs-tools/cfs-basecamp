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


/*********************/
/** Local Functions **/
/*********************/


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


