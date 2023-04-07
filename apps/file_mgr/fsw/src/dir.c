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
**    Implement the DIR_Class methods
**
**  Notes:
**    1. TODO: Complete EDS integration. EDS is used for cmd & tlm but not files
**       so the LoadFileEntry(), used during telemetry and file generation, uses
**       a dir.h typedef and not an EDS generated typedef  
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

/*
** Include Files:
*/

#include <string.h>

#include "app_cfg.h"
#include "dir.h"


/**********************/
/** Type Definitions **/
/**********************/

typedef enum
{

   SEND_ONE_DIR_TLM_PKT,
   SEND_ALL_DIR_TLM_PKT

} SendDirListOpt_t;


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static bool SendDirListTlm(const char *DirName, uint16 DirListOffset, bool IncludeSizeTime, SendDirListOpt_t SendDirListOpt);
static void LoadFileEntry(const char* PathFilename, DIR_FileEntry_t* FileEntry, uint16* TaskBlockCount, bool IncludeSizeTime);
static bool WriteDirListToFile(const char* DirNameWithSep, osal_id_t DirId, int32 FileHandle, bool IncludeSizeTime);


/**********************/
/** Global File Data **/
/**********************/

static DIR_Class_t*  Dir = NULL;


/******************************************************************************
** Function: DIR_Constructor
**
*/
void DIR_Constructor(DIR_Class_t*  DirPtr, const INITBL_Class_t* IniTbl)
{
 
   Dir = DirPtr;

   CFE_PSP_MemSet((void*)Dir, 0, sizeof(DIR_Class_t));
 
   Dir->IniTbl = IniTbl;

   /* These are used in loops so save value once */
   Dir->TaskFileStatCnt   = INITBL_GetIntConfig(Dir->IniTbl, CFG_TASK_FILE_STAT_CNT);
   Dir->TaskFileStatDelay = INITBL_GetIntConfig(Dir->IniTbl, CFG_TASK_FILE_STAT_DELAY);
   Dir->ChildTaskPerfId   = INITBL_GetIntConfig(Dir->IniTbl, CFG_CHILD_TASK_PERF_ID);

} /* End DIR_Constructor() */


/******************************************************************************
** Function: DIR_CreateCmd
**
*/
bool DIR_CreateCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_CreateDir_Payload_t *CreateCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_CreateDir_t);
   bool                RetStatus = false;
   int32               SysStatus;
   os_err_name_t       OsErrStr;   
   FileUtil_FileInfo_t FileInfo;


   FileInfo = FileUtil_GetFileInfo(CreateCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_NONEXISTENT)
   {
      
      SysStatus = OS_mkdir(CreateCmd->DirName, 0);

      if (SysStatus == OS_SUCCESS)
      {
         RetStatus = true;         
         CFE_EVS_SendEvent(DIR_CREATE_EID, CFE_EVS_EventType_DEBUG, "Created directory %s", CreateCmd->DirName);  
      }
      else
      {
         OS_GetErrorName(SysStatus,&OsErrStr);
         CFE_EVS_SendEvent(DIR_CREATE_ERR_EID, CFE_EVS_EventType_ERROR,
            "Create directory %s failed: Parameters validated but OS_mkdir() failed %s",
            CreateCmd->DirName, OsErrStr);
      }
   }
   else
   {
      CFE_EVS_SendEvent(DIR_CREATE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Create directory command for %s failed due to %s",
         CreateCmd->DirName, FileUtil_FileStateStr(FileInfo.State));  
   }
   
   return RetStatus;
   
} /* End DIR_CreateCmd() */



