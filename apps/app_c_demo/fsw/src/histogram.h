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
**    Create a histogram of integer data 
**
**  Notes:
**    1. This is for demonstration purposes and is part of the App
**       code-as-you-go (CAYG) tutorial.
**
*/

#ifndef _histogram_
#define _histogram_

/*
** Includes
*/

#include "app_cfg.h"
#include "histogram_log.h"
#include "histogram_tbl.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define HISTOGRAM_START_CMD_EID      (HISTOGRAM_BASE_EID + 0)
#define HISTOGRAM_STOP_CMD_EID       (HISTOGRAM_BASE_EID + 1)
#define HISTOGRAM_DATA_SAMPLE_EID    (HISTOGRAM_BASE_EID + 2)
#define HISTOGRAM_ACCEPT_NEW_TBL_EID (HISTOGRAM_BASE_EID + 3)

/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Command Packets
** - See EDS command definitions in app_c_demo.xml
*/


/******************************************************************************
** HISTOGRAM_Class
*/


typedef struct
{

   /*
   ** App Framework References
   */
   
   
   /*
   ** Class State Data
   */

   bool    Ena;
   uint32  SampleCnt;
   uint16  DataSampleMaxValue;
   
   uint16  Bin[HISTOGRAM_MAX_BINS];
   char    BinCntStr[OS_MAX_PATH_LEN];    //TODO use EDS definition
       
   /*
   ** Contained Objects
   */

   HISTOGRAM_LOG_Class_t  Log;
   HISTOGRAM_TBL_Class_t  Tbl;
   
} HISTOGRAM_Class_t;



/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: HISTOGRAM_Constructor
**
** Initialize the packet log to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void HISTOGRAM_Constructor(HISTOGRAM_Class_t *HistogramPtr, 
                           const INITBL_Class_t *IniTbl,
                           TBLMGR_Class_t *TblMgr);


/******************************************************************************
** Function: HISTOGRAM_AddDataSample
**
** Notes:
**   1. If data sample succesfully recorded (i.e. return true) then BinNum
**      is loaded with the bin number that the sample was recorded
**
*/
bool HISTOGRAM_AddDataSample(uint16 DataSample, uint16 *BinNum);


/******************************************************************************
** Function: HISTOGRAM_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void HISTOGRAM_ResetStatus(void);


/******************************************************************************
** Function: HISTOGRAM_StartCmd
**
** Notes:
**   1. No command parameters
**   2. If a start command is recieved when a histogram is in progress the
**      command will serve as a reset. It is not considered an error.
*/
bool HISTOGRAM_StartCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: HISTOGRAM_StopCmd
**
** Notes:
**   1. No command parameters
**   2. Receiving a stop command when the histogram is not actve is not 
**      considered an error. A stop command could be issued as part of an 
**      onboard command sequence.  
*/
bool HISTOGRAM_StopCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _histogram_ */
