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
**    Telemeter (playback) the contents of the current cFE event
**    message log.
**
**  Notes:
**    1. Initially motivated by a scenario where ground tools for dumping
**       and displaying the event log were not available.s
**
*/
#ifndef _evt_plbk_
#define _evt_plbk_

/*
** Includes
*/

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/
#define EVT_PLBK_LOG_READ_ERR_EID        (EVT_PLBK_BASE_EID +  0)
#define EVT_PLBK_SENT_WRITE_LOG_CMD_EID  (EVT_PLBK_BASE_EID +  1)
#define EVT_PLBK_STOP_CMD_EID            (EVT_PLBK_BASE_EID +  2)
#define EVT_PLBK_LOG_HDR_TYPE_ERR_EID    (EVT_PLBK_BASE_EID +  3)
#define EVT_PLBK_LOG_HDR_READ_ERR_EID    (EVT_PLBK_BASE_EID +  4)
#define EVT_PLBK_LOG_OPEN_ERR_EID        (EVT_PLBK_BASE_EID +  5)
#define EVT_PLBK_LOG_NONEXISTENT_EID     (EVT_PLBK_BASE_EID +  6)
#define EVT_PLBK_READ_LOG_SUCCESS_EID    (EVT_PLBK_BASE_EID +  7)
#define EVT_PLBK_CFG_CMD_ERR_EID         (EVT_PLBK_BASE_EID +  8)
#define EVT_PLBK_CFG_CMD_EID             (EVT_PLBK_BASE_EID +  9)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command & Telemetry Packets
**
** - Packets are defined in EDS definition file kit_to.xml
** - Separate start/stop commands are defined so command parameters are not
**   required. This simplifies soem remote ops configurations. 
*/

/**********TODO: Start Remove
typedef struct
{

   CFE_MSG_CommandHeader_t  CmdHeader;

   char    EvsLogFilename[CFE_MISSION_MAX_PATH_LEN];   // Filename to use when command write EVS log file
   uint16  HkCyclesPerPkt;                             // Number of HK request cycles between event log telemetry packets
   
} EVT_PLBK_ConfigCmdMsg_t;
#define EVT_PLBK_CONFIG_CMD_DATA_LEN  (sizeof(EVT_PLBK_ConfigCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))

typedef struct
{

   CFE_MSG_CommandHeader_t  CmdHeader;

} EVT_PLBK_NoParamCmdMsg_t;
#define EVT_PLBK_NO_PARAM_CMD_DATA_LEN  (sizeof(EVT_PLBK_NoParamCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))
#define EVT_PLBK_START_CMD_DATA_LEN (EVT_PLBK_NO_PARAM_CMD_DATA_LEN)
#define EVT_PLBK_STOP_CMD_DATA_LEN  (EVT_PLBK_NO_PARAM_CMD_DATA_LEN)

typedef struct 
{

   CFE_TIME_SysTime_t Time;
   char    AppName[CFE_MISSION_MAX_API_LEN];
   uint16  EventId;
   uint16  EventType;
   char    Message[CFE_MISSION_EVS_MAX_MESSAGE_LENGTH];

   
} EVT_PLBK_TlmEvent_t;

typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;

   char    EvsLogFilename[CFE_MISSION_MAX_PATH_LEN];
   uint16  EventCnt;
   uint16  PlbkIdx;
   
   EVT_PLBK_TlmEvent_t Event[EVT_PLBK_EVENTS_PER_TLM_MSG];

} EVT_PLBK_TlmMsg_t;

#define EVT_PLBK_TLM_MSG_LEN sizeof (EVT_PLBK_TlmMsg_t)

TODO: End Remove ************/


/******************************************************************************
** Event Playback Class
**
** Since KIT_TO is non-flight without memory constraints and the default event 
** log message count is 20, I took the simple approach to just read the entire
** log into memory. Using default configurations it's less than 3K bytes.
**
*/

typedef struct
{

   bool Loaded;
   KIT_TO_PlbkEvent_t Event;

} EVT_PLBK_EventLogEntry_t;

typedef struct
{

   uint16 EventCnt;    /* Number of entries loaded from log file. */
   uint16 PlbkIdx;     /* Currrent index used during playback */
   EVT_PLBK_EventLogEntry_t Entry[CFE_PLATFORM_EVS_LOG_MAX];

} EVT_PLBK_EventLog_t;

typedef struct
{

   /*
   ** Telemetry Packets
   */
   
   KIT_TO_PlbkEventTlm_t   Tlm;

   /*
   ** Event Playback Data
   */

   bool     Enabled;
   bool     LogFileCopied;
   uint16   EvsLogFileOpenAttempts;  /* Number of execution cycle attempts to open log file after write log commanded */  
   
   uint16   HkCyclePeriod;       /* Number of HK request cycles between event log telemetry packets */
   uint16   HkCycleCount;        /* Current count of HK cycles between telemetry packets sent */

   CFE_TIME_SysTime_t  StartTime;
   
   char EventLogFile[CFE_MISSION_MAX_PATH_LEN];
      
   EVT_PLBK_EventLog_t  EventLog;
   
} EVT_PLBK_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: EVT_PLBK_Constructor
**
** Construct an EVT_PLBK object. 
**
** Notes:
**   1. This must be called prior to any other function.
**   2. Disabled by default.
**
*/
void EVT_PLBK_Constructor(EVT_PLBK_Class_t *EvtPbPtr, INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: EVT_PLBK_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void EVT_PLBK_ResetStatus(void);


/******************************************************************************
** Function: EVT_PLBK_Execute
**
** If enabled create and send a new telmetry packet with the next set of 
** event messages.
**
** Notes:
**   1. It's assumed this function is called during the main apps HK request
**      execution cycle and the HkCyclePeriod determines how many HK cycles
**      should be between telemetry packet generations.
**   2. The current event log is captured (written to a file) when the start
**      playback command is received and this function continually loops
**      through the log file. 
**
*/
void EVT_PLBK_Execute(void);


/******************************************************************************
** Function: EVT_PLBK_ConfigCmd
**
** Configure the behavior of playbacks. See command parameters definitions
** for details.
**
*/
bool EVT_PLBK_ConfigCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: EVT_PLBK_StartCmd
**
** Command EVS to write the current event log to a file and start playing back
** the messages.
**
*/
bool EVT_PLBK_StartCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: EVT_PLBK_StopCmd
**
** Stop a playback if one is in progress.
**
*/
bool EVT_PLBK_StopCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _evt_plbk_ */
