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
**    Provide file system services for FILE_MGR
**
*/

/*
** Include Files:
*/

#include <string.h>

#include "app_cfg.h"
#include "filesys.h"
#include "initbl.h"


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static bool FileSysTblLoadCallback(FILESYS_TBL_Data_t *TblPtr);
static CFE_Status_t GetDirUsedSpace(const char *Directory, uint64 *BlockCount, uint64 *ByteCount);
static CFE_Status_t GetVolFreeSpace(const char *FileSys, uint64 *BlockCount, uint64 *ByteCount);

/**********************/
/** File Global Data **/
/**********************/

static FILESYS_Class_t  *FileSys = NULL;


/******************************************************************************
** Function: FILESYS_Constructor
**
*/
void FILESYS_Constructor(FILESYS_Class_t *FileSysPtr, const INITBL_Class_t *IniTbl,
                         TBLMGR_Class_t  *TblMgr)
{
 
   FileSys = FileSysPtr;

   CFE_PSP_MemSet((void*)FileSys, 0, sizeof(FILESYS_Class_t));
   
   FileSys->IniTbl  = IniTbl;
   FileSys->TblName = INITBL_GetStrConfig(FileSys->IniTbl, CFG_FILESYS_TBL_NAME);

   FILESYS_TBL_Constructor(&FileSys->Tbl, FileSysTblLoadCallback);

   TBLMGR_RegisterTblWithDef(TblMgr, FileSys->TblName,
                             FILESYS_TBL_LoadCmd, FILESYS_TBL_DumpCmd,  
                             INITBL_GetStrConfig(IniTbl, CFG_FILESYS_TBL_LOAD_FILE));
                             
   CFE_MSG_Init(CFE_MSG_PTR(FileSys->EstSpaceTlm.TelemetryHeader), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(FileSys->IniTbl, CFG_FILE_MGR_FILE_SYS_EST_SPACE_TLM_TOPICID)),
                sizeof(FILE_MGR_FileSysEstSpaceTlm_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(FileSys->OpenFileTlm.TelemetryHeader),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(FileSys->IniTbl, CFG_FILE_MGR_OPEN_FILE_TLM_TOPICID)),
                sizeof(FILE_MGR_OpenFileTlm_t));

} /* End FILESYS_Constructor() */


/******************************************************************************
** Function:  FILESYSTBL_ResetStatus
**
*/
void FILESYS_ResetStatus(void)
{

   FILESYS_TBL_ResetStatus();

} /* End FILESYSTBL_ResetStatus() */


/******************************************************************************
** Function: FILESYS_SendOpenFileTlmCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SendOpenFileTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   uint16 i;

   /* Don't assume utility will null char strings */
   memset(&FileSys->OpenFileTlm.Payload,0,sizeof(FILE_MGR_OpenFileTlm_Payload_t));
   
   FileUtil_GetOpenFileList(&FileSys->OpenFileList);
   FileSys->OpenFileTlm.Payload.File.Count = FileSys->OpenFileList.Count;
   for (i=0; i < EdsParam_FILE_MGR_OS_MAX_NUM_OPEN_FILES; i++)
   {
       strncpy(FileSys->OpenFileTlm.Payload.File.Entry[i].AppName,  FileSys->OpenFileList.Entry[i].AppName,  OS_MAX_API_NAME);
       strncpy(FileSys->OpenFileTlm.Payload.File.Entry[i].Filename, FileSys->OpenFileList.Entry[i].Filename, OS_MAX_PATH_LEN);
   }

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(FileSys->OpenFileTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(FileSys->OpenFileTlm.TelemetryHeader), true);
   
   CFE_EVS_SendEvent(FILESYS_SEND_OPEN_FILES_CMD_EID, CFE_EVS_EventType_DEBUG,
                     "Sent open files telemetry packets with %d file reported as open", 
                     FileSys->OpenFileTlm.Payload.File.Count);
   
   return true;
   
} /* End FILESYS_SendOpenFileTlmCmd() */