/******************************************************************************
** Function: DIR_DeleteAllCmd
**
*/
bool DIR_DeleteAllCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_DeleteAllDir_Payload_t *DeleteAllCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_DeleteAllDir_t);
   bool                RetStatus = false;
   int32               SysStatus;
   os_err_name_t       OsErrStr;   
   osal_id_t           DirId;
   os_dirent_t         DirEntry;
   FileUtil_FileInfo_t FileInfo;
   
   uint32  NameLength = 0;
   uint32  DeleteCount = 0;
   uint32  FileNotDeletedCount = 0;
   uint32  DirSkippedCount = 0;
   char    DirWithSep[OS_MAX_PATH_LEN] = "\0";
   char    Filename[OS_MAX_PATH_LEN] = "\0"; 


   FileInfo = FileUtil_GetFileInfo(DeleteAllCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_IS_DIR)
   {
      
      strcpy(DirWithSep, DeleteAllCmd->DirName);
      if (FileUtil_AppendPathSep(DirWithSep, OS_MAX_PATH_LEN))
      {
      
         SysStatus = OS_DirectoryOpen(&DirId, DeleteAllCmd->DirName);
         
         if (SysStatus == OS_SUCCESS) {
            
            while ((SysStatus = OS_DirectoryRead(DirId, &DirEntry)) == OS_SUCCESS) {
               
               /* Ignore the "." and ".." directory entries */
               
               if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                   (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR)  != 0))
               {
 
                  /* Construct full path filename */
                  NameLength = strlen(DirWithSep) + strlen(OS_DIRENTRY_NAME(DirEntry));

                  if (NameLength < OS_MAX_PATH_LEN)
                  {
  
                     strncpy(Filename, DirWithSep, OS_MAX_PATH_LEN - 1);
                     Filename[OS_MAX_PATH_LEN - 1] = '\0';
                          
                     strncat(Filename, OS_DIRENTRY_NAME(DirEntry), (NameLength - strlen(DirWithSep)));

                     FileInfo = FileUtil_GetFileInfo(Filename, OS_MAX_PATH_LEN, false);

                     switch (FileInfo.State)
                     {
                              
                        case FILEUTIL_FILE_CLOSED:                   
                        
                           if ((SysStatus = OS_remove(Filename)) == OS_SUCCESS)
                           {
                              
                              SysStatus = OS_DirectoryRewind(DirId);  /* Rewind prevents file system from getting confused */

                              ++DeleteCount;
                           }
                           else
                           {
                           
                              ++FileNotDeletedCount;
                           
                           }
                           break;
                        
                        case FILEUTIL_FILE_IS_DIR:
                        
                           ++DirSkippedCount;
                           break;
         
                        case FILEUTIL_FILENAME_INVALID:
                        case FILEUTIL_FILE_NONEXISTENT:
                        case FILEUTIL_FILE_OPEN:
                        default:
                           ++FileNotDeletedCount;
                           break;
                  
                     } /* End FileInfo.State switch */
                  
                  } /* End if valid Filename length */
                  else {

                     ++FileNotDeletedCount;
                  
                  } /* End if invalid Filename length */
               } /* End if "." or ".." directory entries */
            } /* End while read dir */

            OS_DirectoryClose(DirId);
             
            CFE_EVS_SendEvent(DIR_DELETE_ALL_EID, CFE_EVS_EventType_DEBUG,
                  "Deleted %d files in %s", (int)DeleteCount, DeleteAllCmd->DirName);

            if (FileNotDeletedCount > 0)
            {
               CFE_EVS_SendEvent(DIR_DELETE_ALL_WARN_EID, CFE_EVS_EventType_INFORMATION,
                  "Delete all files in %s warning: %d could not be deleted",
                  DeleteAllCmd->DirName, FileNotDeletedCount);
            }

            if (DirSkippedCount > 0)
            {
               CFE_EVS_SendEvent(DIR_DELETE_ALL_WARN_EID, CFE_EVS_EventType_INFORMATION,
                  "Delete all files in %s warning: %d directories skipped",
                  DeleteAllCmd->DirName, DirSkippedCount);
            }
         
            if ((FileNotDeletedCount > 0) || (DirSkippedCount > 0)) ++Dir->CmdWarningCnt;
          
         } /* End if open dir */
         else
         {
            OS_GetErrorName(SysStatus,&OsErrStr);
            CFE_EVS_SendEvent(DIR_DELETE_ALL_ERR_EID, CFE_EVS_EventType_ERROR,
               "Delete all files in %s failed: Error opening dir, status %s",
               DeleteAllCmd->DirName, OsErrStr);
             
         } /* End if DirId == NULL */
      
      
      } /* End if append path separator */
      else
      {
      
         CFE_EVS_SendEvent(DIR_DELETE_ALL_ERR_EID, CFE_EVS_EventType_ERROR,
            "Delete all files in %s failed: Max path length %d exceeded",
            DeleteAllCmd->DirName, OS_MAX_PATH_LEN);
      
      } /* End if couldn't append path separator */ 
      
   } /* End if file is a directory */ 
   else
   {
   
      CFE_EVS_SendEvent(DIR_DELETE_ALL_ERR_EID, CFE_EVS_EventType_ERROR,
         "Delete all files in %s failed due to %s",
         DeleteAllCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
   
   } /* End if file is not a directory */
   
   return RetStatus;

} /* End of DIR_DeleteAllCmd() */


