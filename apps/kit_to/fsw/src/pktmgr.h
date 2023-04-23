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
**    Manage the Packet Table that defines which packets will be sent
**    from the software bus to a socket.
**
**  Notes:
**    1. This has some of the features of a flight app such as packet filtering
**       but it would need design/code reviews to transition it to a flight
**       mission. For starters it uses UDP sockets and it doesn't regulate
**       output bit rates.  
**    2. The term packet is used in a generic sense to refer to a telemetry 
**       packet. A packet has a message ID that corresponds to a cFE message
**       ID. KIT_TO stores its message ID as an integer and the cFE provides
**       conversion functions that translate from a 'value' to the cFE's SB
**       message ID repreentation.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/
#ifndef _pktmgr_
#define _pktmgr_

/*
** Includes
*/

#include "app_cfg.h"
#include "pkttbl.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define PKTMGR_IP_STR_LEN  16


/*
** Event Message IDs
*/
#define PKTMGR_SOCKET_SEND_ERR_EID      (PKTMGR_BASE_EID +  0)
#define PKTMGR_LOAD_TBL_EID             (PKTMGR_BASE_EID +  1)
#define PKTMGR_LOAD_TBL_ENTRY_EID       (PKTMGR_BASE_EID +  2)
#define PKTMGR_TLM_ENA_OUTPUT_EID       (PKTMGR_BASE_EID +  3)
#define PKTMGR_ADD_PKT_EID              (PKTMGR_BASE_EID +  4)
#define PKTMGR_REMOVE_PKT_EID           (PKTMGR_BASE_EID +  5)
#define PKTMGR_REMOVE_ALL_PKTS_EID      (PKTMGR_BASE_EID +  6)
#define PKTMGR_DESTRUCTOR_INFO_EID      (PKTMGR_BASE_EID +  7)
#define PKTMGR_UPDATE_FILTER_CMD_EID    (PKTMGR_BASE_EID +  8)
#define PKTMGR_SET_TLM_SOURCE_CMD_EID   (PKTMGR_BASE_EID +  9)
#define PKTMGR_SUBSCRIBE_EID            (PKTMGR_BASE_EID + 10)
#define PKTMGR_FORWARD_EID              (PKTMGR_BASE_EID + 11)
#define PKTMGR_UNWRAP_EID               (PKTMGR_BASE_EID + 12)
#define PKTMGR_DEBUG_EID                (PKTMGR_BASE_EID + 13)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
** - See EDS command definitions in app_c_demo.xml
*/


/******************************************************************************
** Telmetery Packets
** - See EDS command definitions in app_c_demo.xml
*/


/******************************************************************************
** Packet Manager Class
*/

/*
** Packet Manager Statistics
** - Stats are computed over the OutputTlmInterval
*/
typedef enum
{

   PKTMGR_STATS_INIT_CYCLE    = 1,
   PKTMGR_STATS_INIT_INTERVAL,
   PKTMGR_STATS_VALID
   
} PKTMGR_StatsState_t;

typedef struct
{

   uint16  InitCycles;         /* 0: Init done, >0: Number of remaining init cycles  */  
   
   double  OutputTlmInterval;  /* ms between calls to PKTMGR_OutputTelemetry()    */  
   double  IntervalMilliSecs;  /* Number of ms in the current computational cycle */
   uint32  IntervalPkts;
   uint32  IntervalBytes;
   
   CFE_TIME_SysTime_t PrevTime; 
   double  PrevIntervalAvgPkts;
   double  PrevIntervalAvgBytes;
   
   double  AvgPktsPerSec;
   double  AvgBytesPerSec;
   
   PKTMGR_StatsState_t State;
   
} PKTMGR_Stats_t;


typedef struct
{
   
   /*
   ** Framework Object References
   */

   INITBL_Class_t *IniTbl;
   
   /*
   ** Telemetry Packets
   */
   
   KIT_TO_PktTblTlm_t        PktTblTlm;
   KIT_TO_WrappedSbMsgTlm_t  TunnelTlm;

   /*
   ** PktMgr Data
   */

   CFE_SB_PipeId_t  TlmPipe;
   uint32           TlmUdpPort;
   osal_id_t        TlmSockId;
   char             TlmDestIp[PKTMGR_IP_STR_LEN];
   CFE_SB_MsgId_t   TunnelTlmMid;
   CFE_SB_MsgId_t   SbWrapToUdpTlmMid;
   
   bool                    DownlinkOn;
   bool                    SuppressSend;
   KIT_TO_TlmSource_Enum_t TlmSource;
   PKTMGR_Stats_t          Stats;

   /*
   ** Contained Objects
   */ 

   PKTTBL_Class_t    PktTbl;

} PKTMGR_Class_t;

/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: PKTMGR_Constructor
**
** Construct a PKTMGR object. All table entries are cleared and the LoadTbl()
** function should be used to load an initial table.
**
** Notes:
**   1. This must be called prior to any other function.
**   2. Decoupling the initial table load gives an app flexibility in file
**      management during startup.
**
*/
void PKTMGR_Constructor(PKTMGR_Class_t *PktMgrPtr, INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: PKTMGR_AddPktCmd
**
** Add a packet to the table and subscribe for it on the SB.
**
** Notes:
**   1. Command rejected if table has existing entry for the commanded msg ID
**   2. Only update the table if the software bus subscription successful
** 
*/
bool PKTMGR_AddPktCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: PKTMGR_EnableOutputCmd
**
** The commanded IP is always saved and downlink suppression is turned off. If
** downlink is disabled then a new socket is created with the new IP and
** downlink is turned on.
**
*/
bool PKTMGR_EnableOutputCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function:  PKTMGR_InitStats
**
** OutputTlmInterval - Number of ms between calls to PKTMGR_OutputTelemetry()
**                     If zero retain the last interval value
** InitDelay         - Number of ms to delay starting stats computation
*/
void PKTMGR_InitStats(uint16 OutputTlmInterval, uint16 InitDelay);


/******************************************************************************
** Function: PKTMGR_OutputTelemetry
**
** If downlink is enabled and output hasn't been suppressed it sends all of the
** SB packets on the telemetry input pipe out the socket.
**
*/
uint16 PKTMGR_OutputTelemetry(void);


/******************************************************************************
** Function: PKTMGR_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void PKTMGR_ResetStatus(void);


/******************************************************************************
** Function: PKTMGR_RemoveAllPktsCmd
**
** Notes:
**   1. The cFE to_lab code unsubscribes the command and send HK MIDs. I'm not
**      sure why this is done and I'm not sure how the command is used. This 
**      command is intended to help manage TO telemetry packets.
*/
bool PKTMGR_RemoveAllPktsCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: PKTMGR_RemovePktCmd
**
** Remove a packet from the table and unsubscribe from receiving it on the SB.
**
** Notes:
**   1. Don't consider trying to remove an non-existent entry an error
*/
bool PKTMGR_RemovePktCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: PKTMGR_SendPktTblTlmCmd
**
** Send a telemetry packet containg the packet table entry for the commanded
** msg ID.
**
*/
bool PKTMGR_SendPktTblTlmCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/*******************************************************************
** Function: PKTMGR_SetTlmSourceCmd
**
*/
bool PKTMGR_SetTlmSourceCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: PKTMGR_UpdatePktFilterCmd
**
** Notes:
**   1. Command rejected if AppId packet entry has not been loaded 
**   2. The filter type is verified but the filter parameter values are not 
** 
*/
bool PKTMGR_UpdatePktFilterCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _pktmgr_ */
