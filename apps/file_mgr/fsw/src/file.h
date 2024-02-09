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
**    Define methods for managing files
**
**  Notes:
**    1. Command and telemetry packets are defined in EDS file filemgr.xml.
**
*/

#ifndef _file_
#define _file_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define FILE_IGNORE_CRC  0


/*
** Event Message IDs
*/

#define FILE_CONCATENATE_EID          (FILE_BASE_EID +  0)
#define FILE_CONCATENATE_ERR_EID      (FILE_BASE_EID +  1)
#define FILE_COPY_EID                 (FILE_BASE_EID +  2)
#define FILE_COPY_ERR_EID             (FILE_BASE_EID +  3)
#define FILE_DECOMPRESS_EID           (FILE_BASE_EID +  4)
#define FILE_DECOMPRESS_ERR_EID       (FILE_BASE_EID +  5)
#define FILE_DELETE_EID               (FILE_BASE_EID +  6)
#define FILE_DELETE_ERR_EID           (FILE_BASE_EID +  7)
#define FILE_MOVE_EID                 (FILE_BASE_EID +  8)
#define FILE_MOVE_ERR_EID             (FILE_BASE_EID +  9)
#define FILE_RENAME_EID               (FILE_BASE_EID + 10)
#define FILE_RENAME_ERR_EID           (FILE_BASE_EID + 11)
#define FILE_SEND_INFO_EID            (FILE_BASE_EID + 12)
#define FILE_SEND_INFO_ERR_EID        (FILE_BASE_EID + 13)
#define FILE_SET_PERMISSIONS_EID      (FILE_BASE_EID + 14)
#define FILE_SET_PERMISSIONS_ERR_EID  (FILE_BASE_EID + 15)
#define FILE_COMPUTE_FILE_CRC_ERR_EID (FILE_BASE_EID + 16)

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
** FILE_Class
*/

typedef struct
{

   /*
   ** App Framework References
   */
   
   const INITBL_Class_t*  IniTbl;

   /*
   ** Telemetry Packets
   */
   
   FILE_MGR_FileInfoTlm_t  InfoTlm;

   /*
   ** File State Data
   */

   uint16  CmdWarningCnt;

   char FileTaskBuf[FILE_MGR_TASK_FILE_BLOCK_SIZE];
   
} FILE_Class_t;


/******************************************************************************
** File Structure
*/


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FILE_Constructor
**
** Initialize the example object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void FILE_Constructor(FILE_Class_t *FilePtr, const INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: FILE_ConcatenateCmd
**
*/
bool FILE_ConcatenateCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_CopyCmd
**
*/
bool FILE_CopyCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_DecompressCmd
**
*/
bool FILE_DecompressCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_DeleteCmd
**
*/
bool FILE_DeleteCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_MoveCmd
**
*/
bool FILE_MoveCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_RenameCmd
**
*/
bool FILE_RenameCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void FILE_ResetStatus(void);


/******************************************************************************
** Function: FILE_SendInfoTlmCmd
**
** Notes:
**   1. If the file exists then a telemetry packet will be sent regardless of
**      whether a CRC was request but could not be computed due to the file
**      being open
**
*/
bool FILE_SendInfoTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: FILE_SetPermissionsCmd
**
*/
bool FILE_SetPermissionsCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _file_ */