/******************************************************************************
** Function: DIR_DeleteCmd
**
*/
bool DIR_DeleteCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_DeleteDir_Payload_t *DeleteCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_DeleteDir_t);
   bool                RetStatus = false;
   int32               SysStatus;
   bool                RemoveDir = true;
   os_err_name_t       OsErrStr;   
   osal_id_t           DirId;
   os_dirent_t         DirEntry;
   FileUtil_FileInfo_t FileInfo;
   
   
   FileInfo = FileUtil_GetFileInfo(DeleteCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_IS_DIR)
   {
      
      SysStatus = OS_DirectoryOpen(&DirId, DeleteCmd->DirName);  /* Open the dir so we can see if it is empty */ 

      if (SysStatus == OS_SUCCESS)
      {
 
        /* Look for a directory entry that is not "." or ".." */
        while (((SysStatus = OS_DirectoryRead(DirId, &DirEntry)) == OS_SUCCESS) && (RemoveDir == true))
        {
           
            if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR)  != 0))
            {
                   
               CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
                  "Delete directory %s failed: Dir not empty", DeleteCmd->DirName);

                RemoveDir = false;
            }
         }

         OS_DirectoryClose(DirId);

      } /* End if opened dir */
      else
      {
         OS_GetErrorName(SysStatus,&OsErrStr);
         CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
            "Delete directory %s failed: Unable to open dir, %s", 
            DeleteCmd->DirName, OsErrStr);

         RemoveDir = false;
      }

      if (RemoveDir)
      {
         
         SysStatus = OS_rmdir(DeleteCmd->DirName);

         if (SysStatus == OS_SUCCESS)
         {
            
            RetStatus = true;
            CFE_EVS_SendEvent(DIR_DELETE_EID, CFE_EVS_EventType_DEBUG, "Deleted directory %s", DeleteCmd->DirName);
            
         }
         else
         {
            OS_GetErrorName(SysStatus,&OsErrStr);            
            CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
               "Delete directory %s failed: Parameters validated but OS_rmdir() failed, %s",
               DeleteCmd->DirName, OsErrStr);

         }
      }


   } /* End if file is a directory */
   else
   {
      
      CFE_EVS_SendEvent(DIR_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Delete directory command for %s failed due to %s",
         DeleteCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
      
   } /* End if file is not a directory */
   
   return RetStatus;
   
   
} /* End DIR_DeleteCmd() */


/******************************************************************************
** Function:  DIR_ResetStatus
**
*/
void DIR_ResetStatus()
{
 
   Dir->CmdWarningCnt = 0;
   
} /* End DIR_ResetStatus() */


/******************************************************************************
** Function: DIR_SendDirListTlmCmd
**
*/
bool DIR_SendDirListTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const FILE_MGR_SendDirListTlm_Payload_t *SendDirListTlmCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_SendDirListTlm_t);      
   bool  RetStatus;

   RetStatus = SendDirListTlm(SendDirListTlmCmd->DirName, SendDirListTlmCmd->DirListOffset, 
                              SendDirListTlmCmd->IncludeSizeTime, SEND_ONE_DIR_TLM_PKT);

   return RetStatus;
      
} /* End DIR_SendDirListTlmCmd() */


/******************************************************************************
** Function: DIR_SendDirTlmCmd
**
** Notes:
**   1. Sends an entire directry listing in mutliple telemetry messages using
**      DIR_SendDirListTlmCmd() for each sublisting. 
** 
*/
bool DIR_SendDirTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_SendDirTlm_Payload_t *SendDirTlmCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_SendDirTlm_t);
   
   bool  RetStatus;

   RetStatus = SendDirListTlm(SendDirTlmCmd->DirName, 0, 
                              SendDirTlmCmd->IncludeSizeTime, SEND_ALL_DIR_TLM_PKT);

   return RetStatus;

} /* End DIR_SendDirTlmCmd() */


