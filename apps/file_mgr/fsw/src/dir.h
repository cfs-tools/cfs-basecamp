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
**    Define methods for managing directories
**
**  Notes:
**    1. Command and telemetry packets are defined in EDS file filemgr.xml.
**
*/

#ifndef _dir_
#define _dir_

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

#define DIR_CREATE_EID               (DIR_BASE_EID +  0)
#define DIR_CREATE_ERR_EID           (DIR_BASE_EID +  1)
#define DIR_DELETE_ALL_EID           (DIR_BASE_EID +  2)
#define DIR_DELETE_ALL_ERR_EID       (DIR_BASE_EID +  3)
#define DIR_DELETE_ALL_WARN_EID      (DIR_BASE_EID +  4)
#define DIR_DELETE_EID               (DIR_BASE_EID +  5)
#define DIR_DELETE_ERR_EID           (DIR_BASE_EID +  6)
#define DIR_SEND_LIST_PKT_EID        (DIR_BASE_EID +  7)
#define DIR_SEND_LIST_PKT_ERR_EID    (DIR_BASE_EID +  8)
#define DIR_SEND_LIST_PKT_WARN_EID   (DIR_BASE_EID +  9)
#define DIR_WRITE_LIST_FILE_EID      (DIR_BASE_EID + 10)
#define DIR_WRITE_LIST_FILE_ERR_EID  (DIR_BASE_EID + 11)
#define DIR_WRITE_LIST_FILE_WARN_EID (DIR_BASE_EID + 12)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
** - See EDS command definitions in file_mgr.xml
*/


/******************************************************************************
** Telemetry Packets
** - See EDS telemetry definitions in file_mgr.xml
*/


/******************************************************************************
** File Structures
*/

typedef struct
{

   char    Name[OS_MAX_PATH_LEN];
   uint32  Size;
   uint32  Time;     /* File's Last Modification Times */
   uint32  Mode;     /* Mode of the file (Permissions) */

} DIR_FileEntry_t;

typedef struct
{

   char    DirName[OS_MAX_PATH_LEN];
   uint32  DirFileCnt;                 /* Number of files in the directory */
   uint32  FilesWrittenCnt;            /* Number of entries written to file  */

} DIR_ListFilesStats_t;


/******************************************************************************
** DIR_Class
*/

typedef struct
{

   /*
   ** App Framework References
   */
   
   const INITBL_Class_t*  IniTbl;

   /*
   ** Telemetry
   */
   
   FILE_MGR_DirListTlm_t  ListTlm;
   
   /*
   ** Files
   */

   DIR_ListFilesStats_t  ListFileStats;
   
   /*
   ** FileMgr State Data
   */

   uint32  TaskFileStatCnt;
   uint32  TaskFileStatDelay;
   uint32  ChildTaskPerfId;

   uint16  CmdWarningCnt;
   

} DIR_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: DIR_Constructor
**
** Initialize the DIR to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void DIR_Constructor(DIR_Class_t *DirPtr, const INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: DIR_CreateCmd
**
** Create a new directory. 
*/
bool DIR_CreateCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_DeleteAllCmd
**
*/
bool DIR_DeleteAllCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_DeleteCmd
**
** Delete an existing empty directory.
*/
bool DIR_DeleteCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void DIR_ResetStatus(void);


/******************************************************************************
** Function: DIR_SendDirListTlmCmd
**
*/
bool DIR_SendDirListTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_SendDirTlmCmd
**
*/
bool DIR_SendDirTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: DIR_WriteListFileCmd
**
** Notes:
**   1. Target file will be overwritten if it exists an is closed.
*/
bool DIR_WriteListFileCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _dir_ */