/******************************************************************************
** Function: FILESYS_SendEstSpaceTlmCmd
**
** Note:
**  1. FileSys table determines what will be reported in telemetry.
**  2. A false return status indicates there were errors retrieving the
**     file system data. The telemetry message is always sent.
*/
bool FILESYS_SendEstSpaceTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool  RetStatus = true;
   CFE_Status_t GetSpaceStatus;

   const FILE_MGR_FileSysTblEntry_t *FileSysTbl;
   FILE_MGR_FileSysEstSpaceEntry_t  *FileSysEstSpaceTlm;

   if (FileSys->Tbl.Json.Loaded)
   {
      memset(&FileSys->EstSpaceTlm.Payload, 0, sizeof(FileSys->EstSpaceTlm.Payload));
      
      for (uint8 i=0; i < EdsParam_FILE_MGR_FILESYS_TBL_ENTRY_CNT; i++)
      {
         FileSysTbl         = &FileSys->Tbl.Data.Entry[i];
         FileSysEstSpaceTlm = &FileSys->EstSpaceTlm.Payload.Entry[i];

         FileSysEstSpaceTlm->State = FileSysTbl->State;

         if (FileSysTbl->State == APP_C_FW_ConfigEnaTriState_ENABLED)
         {
            
            FileSysEstSpaceTlm->Type = FileSysTbl->Type;
            CFE_SB_MessageStringSet(FileSysEstSpaceTlm->Name, FileSysTbl->Name,
                                    sizeof(FileSysEstSpaceTlm->Name), sizeof(FileSysTbl->Name));
            
            // Get<VolFree|DirUsed>Space() send error events 
            if (FileSysTbl->Type == FILE_MGR_FileSysTblEntryType_VOLUME_FREE_SPACE)
            {
               GetSpaceStatus = GetVolFreeSpace(FileSysTbl->Name, &FileSysEstSpaceTlm->Blocks, &FileSysEstSpaceTlm->Bytes);
            }
            else if (FileSysTbl->Type == FILE_MGR_FileSysTblEntryType_DIRECTORY_ESTIMATE)
            {
               GetSpaceStatus = GetDirUsedSpace(FileSysTbl->Name, &FileSysEstSpaceTlm->Blocks, &FileSysEstSpaceTlm->Bytes);
            }
            if (GetSpaceStatus != CFE_SUCCESS)
            {
               RetStatus = false;
            }
         } /* End if enabled */
         else if (FileSysTbl->State == APP_C_FW_ConfigEnaTriState_DISABLED)
         {
            FileSysEstSpaceTlm->Type = FileSysTbl->Type;
            CFE_SB_MessageStringSet(FileSysEstSpaceTlm->Name, FileSysTbl->Name,
                                    sizeof(FileSysEstSpaceTlm->Name), sizeof(FileSysTbl->Name));
         }
      } /* End entry loop */

      CFE_SB_TimeStampMsg(CFE_MSG_PTR(FileSys->EstSpaceTlm.TelemetryHeader));
      CFE_SB_TransmitMsg(CFE_MSG_PTR(FileSys->EstSpaceTlm.TelemetryHeader), true);
      
      CFE_EVS_SendEvent(FILESYS_SEND_TLM_CMD_EID, CFE_EVS_EventType_DEBUG,
                       "Sent file system estimated space telemetry message using %s table entries",
                       FileSys->TblName);
   
   } /* End if table loaded */
   else
   {
      
      CFE_EVS_SendEvent(FILESYS_SEND_TLM_CMD_EID, CFE_EVS_EventType_ERROR,
                        "Send FileSys table telemetry error: %s has never been loaded",
                        FileSys->TblName);
                        
   } /* End if table not loaded */

   return RetStatus;

} /* End FILESYS_SendEstSpaceTlmCmd() */