/******************************************************************************
** Function: DIR_WriteListFileCmd
**
** Notes:
**   1. Target file will be overwritten if it exists and is closed.
** 
*/
bool DIR_WriteListFileCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_WriteDirListFile_Payload_t *WriteDirListFileCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_WriteDirListFile_t);
   bool RetStatus = false;
      
   int32         SysStatus;
   osal_id_t     FileHandle;
   osal_id_t     DirId;
   os_err_name_t OsErrStr;
   char DirNameWithSep[OS_MAX_PATH_LEN] = "\0";
   char Filename[OS_MAX_PATH_LEN] = "\0";
   FileUtil_FileInfo_t FileInfo;

   
   FileInfo = FileUtil_GetFileInfo(WriteDirListFileCmd->DirName, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_IS_DIR)
   {
      
      if (WriteDirListFileCmd->Filename[0] == '\0')
      {
         strncpy(Filename, INITBL_GetStrConfig(Dir->IniTbl,CFG_DIR_LIST_FILE_DEFNAME), OS_MAX_PATH_LEN - 1);
         Filename[OS_MAX_PATH_LEN - 1] = '\0';
      }
      else
      {
         CFE_PSP_MemCpy(Filename, WriteDirListFileCmd->Filename, OS_MAX_PATH_LEN);
      }
      
      FileInfo = FileUtil_GetFileInfo(Filename, OS_MAX_PATH_LEN, false);

      if ((FileInfo.State == FILEUTIL_FILE_CLOSED) ||
          (FileInfo.State == FILEUTIL_FILE_NONEXISTENT))
      {
              
         strcpy(DirNameWithSep, WriteDirListFileCmd->DirName);
         if (FileUtil_AppendPathSep(DirNameWithSep, OS_MAX_PATH_LEN))
         {
      
            SysStatus = OS_DirectoryOpen(&DirId, WriteDirListFileCmd->DirName);
            if (SysStatus == OS_SUCCESS)
            {
               
               SysStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_READ_WRITE);
                   
               if (SysStatus == OS_SUCCESS)
               {
                  
                  RetStatus = WriteDirListToFile(DirNameWithSep, DirId, FileHandle, WriteDirListFileCmd->IncludeSizeTime);
                 
                  OS_close(FileHandle);
                  
                  CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_EID, CFE_EVS_EventType_DEBUG, 
                                    "Directory %s written to %s with status %d", 
                                    WriteDirListFileCmd->DirName, WriteDirListFileCmd->Filename, RetStatus);
                  
               }
               else
               {
                  OS_GetErrorName(SysStatus,&OsErrStr);
                  CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                    "Send dir list pkt cmd failed for %s: File create status %s",
                                    Filename, OsErrStr);
           
               } /* End if error opening output file */
            
               OS_DirectoryClose(DirId);
            
            } /* Open dir */
            else
            {
            
               OS_GetErrorName(SysStatus, &OsErrStr);
               CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "Send dir list pkt cmd failed for %s: Open dir atatus %s",
                                 WriteDirListFileCmd->DirName, OsErrStr);
            
            } /* DirPtr == NULL */
      
      
         } /* DirWithSep length okay */
         else
         {
         
            CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Send dir list pkt cmd failed: %s with path separator is too long",
                              WriteDirListFileCmd->DirName);

         } /* DirWithSep length too long */
      
      } /* Target file okay to write */
      else
      {
         
         CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Write dir list file cmd failed: %s must be closed or nonexistent. File state is %s",
                           Filename, FileUtil_FileStateStr(FileInfo.State));
         
      } /* Target file not okay to write */
   
   } /* End if file is a directory */
   else
   {
      
      CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Write dir list file cmd failed: %s is not a directory. Identified as %s",
                        WriteDirListFileCmd->DirName, FileUtil_FileStateStr(FileInfo.State));
      
   } /* End if file is not a directory */
   
   return RetStatus;
      
} /* End DIR_WriteListFileCmd() */


