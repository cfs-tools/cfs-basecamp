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
**    Manage logging time-stamped data sample values for a single
**    histogram bin.
**
**  Notes:
**    1. This is for demonstration purposes and is part of the App
**       code-as-you-go (CAYG) tutorial.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

#ifndef _histogram_log_
#define _histogram_log_

/*
** Includes
*/

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* 
** Even number of bytes since hex char array used in tlm pkt definition
*/
#define HISTOGRAM_LOG_TEXT_LEN 36

/*
** Event Message IDs
*/

#define HISTOGRAM_LOG_RUN_EID               (HISTOGRAM_LOG_BASE_EID + 0)
#define HISTOGRAM_LOG_START_LOG_CMD_EID     (HISTOGRAM_LOG_BASE_EID + 1)
#define HISTOGRAM_LOG_RUN_LOG_EID           (HISTOGRAM_LOG_BASE_EID + 2)
#define HISTOGRAM_LOG_STOP_LOG_CMD_EID      (HISTOGRAM_LOG_BASE_EID + 3)
#define HISTOGRAM_LOG_START_PLAYBK_CMD_EID  (HISTOGRAM_LOG_BASE_EID + 4)
#define HISTOGRAM_LOG_RUN_PLAYBK_EID        (HISTOGRAM_LOG_BASE_EID + 5)
#define HISTOGRAM_LOG_STOP_PLAYBK_CMD_EID   (HISTOGRAM_LOG_BASE_EID + 6)


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
** HISTOGRAM_LOG_Class
*/

typedef struct
{

   /*
   ** App Framework References
   */
   
   INITBL_Class_t *IniTbl;

   /*
   ** Telemetry Packets
   */
   
   APP_C_DEMO_BinPlaybkTlm_t  BinPlaybkTlm;

   /*
   ** Class State Data
   */

   bool      Ena;
   uint16    BinNum;
   uint16    Cnt;
   uint16    MaxEntries;
      
   bool      PlaybkEna;
   uint16    PlaybkCnt;
   
   const char *FilePrefix;
   const char *FileExtension;   
   osal_id_t  FileHandle;
   char       Filename[OS_MAX_PATH_LEN];
    
} HISTOGRAM_LOG_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: HISTOGRAM_LOG_Constructor
**
** Initialize the histogram log to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void HISTOGRAM_LOG_Constructor(HISTOGRAM_LOG_Class_t *HistogramLogPtr,
                               INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: HISTOGRAM_LOG_AddDataSample
**
** Notes:
**   None
**
*/
bool HISTOGRAM_LOG_AddDataSample(uint16 Bin, uint16 DataSample);


/******************************************************************************
** Function: HISTOGRAM_LOG_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void HISTOGRAM_LOG_ResetStatus(void);


/******************************************************************************
** Function: HISTOGRAM_LOG_RunChildTaskCmd
**
** Run the child task function that manages the histogram bin logging
**
** Notes:
**   None
**
*/
bool HISTOGRAM_LOG_RunChildTaskCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: HISTOGRAM_LOG_StartLogCmd
**
*/
bool HISTOGRAM_LOG_StartLogCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: HISTOGRAM_LOG_StartPlaybkCmd
**
*/
bool HISTOGRAM_LOG_StartPlaybkCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: HISTOGRAM_LOG_StopLogCmd
**
*/
bool HISTOGRAM_LOG_StopLogCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: HISTOGRAM_LOG_StopPlaybkCmd
**
*/
bool HISTOGRAM_LOG_StopPlaybkCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _histogram_log_ */
