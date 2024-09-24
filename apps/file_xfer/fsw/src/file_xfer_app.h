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
**    Define the File Transfer application
**
**  Notes:
**    1. This app manages file transfers using the File Input Transfer Protocol
**       (FITP) to file input/creation and the File Output Transfer Protocol (FOTP)
**       to output files. FITP and FOTP are custom protocols (see fitp.h and fotp.h)
**       that can support a half duplex comm link.
**    2. Input and outut are relaive to the software bus and the file system used
**       by this app so the notion of a comm link is not embedded in the app.
**    3. Nominally only a single file in or out transfer would occur at one time,
**       but the app is written to allow FITP and FOTP to operate concurrently.
**
*/

#ifndef _file_xfer_app_
#define _file_xfer_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "fitp.h"
#include "fotp.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define FILE_XFER_INIT_EID        (FILE_XFER_APP_BASE_EID + 0)
#define FILE_XFER_INIT_ERR_EID    (FILE_XFER_APP_BASE_EID + 1)
#define FILE_XFER_NOOP_EID        (FILE_XFER_APP_BASE_EID + 2)
#define FILE_XFER_EXIT_EID        (FILE_XFER_APP_BASE_EID + 3)
#define FILE_XFER_INVALID_MID_EID (FILE_XFER_APP_BASE_EID + 4)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
** - See EDS command definitions in file_xfer.xml
*/


/******************************************************************************
** Telemetry Packets
*/

typedef struct
{

   uint8   FileTransferCnt;      /* Number of complete file transfers          */                
   uint8   FileTransferActive;   /* Boolean indicating if file transfer active */ 
   uint16  LastDataSegmentId;    /* ID of the last data segment saved to file  */
   uint16  DataSegmentErrCnt;    /* Count of data segments with errors         */                
   uint32  FileTransferByteCnt;  /* Number of file data bytes received/written */
   uint32  FileRunningCrc;       /* Running CRC of file data received          */
   char    DestFilename[FITP_FILENAME_LEN];
   
} FITP_HkData_t;

typedef struct
{

   uint8   FileTransferCnt;     /* Number of complete file transfers       */                
   uint8   FileTransferState;   /* See FOTP_FileTransferState_t definition */ 
   uint8   PausedTransferState; /* Identify which state was paused         */ 
   uint8   PrevSegmentFailed;   /* If true then FOTP attempts to resend    */ 
   
   uint32  FileTranferByteCnt;  /* Number of file data bytes sent          */
   uint32  FileRunningCrc;      /* Running CRC of file data sent           */
   
   uint32  DataTransferLen;   
   uint32  FileLen;
   uint32  FileByteOffset;      /* DataSegmentOffset*DataSegmentLen        */
   uint16  DataSegmentLen;      /* Length in start transfer command        */
   uint16  DataSegmentOffset;   /* Starting data segment                   */   
   uint16  NextDataSegmentId;
   
   char    SrcFilename[FOTP_FILENAME_LEN];
   
} FOTP_HkData_t;

typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;

   /*
   ** CMDMGR Data
   */
   
   uint16   ValidCmdCnt;
   uint16   InvalidCmdCnt;

   FITP_HkData_t Fitp;
   FOTP_HkData_t Fotp;
      
} FILE_XFER_HkPkt_t;
#define FILE_XFER_TLM_HK_LEN sizeof (FILE_XFER_HkPkt_t)

   
/******************************************************************************
** FILE_XFER Class
*/

typedef struct
{

   /* 
   ** App Framework
   */
   
   INITBL_Class_t   IniTbl;
   CFE_SB_PipeId_t  CmdPipe;
   CMDMGR_Class_t   CmdMgr;

   /*
   ** Telemetry Packets
   */
   
   FILE_XFER_HkPkt_t  HkPkt;

   /*
   ** FILE_XFER State & Contained Objects
   */

   uint32  PerfId;
  
   CFE_SB_MsgId_t   CmdMid;
   CFE_SB_MsgId_t   SendHkMid;
   CFE_SB_MsgId_t   ExecuteMid;
   
   FITP_Class_t Fitp;
   FOTP_Class_t Fotp;

} FILE_XFER_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern FILE_XFER_Class_t  FileXfer;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILE_XFER_AppMain
**
*/
void FILE_XFER_AppMain(void);


/******************************************************************************
** Function: FILE_XFER_NoOpCmd
**
*/
bool FILE_XFER_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_XFER_ResetAppCmd
**
*/
bool FILE_XFER_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _file_xfer_app_ */
