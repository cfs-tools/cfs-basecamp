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
**    Define the File Manager application
**
**  Notes:
**    1. This is a refactor of NASA's File Manager (FM) app. The refactor includes
**       adaptation to the OSK app framework and prootyping the usage of an app 
**       init JSON file. The idea is to rethink whcih configuration paarameters
**       should be compile time and which should be runtime.
**    2. Command and telemetry packets are defined in EDS file file_mgr.xml.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef _file_mgr_app_
#define _file_mgr_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "dir.h"
#include "file.h"
#include "filesys.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define FILE_MGR_INIT_APP_EID    (FILE_MGR_BASE_EID + 0)
#define FILE_MGR_NOOP_EID        (FILE_MGR_BASE_EID + 1)
#define FILE_MGR_EXIT_EID        (FILE_MGR_BASE_EID + 2)
#define FILE_MGR_INVALID_MID_EID (FILE_MGR_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
*/


/******************************************************************************
** Telemetry Packets
*/


//EDS typedef struct
//EDS {
//EDS 
//EDS    CFE_MSG_TelemetryHeader_t TlmHeader;
//EDS 
//EDS    uint8   CommandCounter;
//EDS    uint8   CommandErrCounter;
//EDS    uint8   Spare;                          
//EDS    uint8   NumOpenFiles;                   /* Number of open files in the system */
//EDS    uint8   ChildCmdCounter;                /* Child task command counter */
//EDS    uint8   ChildCmdErrCounter;             /* Child task command error counter */
//EDS    uint8   ChildCmdWarnCounter;            /* Child task command warning counter */
//EDS    uint8   ChildQueueCount;                /* Number of pending commands in queue */
//EDS    uint8   ChildCurrentCC;                 /* Command code currently executing */
//EDS    uint8   ChildPreviousCC;                /* Command code previously executed */
//EDS 
//EDS } FILE_MGR_HkPkt_t;
//EDS #define FILE_MGR_TLM_HK_LEN sizeof (FILE_MGR_HkPkt_t)


/******************************************************************************
** FILE_MGR_Class
*/
typedef struct
{

   /* 
   ** App Framework
   */ 
   
   INITBL_Class_t   IniTbl; 
   CFE_SB_PipeId_t  CmdPipe;
   CMDMGR_Class_t   CmdMgr;
   TBLMGR_Class_t   TblMgr;
   
   CHILDMGR_Class_t ChildMgr;
   
   FileUtil_OpenFileList_t OpenFileList;
   
   /*
   ** Telemetry Packets
   */
   
   FILE_MGR_HkTlm_t HkPkt;
   
   /*
   ** FILE_MGR State & Contained Objects
   */
          
   uint32           PerfId;
   CFE_SB_MsgId_t   CmdMid;
   CFE_SB_MsgId_t   SendHkMid;
   
   DIR_Class_t      Dir;
   FILE_Class_t     File;
   FILESYS_Class_t  FileSys;
 
} FILE_MGR_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern FILE_MGR_Class_t  FileMgr;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILE_MGR_AppMain
**
*/
void FILE_MGR_AppMain(void);


/******************************************************************************
** Function: FILE_MGR_NoOpCmd
**
*/
bool FILE_MGR_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_MGR_ResetAppCmd
**
*/
bool FILE_MGR_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _file_mgr_app_ */
