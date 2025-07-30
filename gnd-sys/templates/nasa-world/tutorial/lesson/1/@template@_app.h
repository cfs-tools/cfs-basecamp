/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *
 * Main header file for the SAMPLE application
 */

#ifndef @TEMPLATE@_H
#define @TEMPLATE@_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

//EX1
/*
** There are no code changes in this exercise. The point of the exercise
** is to make you aware that @template@_msg.h has been commented out and
** replaced with the eds header files. 
**
*/
#include "@template@_perfids.h"
#include "@template@_msgids.h"
// Replaced by EDS: #include "@template@_msg.h"
#include "@template@_eds_cc.h"
#include "@template@_eds_typedefs.h"
//EX1

/***********************************************************************/
#define @TEMPLATE@_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define @TEMPLATE@_NUMBER_OF_TABLES 1 /* Number of Table(s) */

/* Define filenames of default data images for tables */
#define @TEMPLATE@_TABLE_FILE "/cf/@template@_tbl.tbl"

#define @TEMPLATE@_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define @TEMPLATE@_TBL_ELEMENT_1_MAX 10
/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{

    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    /*
    ** Housekeeping telemetry packet...
    */
    @TEMPLATE@_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;

    CFE_TBL_Handle_t TblHandles[@TEMPLATE@_NUMBER_OF_TABLES];
} @TEMPLATE@_Data_t;

/****************************************************************************/
//EX2
/*
** Local function prototypes.
**
** Note: Except for the entry point (@TEMPLATE@_Main), these
**       functions are not called from any other source module.
*/
void  @TEMPLATE@_AppMain(void);
int32 @TEMPLATE@_Init(void);
void  @TEMPLATE@_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void  @TEMPLATE@_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr);
int32 @TEMPLATE@_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg);
int32 @TEMPLATE@_Noop(const @TEMPLATE@_NoopCmd_t *Msg);
int32 @TEMPLATE@_ResetCounters(const @TEMPLATE@_ResetCountersCmd_t *Msg);
int32 @TEMPLATE@_ExampleParamCmd(const @TEMPLATE@_ExampleParamCmd_t *Msg);
void  @TEMPLATE@_GetCrc(const char *TableName);

int32 @TEMPLATE@_TblValidationFunc(void *TblData);

bool @TEMPLATE@_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);
//EX2

#endif /* @TEMPLATE@_H */
