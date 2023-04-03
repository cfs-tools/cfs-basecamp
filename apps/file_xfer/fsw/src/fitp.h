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
**    Implement the "File Input Transfer Protocol" (FITP)
**
**  Notes:
**    1. The FITP is a custom algorithm that is similar to the CFDP Class 1
**       protocol and the TFTP algorithm without the acks for each data
**       segment. The need for this protocol was driven by half-duplex and 
**       highly imbalanced communication links.
**    2. Only one file tranfer can be active at a time and it is considered
**       an error if a new transfer is attempted when a trasnfer is already
**       in progress.
**    3. The file transfer is command driven with the file sender issuing
**       the following sequence:
**
**         A. Start File Transfer command
**         B. 1..N Data Segment commands
**         C. Finish File Transfer command
**
**    4. There are no timers associated with the protocol and a cancel file
**       transfer command can be sent at any time.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
*/


#ifndef _fitp_
#define _fitp_

/*
** Includes
*/

#include <string.h>

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define FITP_START_TRANSFER_CMD_EID       (FITP_BASE_EID +  0)
#define FITP_START_TRANSFER_CMD_ERR_EID   (FITP_BASE_EID +  1)

#define FITP_DATA_SEGMENT_CMD_EID         (FITP_BASE_EID +  2)
#define FITP_DATA_SEGMENT_CMD_ERR_EID     (FITP_BASE_EID +  3)

#define FITP_FINISH_TRANSFER_CMD_EID      (FITP_BASE_EID +  4)
#define FITP_FINISH_TRANSFER_CMD_ERR_EID  (FITP_BASE_EID +  5)

#define FITP_CANCEL_TRANSFER_CMD_EID      (FITP_BASE_EID +  6)
#define FITP_CANCEL_TRANSFER_CMD_ERR_EID  (FITP_BASE_EID +  7)


/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Command Packets
*/

/* TODO - Delete
typedef struct
{

   char    DestFilename[FITP_FILENAME_LEN];

} FITP_StartTransferCmdPayload_t;

typedef struct
{

   CFE_MSG_CommandHeader_t        CmdHeader;
   FITP_StartTransferCmdPayload_t Payload;

} FITP_StartTransferCmdMsg_t;
#define FITP_START_TRANSFER_CMD_DATA_LEN  (sizeof(FITP_StartTransferCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


typedef struct
{

   uint16  Id;
   uint16  Len;
   uint8   Data[FITP_DATA_SEG_MAX_LEN];

} FITP_DataSegmentCmdPayload_t;

typedef struct
{

   CFE_MSG_CommandHeader_t      CmdHeader;
   FITP_DataSegmentCmdPayload_t Payload;

} FITP_DataSegmentCmdMsg_t;
#define FITP_DATA_SEGMENT_CMD_DATA_LEN  (sizeof(FITP_DataSegmentCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


typedef struct
{

   uint32  FileLen;
   uint32  FileCrc;
   uint16  LastDataSegmentId;

} FITP_FinishTransferCmdPayload_t;

typedef struct
{

   CFE_MSG_CommandHeader_t         CmdHeader;
   FITP_FinishTransferCmdPayload_t Payload; 

} FITP_FinishTransferCmdMsg_t;
#define FITP_FINISH_TRANSFER_CMD_DATA_LEN  (sizeof(FITP_FinishTransferCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))

#define FITP_CANCEL_TRANSFER_CMD_DATA_LEN  PKTUTIL_NO_PARAM_CMD_DATA_LEN
*/

/******************************************************************************
** FITP Class
**
** - See command function implementations for details on the file transfer
**   state management
*/

typedef struct
{

   char      DestFilename[FITP_FILENAME_LEN];

   osal_id_t FileHandle;
   uint32    FileTransferByteCnt;
   uint32    FileRunningCrc;
   bool      FileTransferActive;
   uint16    FileTransferCnt;
   bool      BinFile;
   
   uint16    LastDataSegmentId;
   uint16    DataSegmentErrCnt;   

} FITP_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: FITP_Constructor
**
** Construct a FITP object.
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void FITP_Constructor(FITP_Class_t*  FitpPtr);


/******************************************************************************
** Function: FITP_CancelTransferCmd
**
** 
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
*/
bool FITP_CancelTransferCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FITP_DataSegmentCmd
**
** 
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
*/
bool FITP_DataSegmentCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FITP_FinishTransferCmd
**
** 
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
*/
bool FITP_FinishTransferCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function:  FITP_ResetStatus
**
*/
void FITP_ResetStatus(void);


/******************************************************************************
** Function: FITP_StartBinTransferCmd
**
** 
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
*/
bool FITP_StartBinTransferCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FITP_StartTransferCmd
**
** 
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
*/
bool FITP_StartTransferCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _fitp_ */
