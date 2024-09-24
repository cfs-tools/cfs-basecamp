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
**    Provide a table for identifying onboard file systems and for managing
**    commands that obtain information about the file systems
**
**  Notes:
**    1. Refactored from NASA's FM FreeSpace table. I renamed to FileSys 
**       because "free space" is an attrbute of a file system volume.
**    2. The original design doesn't have concepts such as File and Dir
**       objects but it did separate table from non-table functions. This
**       design includes file system functions like "SendOpenFilesTlm"
**       because it is not operating on a File object. 
**    3. Use the Singleton design pattern. A pointer to the table object
**       is passed to the constructor and saved for all other operations.
**       Note the cFE's buffers are used to store the actual data itself.
**       This is a table-specific file so it doesn't need to be re-entrant.
**    4. Command and telemetry packets are defined in EDS file filemgr.xml.
**
*/

#ifndef _filesys_
#define _filesys_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/


#define FILESYS_TBL_ENTRY_DISABLED     0
#define FILESYS_TBL_ENTRY_ENABLED      1
#define FILESYS_TBL_ENTRY_UNUSED       2


#define FILESYS_TBL_REGISTER_ERR_EID         (FILESYS_BASE_EID + 0)
#define FILESYS_TBL_VERIFY_ERR_EID           (FILESYS_BASE_EID + 1)
#define FILESYS_TBL_VERIFIED_EID             (FILESYS_BASE_EID + 2)
#define FILESYS_SEND_TLM_ERR_EID             (FILESYS_BASE_EID + 3)
#define FILESYS_SEND_TLM_CMD_EID             (FILESYS_BASE_EID + 4)
#define FILESYS_SET_TBL_STATE_LOAD_ERR_EID   (FILESYS_BASE_EID + 5)
#define FILESYS_SET_TBL_STATE_ARG_ERR_EID    (FILESYS_BASE_EID + 5)
#define FILESYS_SET_TBL_STATE_UNUSED_ERR_EID (FILESYS_BASE_EID + 6)
#define FILESYS_SET_TBL_STATE_CMD_EID        (FILESYS_BASE_EID + 7)
#define FILESYS_SEND_OPEN_FILES_CMD_EID      (FILESYS_BASE_EID + 8)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Table Struture
*/

typedef struct
{
   
   uint32  State; 
   char    Name[OS_MAX_PATH_LEN];

} FILESYS_Volume_t;


typedef struct{
   
   FILESYS_Volume_t Volume[FILE_MGR_FILESYS_TBL_VOL_CNT];

} FILESYS_TblData_t;


/******************************************************************************
** Class Struture
*/

typedef struct
{

   bool                Registered;
   int32               Status;        /* Status of last cFE Table service call */
   CFE_TBL_Handle_t    Handle;
   FILESYS_TblData_t   *DataPtr;

} FILESYS_CfeTbl_t;

typedef struct
{

   /*
   ** App Framework
   */
   
   const INITBL_Class_t *IniTbl;

   /*
   ** Tables
   */
   
   FILESYS_CfeTbl_t  CfeTbl;

   /*
   ** Telemetry
   */
   
   FILE_MGR_FileSysTblTlm_t  TblTlm;
   FILE_MGR_OpenFileTlm_t    OpenFileTlm;

   /*
   ** Class State Data
   */

   const char *CfeTblName;
   FileUtil_OpenFileList_t OpenFileList;
   

} FILESYS_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILESYS_Constructor
**
** Initialize the example cFE Table object.
**
** Notes:
**   None
*/
void FILESYS_Constructor(FILESYS_Class_t *FileSysPtr, const INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: FILESYS_ManageTbl
**
** Manage the cFE table interface for table loads and validation. 
*/
void FILESYS_ManageTbl(void);


/******************************************************************************
** Function: FILESYS_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void FILESYS_ResetStatus(void);


/******************************************************************************
** Function: FILESYS_SendOpenFileTlmCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SendOpenFileTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILESYS_SetTblStateCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SetTblStateCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILESYS_SendTblTlmCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SendTblTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _filesys_ */