/******************************************************************************
** Function: FILESYS_SetTblStateCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SetTblStateCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_SetFileSysTblState_CmdPayload_t *SetFileSysTblStateCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_SetFileSysTblState_t);   
   bool   RetStatus = false;
   uint16 CmdEntryIndex = SetFileSysTblStateCmd->TblEntryIndex;
   uint16 CmdEntryState = SetFileSysTblStateCmd->TblEntryState;

   if (FileSys->Tbl.Json.Loaded)
   {
      if (CmdEntryIndex >= EdsParam_FILE_MGR_FILESYS_TBL_ENTRY_CNT)
      {
         
         CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_CMD_EID, CFE_EVS_EventType_ERROR,
            "Set %s Table State Command Error: Commanded index %d is not in valid range of 0..%d",
            FileSys->TblName, CmdEntryIndex, (EdsParam_FILE_MGR_FILESYS_TBL_ENTRY_CNT-1));
           
      }
      else if ((CmdEntryState != APP_C_FW_ConfigEnaTriState_ENABLED) &&
               (CmdEntryState != APP_C_FW_ConfigEnaTriState_DISABLED))
      {
                  
         CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_CMD_EID, CFE_EVS_EventType_ERROR,
            "Set %s Table State Command Error: Commanded state %d is not in (%d=Enabled, %d=Disabled)",
            FileSys->TblName, CmdEntryState,
            APP_C_FW_ConfigEnaTriState_DISABLED, APP_C_FW_ConfigEnaTriState_ENABLED);

      }
      else if (FileSys->Tbl.Data.Entry[CmdEntryIndex].State == APP_C_FW_ConfigEnaTriState_UNUSED)
      {
         
         CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_CMD_EID, CFE_EVS_EventType_ERROR,
            "Set %s Table State Command Error: Attempt to change state of unused table entry at index %d",
            FileSys->TblName, CmdEntryIndex);
           
      }
      else
      {
         if (CmdEntryState == APP_C_FW_ConfigEnaTriState_ENABLED &&
             !FileSys->Tbl.Data.Entry[CmdEntryIndex].Valid)
         {
            CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_CMD_EID, CFE_EVS_EventType_INFORMATION,
               "Rejected Set %s Table State Command: Attempted to enabled entry %d that was flagged invalid during the table load",
               FileSys->TblName, CmdEntryIndex);            
         }
         else
         {
            FileSys->Tbl.Data.Entry[CmdEntryIndex].State = CmdEntryState;

            CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_CMD_EID, CFE_EVS_EventType_INFORMATION,
               "Set %s Table State Command: Set table index %d state to %s",
               FileSys->TblName, CmdEntryIndex, PktUtil_ConfigTriStateStr(CmdEntryState));
         }
      } 
      
   } /* End if table loaded */
   else
   {
      
      CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_CMD_EID, CFE_EVS_EventType_ERROR,
                  "Set file system table state error: %s has never been loaded",
                  FileSys->TblName);
                  
   } /* End if table not loaded */


   return RetStatus;

} /* End of FILESYS_SetTableStateCmd() */


/******************************************************************************
** Function: FileSysTblLoadCallback
**
** Callback function used by App Framework's TBL_MGR.
**
** Note:
**  1. Function signature must match FILESYS_TBL_LoadFunc_t
**  2. The purpose of this function is to perform processing that needs 
**     to be done when the FileSys table is loaded. It may also include
**     validation that is performed within the FileSys context that is 
**     broader than the table-specific validation context.   
**
*/
static bool FileSysTblLoadCallback(FILESYS_TBL_Data_t *TblPtr) 
{
   
   return true;
  
} /* End FileSysTblLoadCallback() */