/******************************************************************************
** Function: LoadFileEntry
**
** Notes:
**   1. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
** 
*/
static void LoadFileEntry(const char *PathFilename, DIR_FileEntry_t *FileEntry, 
                          uint16 *TaskBlockCount, bool IncludeSizeTime)
{
   
   int32       SysStatus; 
   os_fstat_t  FileStatus;
   
   
   if (IncludeSizeTime)
   {
      
      CHILDMGR_PauseTask(TaskBlockCount, Dir->TaskFileStatCnt, 
                         Dir->TaskFileStatDelay, Dir->ChildTaskPerfId);
                         
      CFE_PSP_MemSet(&FileStatus, 0, sizeof(os_fstat_t));
      SysStatus = OS_stat(PathFilename, &FileStatus);
      
      if (SysStatus == OS_SUCCESS)
      {
         
         FileEntry->Size = FileStatus.FileSize;
         FileEntry->Mode = FileStatus.FileModeBits;
         FileEntry->Time = OS_GetLocalTime(&FileStatus.FileTime);
        
      }
      else
      {
         
         FileEntry->Size = 0;
         FileEntry->Time = 0;
         FileEntry->Mode = 0;
      
      }
      
   } /* End if include Size & Time */
   else
   {
         
      FileEntry->Size = 0;
      FileEntry->Time = 0;
      FileEntry->Mode = 0;
      
   } /* End if don't include Size & Time */


} /* End LoadFileEntry() */