/******************************************************************************
** Function: GetDirUsedSpace
**
*/
static CFE_Status_t GetDirUsedSpace(const char *Directory, uint64 *BlockCount, 
                                    uint64 *ByteCount)
{
   osal_id_t     DirId;
   os_dirent_t   DirEntry;
   os_fstat_t    FileStat;
   osal_status_t OsStatus;
   CFE_Status_t  RetStatus;
   char          FullPath[CFE_MISSION_MAX_PATH_LEN];
   uint64        TotalBytes;
   size_t        DirLen;

   TotalBytes = 0;

   memset(&DirEntry, 0, sizeof(DirEntry));
   snprintf(FullPath, sizeof(FullPath), "%s", Directory);
   DirLen = OS_strnlen(FullPath, CFE_MISSION_MAX_PATH_LEN);
   
   if (DirLen < (sizeof(FullPath) - 2))
   {
      
      FullPath[DirLen] = '/';
      DirLen++;
      FullPath[DirLen] = 0;

      /* Open directory so that we can read from it */
      OsStatus = OS_DirectoryOpen(&DirId, Directory);
    
   }
   else
   {
      OsStatus = OS_ERR_NAME_TOO_LONG;
   }

   if (OsStatus == OS_SUCCESS)
   {
   
      /* Read each directory entry and stat the files */
      while (OS_DirectoryRead(DirId, &DirEntry) == OS_SUCCESS)
      {
           
         snprintf(&FullPath[DirLen], sizeof(FullPath) - DirLen, "%s", OS_DIRENTRY_NAME(DirEntry));
         
         CFE_EVS_SendEvent(FILESYS_GET_DIR_USED_SPACE_EID, CFE_EVS_EventType_DEBUG,
                           "FullPath[DirLen]: %s", FullPath);
         
         if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
             (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR)  != 0))
         {
            OsStatus = OS_stat(FullPath, &FileStat);
            if (OsStatus != OS_SUCCESS)
            {
               CFE_EVS_SendEvent(FILESYS_GET_DIR_USED_SPACE_EID, CFE_EVS_EventType_ERROR,
                                 "Err computing used space. OS_stat()=%d,path=%s",
                                 (int)OsStatus, FullPath);
            }
            else if (!OS_FILESTAT_ISDIR(FileStat))
            {
               /*
               ** Only need to accumulate regular file entries, not dirs. Note OSAL
               ** does not currently report the number of blocks, only the number of
               ** bytes
               */
               TotalBytes += FileStat.FileSize;
            }
         }            
      } /* End dir while loop */

      OS_DirectoryClose(DirId);

      *ByteCount = TotalBytes;
      RetStatus  = CFE_SUCCESS;
   
   } /* End if dir opened */
   else
   {
      CFE_EVS_SendEvent(FILESYS_GET_DIR_USED_SPACE_EID, CFE_EVS_EventType_ERROR, 
                        "Error computing used space for directory %s. OS_DirectoryOpen() err=%d, path=%s",
                        Directory, (int)OsStatus, Directory);

      RetStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;

   } /* End if dir open failed */

   return RetStatus;
    
} /* End GetDirUsedSpace() */


/******************************************************************************
** Function: GetVolFreeSpace
**
*/
static CFE_Status_t GetVolFreeSpace(const char *FileSys, uint64 *BlockCount, uint64 *ByteCount)
{
   
   OS_statvfs_t   FileStats = {0};
   osal_status_t  OsStatus;
   CFE_Status_t   RetStatus;

   // Get file system free space
   OsStatus = OS_FileSysStatVolume(FileSys, &FileStats);
   
   if (OsStatus == OS_SUCCESS)
   {
      *BlockCount = FileStats.blocks_free;
      *ByteCount  = FileStats.block_size * FileStats.blocks_free;

      RetStatus = CFE_SUCCESS;
   }
   else
   {
      CFE_EVS_SendEvent(FILESYS_GET_VOL_FREE_SPACE_EID, CFE_EVS_EventType_ERROR,
                        "Error computing free space for file system volume %s. OS_FileSysStatVolume() returned %d",
                        FileSys, (int)OsStatus);

      RetStatus = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
   }

   return RetStatus;

} /* End GetVolFreeSpace() */