/******************************************************************************
** Function: SendDirListTlm
**
** Notes:
**   1. This function is called by multiple commands to the wording of the
**      event messages is intentionally generic.
**   2. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
*/
bool SendDirListTlm(const char *DirName, uint16 DirListOffset, bool IncludeSizeTime,
                    SendDirListOpt_t SendOpt)
{
   
   bool                RetStatus = false;
   int32               SysStatus;
   os_err_name_t       OsErrStr;   
   osal_id_t           DirId;
   os_dirent_t         DirEntry;
   FileUtil_FileInfo_t FileInfo;
   DIR_FileEntry_t     DirFileEntry;
   
   bool   ReadingDir     = true;
   bool   CreatingTlmPkt = true;
   uint16 TaskBlockCnt = 0;    /* See prologue */
   uint16 FilenameLen;
   uint16 DirWithSepLen;
   char   DirWithSep[OS_MAX_PATH_LEN] = "\0";
   char   PathFilename[OS_MAX_PATH_LEN] = "\0";


   CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_EID, CFE_EVS_EventType_DEBUG,
                     "sizeof(Dir->ListTlm.Payload.FileList) = %d",
                     (unsigned int)sizeof(Dir->ListTlm.Payload.FileList));

   FileInfo = FileUtil_GetFileInfo(DirName, OS_MAX_PATH_LEN, false);
                          
   if (FileInfo.State == FILEUTIL_FILE_IS_DIR)
   {
      
      /* Clears counters and nulls strings */
      CFE_MSG_Init(CFE_MSG_PTR(Dir->ListTlm.TelemetryHeader), 
                   CFE_SB_ValueToMsgId(INITBL_GetIntConfig(Dir->IniTbl,CFG_FILE_MGR_DIR_LIST_TLM_TOPICID)),
                   sizeof(FILE_MGR_DirListTlm_t));
      
      strcpy(DirWithSep, DirName);
      if (FileUtil_AppendPathSep(DirWithSep, OS_MAX_PATH_LEN))
      {
      
         DirWithSepLen = strlen(DirWithSep);
         
         SysStatus = OS_DirectoryOpen(&DirId, DirName);
         if (SysStatus == OS_SUCCESS) 
         {
            
            strncpy(Dir->ListTlm.Payload.DirName, DirName, OS_MAX_PATH_LEN);
            Dir->ListTlm.Payload.DirListOffset = DirListOffset;
            
            while (ReadingDir)
            {
            
               SysStatus = OS_DirectoryRead(DirId, &DirEntry);
               if (SysStatus != OS_SUCCESS)
               {
                  ReadingDir = false;
               }
               else if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                        (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR)  != 0))
               {
            
                  /* 
                  ** General logic 
                  ** - Do not count the "." and ".." directory entries 
                  ** - Start packet listing at command-specified offset
                  ** - Stop when telemetry packet is full
                  */      
                  ++Dir->ListTlm.Payload.DirFileCnt;
               
                  if (CreatingTlmPkt)
                  {
                     if (Dir->ListTlm.Payload.DirFileCnt > Dir->ListTlm.Payload.DirListOffset)
                     {
                
                        FILE_MGR_DirListFileEntry_t* TlmFileEntry = &Dir->ListTlm.Payload.FileList[Dir->ListTlm.Payload.PktFileCnt];

                        FilenameLen = strlen(OS_DIRENTRY_NAME(DirEntry));

                        /* Verify combined directory plus filename length */
                        if ((FilenameLen < sizeof(DirFileEntry.Name)) &&
                           ((DirWithSepLen + FilenameLen) < OS_MAX_PATH_LEN))
                        {

                           strcpy(DirFileEntry.Name, OS_DIRENTRY_NAME(DirEntry));

                           strcpy(PathFilename, DirWithSep);
                           strcat(PathFilename, OS_DIRENTRY_NAME(DirEntry));

                           LoadFileEntry(PathFilename, &DirFileEntry, &TaskBlockCnt, IncludeSizeTime);

                           strncpy(TlmFileEntry->Name, DirFileEntry.Name, OS_MAX_PATH_LEN);
                           TlmFileEntry->Size = DirFileEntry.Size;
                           TlmFileEntry->Time = DirFileEntry.Time;
                           TlmFileEntry->Mode = DirFileEntry.Mode;
                        
                           ++Dir->ListTlm.Payload.PktFileCnt;
                        
                        }
                        else
                        {
                           Dir->CmdWarningCnt++;

                           CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_WARN_EID, CFE_EVS_EventType_INFORMATION,
                                             "Send dir list path/file len too long: Dir %s, File %s",
                                             DirWithSep, OS_DIRENTRY_NAME(DirEntry));
                        
                        } /* End if valid pah/filename length */
                     
                     } /* End if in range to fill packet */
                     if (Dir->ListTlm.Payload.PktFileCnt >= FILE_MGR_DIR_LIST_PKT_ENTRIES)
                     {
                        /* If sending one pkt, continue counting directory entries and send pkt when done */
                        if (SendOpt == SEND_ONE_DIR_TLM_PKT)
                        {
                           CreatingTlmPkt = false;
                        }
                        else
                        {
                           CFE_SB_TimeStampMsg(CFE_MSG_PTR(Dir->ListTlm.TelemetryHeader));
                           CFE_SB_TransmitMsg(CFE_MSG_PTR(Dir->ListTlm.TelemetryHeader), true);

                           CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_EID, CFE_EVS_EventType_INFORMATION,
                                             "Send all files for dir %s: offset=%d, pktcnt=%d",
                                             Dir->ListTlm.Payload.DirName,
                                             (int)Dir->ListTlm.Payload.DirListOffset, 
                                             (int)Dir->ListTlm.Payload.PktFileCnt);
                        
                           Dir->ListTlm.Payload.PktFileCnt = 0;
                           Dir->ListTlm.Payload.DirListOffset += FILE_MGR_DIR_LIST_PKT_ENTRIES;
                           memset(Dir->ListTlm.Payload.FileList, 0, sizeof(Dir->ListTlm.Payload.FileList));                              
                        }
                     } /* End if filled tlm packet */         
                  } /* End while creating telemetry message */
               } /* End if not current or parent directory */ 
            
            } /* End while reading the directory */
            
            OS_DirectoryClose(DirId);

            /*
            ** Always send packet if commanded to send one. If sending teh entire directory, only sen
            ** the last packet if it has entires==ies
            */
            if ((SendOpt == SEND_ONE_DIR_TLM_PKT) ||
                ((SendOpt == SEND_ALL_DIR_TLM_PKT) && (Dir->ListTlm.Payload.PktFileCnt != 0)))
            {
            
               CFE_SB_TimeStampMsg(CFE_MSG_PTR(Dir->ListTlm.TelemetryHeader));
               CFE_SB_TransmitMsg(CFE_MSG_PTR(Dir->ListTlm.TelemetryHeader), true);

               if (SendOpt == SEND_ONE_DIR_TLM_PKT)
               {
                  CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_EID, CFE_EVS_EventType_INFORMATION,
                                    "Send one pkt for dir %s: offset=%d",
                                    Dir->ListTlm.Payload.DirName,
                                    (int)Dir->ListTlm.Payload.DirListOffset);
               }
               else
               {
                  CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_EID, CFE_EVS_EventType_INFORMATION,
                                    "Send all files for dir %s: offset=%d, pktcnt=%d",
                                     Dir->ListTlm.Payload.DirName,
                                     (int)Dir->ListTlm.Payload.DirListOffset, 
                                     (int)Dir->ListTlm.Payload.PktFileCnt);
               }
               
            } /* End if send packet */

            RetStatus = true;
            
         } /* DirPtr != NULL */
         else
         {
            OS_GetErrorName(SysStatus,&OsErrStr);   
            CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Send dir list pkt cmd failed for %s: OS_opendir status %s",
                              DirName, OsErrStr);

         } /* DirPtr == NULL */
      
      } /* DirWithSep length okay */
      else
      {
         
         CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Send dir list pkt cmd failed: %s with path separator is too long",
                           DirName);

         
      } /* DirWithSep length too long */
   } /* End if file is a directory */
   else
   {
      
      CFE_EVS_SendEvent(DIR_SEND_LIST_PKT_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Send dir list pkt cmd failed: %s is not a directory. Identified as %s",
                        DirName, FileUtil_FileStateStr(FileInfo.State));
      
   } /* End if file is not a directory */
   
   return RetStatus;
      
} /* End SendDirListTlm() */


/******************************************************************************
** Function: WriteDirListToFile
**
** Notes:
**   1. The command handler took care of validating inputs and opening the
**      input directory and output file. This function is responsible for 
**      writing all of the content to the output file including the header.
**   2. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
**
*/
static bool WriteDirListToFile(const char *DirNameWithSep, osal_id_t DirId, 
                               int32 FileHandle, bool IncludeSizeTime)
{
   
   bool ReadingDir   = true;
   bool FileWriteErr = false;
   bool CreatedFile  = false;
     
   uint16 DirWithSepLen = strlen(DirNameWithSep);
   uint16 DirEntryLen   = 0;               /* Length of each directory entry */
   uint16 DirEntryCnt   = 0;
   uint16 FileEntryCnt  = 0;
   uint16 FileEntryMax  = INITBL_GetIntConfig(Dir->IniTbl, CFG_DIR_LIST_FILE_ENTRIES);
   uint16 TaskBlockCnt  = 0;               /* See prologue */
   int32  BytesWritten;
   char   PathFilename[OS_MAX_PATH_LEN] = "\0";   /* Combined directory path and entry filename */
 
   int32            SysStatus;  
   CFE_FS_Header_t  FileHeader;
   os_dirent_t      DirEntry;
   uint16           DirFileStatsLen = sizeof(DIR_ListFilesStats_t);
   uint16           DirFileEntryLen = sizeof(DIR_FileEntry_t);
   DIR_FileEntry_t  DirFileEntry;
   
   /*
   ** Create and write standard cFE file header
   */
   
   CFE_PSP_MemSet(&FileHeader, 0, sizeof(CFE_FS_Header_t));
   FileHeader.SubType = INITBL_GetIntConfig(Dir->IniTbl, CFG_DIR_LIST_FILE_SUBTYPE);
   strncpy(FileHeader.Description, "Directory Listing", sizeof(FileHeader.Description) - 1);
   FileHeader.Description[sizeof(FileHeader.Description) - 1] = '\0';

   BytesWritten = CFE_FS_WriteHeader(FileHandle, &FileHeader);
   if (BytesWritten == sizeof(CFE_FS_Header_t))
   {
      
      /* 
      ** Create initial stats structure and write it to the file. After the
      ** directory is written to the file, the stats fields will be updated.
      */

      CFE_PSP_MemSet(&Dir->ListFileStats, 0, DirFileStatsLen);
      strncpy(Dir->ListFileStats.DirName, DirNameWithSep, OS_MAX_PATH_LEN - 1);
	   Dir->ListFileStats.DirName[OS_MAX_PATH_LEN - 1] = '\0';
      
      BytesWritten = OS_write(FileHandle, &Dir->ListFileStats, DirFileStatsLen);      
      if (BytesWritten == DirFileStatsLen)
      {
      
         while (ReadingDir && !FileWriteErr)
         {
        
            SysStatus = OS_DirectoryRead(DirId, &DirEntry);
            
            if (SysStatus != OS_SUCCESS)
            {
                  
               ReadingDir = false;  /* Normal loop end - no more directory entries */
               
            }
            else if ((strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_CURRENT_DIR) != 0) &&
                     (strcmp(OS_DIRENTRY_NAME(DirEntry), FILEUTIL_PARENT_DIR) != 0))
            {
            
               /* 
               ** General logic 
               ** - Do not count the "." and ".." directory entries 
               ** - Count all files, but limit file to FILE_MGR_INI_DIR_LIST_FILE_ENTRIES
               */      
                  
               ++DirEntryCnt;

               if (FileEntryCnt < FileEntryMax)
               {
                     
                  DirEntryLen = strlen(OS_DIRENTRY_NAME(DirEntry));

                  /* Verify combined directory plus filename length */
                  if ((DirEntryLen < sizeof(DirFileEntry.Name)) &&
                     ((DirWithSepLen + DirEntryLen) < OS_MAX_PATH_LEN))
                  {

                     strncpy(PathFilename, DirNameWithSep, DirWithSepLen);
	        	         PathFilename[DirWithSepLen] = '\0';
                     strncat(PathFilename, OS_DIRENTRY_NAME(DirEntry), (OS_MAX_PATH_LEN - DirWithSepLen));

                     /* Populate directory list file entry */
                     strncpy(DirFileEntry.Name, OS_DIRENTRY_NAME(DirEntry), DirEntryLen);
		               DirFileEntry.Name[DirEntryLen] = '\0';
          
                     LoadFileEntry(PathFilename, &DirFileEntry, &TaskBlockCnt, IncludeSizeTime);
          
                     BytesWritten = OS_write(FileHandle, &DirFileEntry, DirFileEntryLen);
                     if (BytesWritten == DirFileEntryLen)
                     {
                
                        ++FileEntryCnt;
                     
                     } /* End if sucessful file write */
                     else
                     {
                        
                        FileWriteErr = true;
                     
                        CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                          "Write dir list file cmd failed: OS_write entry failed: result = %d, expected = %d",
                                          (int)BytesWritten, (int)DirFileEntryLen);
                       
                    } /* End if file write error */
         
                  } /* End if file name lengths valid */
                  else
                  {
                     
                     ++Dir->CmdWarningCnt;
                        
                     CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_WARN_EID, CFE_EVS_EventType_ERROR,
                                       "Write dir list file cmd warning: Combined dir/entry name too long: dir = %s, entry = %s",
                                       DirNameWithSep, OS_DIRENTRY_NAME(DirEntry));
                     
                  } /* End if invalid file entry name lengths */ 
         
               } /* End if within file entry write limit */
            } /* End if not current/parent directory */ 
         } /* End Reading Dir & Writing file loop */

         /*
         ** Update directory statistics in output file
         ** - Update local stats data structure
         ** - Back up file pointer to the start of the statisitics data
         ** - Write local stats data to the file
         */
   
         if ( (DirEntryCnt > 0) && !FileWriteErr)
         {

            Dir->ListFileStats.DirFileCnt      = DirEntryCnt;
            Dir->ListFileStats.FilesWrittenCnt = FileEntryCnt;

            OS_lseek(FileHandle, sizeof(CFE_FS_Header_t), OS_SEEK_SET);
        
            BytesWritten = OS_write(FileHandle, &Dir->ListFileStats, DirFileStatsLen);

            if (BytesWritten == DirFileStatsLen)
            {
            
               CreatedFile = true;
            
            } /* End if no stats write error */
            else
            {

               CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                                "Write dir list file cmd failed: OS_write stats failed: result = %d, expected = %d",
                                (int)BytesWritten, (int)DirFileStatsLen);

            } /* End if stats write error */
 
         } /* End if sucessful dir read & file write loop */
               
      } /* End if no initial stats write error */
      else
      {

         CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Write dir list file cmd failed: OS_write stats failed: result = %d, expected = %d",
                          (int)BytesWritten, (int)DirFileStatsLen);
      
      } /* End if initial stats write error */         
   } /* End if cFE file header write error */      
   else
   {

      CFE_EVS_SendEvent(DIR_WRITE_LIST_FILE_ERR_EID, CFE_EVS_EventType_ERROR,
                       "Write dir list file cmd failed: OS_write cFE header failed: result = %d, expected = %d",
                       (int)BytesWritten, (int)sizeof(CFE_FS_Header_t));
      
   } /* End if cFE header write error */         

     
   return  CreatedFile;

   
} /* End WriteDirListToFile() */



