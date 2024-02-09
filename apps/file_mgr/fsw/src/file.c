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
**    Implement the FILE_Class methods
**
**  Notes:
**    1. The sprintf() event message strategy uses buffers that are longer than 
**       CFE_MISSION_EVS_MAX_MESSAGE_LENGTH and relies on CFE_EVS_SendEvent() to
**       truncate long messages. TODO - Improve this.
**    2. When get FileUtil_GetFileInfo() is used to verify whether a target file
**       exists it does not verify the directroy path is valid so an operation
**       could still get an error when it tries to use the target path/file.
**
*/

/*
** Include Files:
*/

#include <string.h>

#include "app_cfg.h"
#include "file.h"

/*******************************/
/** Local Function Prototypes **/
/*******************************/

static bool ConcatenateFiles(const char* SrcFile1, const char* SrcFile2, const char* TargetFile);
static bool ComputeFileCrc(const char* CmdName, const char* Filename, uint32* Crc, uint8 CrcType);


/**********************/
/** Global File Data **/
/**********************/

static FILE_Class_t  *File = NULL;


/******************************************************************************
** Function: FILE_Constructor
**
*/
void FILE_Constructor(FILE_Class_t *FilePtr, const INITBL_Class_t *IniTbl)
{
 
   File = FilePtr;

   CFE_PSP_MemSet((void*)File, 0, sizeof(FILE_Class_t));
 
   File->IniTbl = IniTbl;
   
   CFE_MSG_Init(CFE_MSG_PTR(File->InfoTlm.TelemetryHeader), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(File->IniTbl, CFG_FILE_MGR_FILE_INFO_TLM_TOPICID)), 
                sizeof(FILE_MGR_FileInfoTlm_t));

} /* End FILE_Constructor */


/******************************************************************************
** Function: FILE_ConcatenateCmd
**
** Notes:
**   1. FileUtil_GetFileInfo() verifies filename prior to checking state.
**
*/
bool FILE_ConcatenateCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_ConcatenateFile_CmdPayload_t *ConcatenateCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_ConcatenateFile_t);
   FileUtil_FileInfo_t FileInfo;
   char  EventErrStr[256] = "\0";
   
   bool  RetStatus    = false;
   bool  Source1Valid = false;
   bool  Source2Valid = false;
   bool  TargetValid  = false;
   
   sprintf(EventErrStr,"Unhandled concatenate file error");
   
   /*
   ** Verify source files exist and are closed
   ** Verify target file does not exist
   */
   
   FileInfo = FileUtil_GetFileInfo(ConcatenateCmd->Source1Filename, OS_MAX_PATH_LEN, false);
   if (FileInfo.State == FILEUTIL_FILE_CLOSED)
   {
      
      Source1Valid = true;
   }
   else
   {
      sprintf(EventErrStr,"Concatenate file cmd error: Source file must be closed. Source file 1 %s state is %s",
              ConcatenateCmd->Source1Filename, FileUtil_FileStateStr(FileInfo.State));
   }
   
   FileInfo = FileUtil_GetFileInfo(ConcatenateCmd->Source2Filename, OS_MAX_PATH_LEN, false);
   if (FileInfo.State == FILEUTIL_FILE_CLOSED)
   {
      Source2Valid = true;
   }
   else
   {
      sprintf(EventErrStr,"Concatenate file cmd error: Source file must be closed. Source file 2 %s state is %s",
              ConcatenateCmd->Source2Filename, FileUtil_FileStateStr(FileInfo.State));
   }
   FileInfo = FileUtil_GetFileInfo(ConcatenateCmd->TargetFilename, OS_MAX_PATH_LEN, false);
   if (FileInfo.State == FILEUTIL_FILE_NONEXISTENT)
   {
      TargetValid = true;
   }
   else 
   {
      sprintf(EventErrStr,"Concatenate file cmd error: Target file must not exist. Target file %s state is %s",
              ConcatenateCmd->TargetFilename, FileUtil_FileStateStr(FileInfo.State));
   }

   if (Source1Valid && Source2Valid && TargetValid)
   {
   
      RetStatus = ConcatenateFiles(ConcatenateCmd->Source1Filename,
                                   ConcatenateCmd->Source2Filename,
                                   ConcatenateCmd->TargetFilename);
      
      CFE_EVS_SendEvent(FILE_CONCATENATE_EID, CFE_EVS_EventType_DEBUG,
                        "Concatenated %s and %s to create %s. Success=%d",
                        ConcatenateCmd->Source1Filename,
                        ConcatenateCmd->Source2Filename,
                        ConcatenateCmd->TargetFilename, RetStatus);

   }
   else
   {
   
      CFE_EVS_SendEvent(FILE_CONCATENATE_ERR_EID, CFE_EVS_EventType_ERROR,"%s",EventErrStr);
      
   }
   
   return RetStatus;

} /* End of FILE_ConcatenateCmd() */


/******************************************************************************
** Function: FILE_CopyCmd
**
** Notes:
**   1. FileUtil_GetFileInfo() verifies filename prior to checking state.
**
*/
bool FILE_CopyCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_CopyFile_CmdPayload_t *CopyCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_CopyFile_t);
   FileUtil_FileInfo_t FileInfo;
   int32  SysStatus;   
   bool   PerformCopy = false;
   bool   RetStatus   = false;

   if (CMDMGR_ValidBoolArg(CopyCmd->Overwrite))
   {
      
      FileInfo = FileUtil_GetFileInfo(CopyCmd->SourceFilename, OS_MAX_PATH_LEN, false);
    
      if (FILEUTIL_FILE_EXISTS(FileInfo.State))
      {
            
         FileInfo = FileUtil_GetFileInfo(CopyCmd->TargetFilename, OS_MAX_PATH_LEN, false);
            
         if (CopyCmd->Overwrite == true)
         {
            
            if (FileInfo.State == FILEUTIL_FILE_CLOSED)
            {
               
               PerformCopy = true;
               
            }
            else
            {
            
               CFE_EVS_SendEvent(FILE_COPY_ERR_EID, CFE_EVS_EventType_ERROR,
                  "Copy file from %s to %s failed: Attempt to overwrite an open file",
                  CopyCmd->SourceFilename, CopyCmd->TargetFilename);
            }
         
         } /* End if overwrite true */
         else
         {
         
            if (FileInfo.State == FILEUTIL_FILE_NONEXISTENT)
            {
               
               PerformCopy = true;
               
            }
            else
            {
            
               CFE_EVS_SendEvent(FILE_COPY_ERR_EID, CFE_EVS_EventType_ERROR,
                  "Copy file from %s to %s failed: Target file exists and overwrite is false",
                  CopyCmd->SourceFilename, CopyCmd->TargetFilename);
            }
         
         } /* End if overwrite false */
            
      } /* End if source file exists */
      else
      {
          
         CFE_EVS_SendEvent(FILE_COPY_ERR_EID, CFE_EVS_EventType_ERROR,
            "Copy file from %s to %s failed: Source file doesn't exist",
            CopyCmd->SourceFilename, CopyCmd->TargetFilename);
      
      } /* End if source file doesn't exists */
   } /* End if valid Overwrite arg */
   else
   {
   
      CFE_EVS_SendEvent(FILE_COPY_ERR_EID, CFE_EVS_EventType_ERROR,
         "Copy file from %s to %s failed: Invalid overwrite flag %d. Must be True(%d) or False(%d)",
         CopyCmd->SourceFilename, CopyCmd->TargetFilename, CopyCmd->Overwrite, true, false);
   
   } /* End if invalid Overwrite arg */
      
   
   if (PerformCopy)
   {
      
      SysStatus = OS_cp(CopyCmd->SourceFilename, CopyCmd->TargetFilename);

      if (SysStatus == OS_SUCCESS)
      {
      
         RetStatus = true;      
         CFE_EVS_SendEvent(FILE_COPY_EID, CFE_EVS_EventType_DEBUG, "Copied file from %s to %s",
                           CopyCmd->SourceFilename, CopyCmd->TargetFilename);
      }
      else
      {
         
         CFE_EVS_SendEvent(FILE_COPY_ERR_EID, CFE_EVS_EventType_ERROR,
            "Copy file from %s to %s failed: Parameters validated but OS_cp() failed with status=%d",
            CopyCmd->SourceFilename, CopyCmd->TargetFilename, (int)SysStatus);
      }
      
   } /* End if Perform copy */
   
   return RetStatus;

} /* End of FILE_CopyCmd() */


/******************************************************************************
** Function: FILE_DecompressCmd
**
** Notes:
**    1. FileUtil_GetFileInfo() verifies filename prior to checking state
*/
bool FILE_DecompressCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   /* Can't uses const because CFE_PSP_Decompress() */
   const FILE_MGR_DecompressFile_CmdPayload_t *DecompressCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_DecompressFile_t);
   bool   RetStatus = false;

   int32  CfeStatus;
   FileUtil_FileInfo_t  FileInfo;
   
   
   FileInfo = FileUtil_GetFileInfo(DecompressCmd->SourceFilename, OS_MAX_PATH_LEN, false);
   
   if (FileInfo.State == FILEUTIL_FILE_CLOSED)
   {
   
      FileInfo = FileUtil_GetFileInfo(DecompressCmd->TargetFilename, OS_MAX_PATH_LEN, false);
   
      if (FileInfo.State == FILEUTIL_FILE_NONEXISTENT)
      {
          
         //todo: CfeStatus = CFE_FS_Decompress(DecompressCmd->SourceFilename, DecompressCmd->TargetFilename); 
         CfeStatus = CFE_SUCCESS;

         if (CfeStatus == CFE_SUCCESS)
         {
 
            RetStatus = true;
            CFE_EVS_SendEvent(FILE_DECOMPRESS_EID, CFE_EVS_EventType_DEBUG,
                              "%s decompressed to %s",
                              DecompressCmd->SourceFilename, DecompressCmd->TargetFilename);
         }
         else
         {

            CFE_EVS_SendEvent(FILE_DECOMPRESS_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Error decompressing %s to %s. CFE status = 0x%04X",
                              DecompressCmd->SourceFilename, DecompressCmd->TargetFilename, CfeStatus);
         }
         
      } /* End if target doesn't exist */ 
      else
      {
   
         CFE_EVS_SendEvent(FILE_DECOMPRESS_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Decompress file cmd error: Target file %s already exists",
                           DecompressCmd->TargetFilename);
                           
      } /* End if target does exist */
   
   } /* End if source exists and is closed */
   else
   {
   
      CFE_EVS_SendEvent(FILE_DECOMPRESS_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Decompress file cmd error: Source file must exist and be closed. Soucre file %s state is %s",
                        DecompressCmd->SourceFilename, FileUtil_FileStateStr(FileInfo.State) );
   
   } /* End if source file not an existing file that is closed */ 
    
   
   return RetStatus;

} /* End of FILE_DecompressCmd() */


/******************************************************************************
** Function: FILE_DeleteCmd
**
** Notes:
**   1. FileUtil_GetFileInfo() verifies filename prior to checking state.
**
*/
bool FILE_DeleteCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_DeleteFile_CmdPayload_t *DeleteCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_DeleteFile_t);
   FileUtil_FileInfo_t FileInfo;
   int32  SysStatus;
   bool   RetStatus = false;
   
   FileInfo = FileUtil_GetFileInfo(DeleteCmd->Filename, OS_MAX_PATH_LEN, false);

   if (FileInfo.State == FILEUTIL_FILE_CLOSED)
   {
 
         SysStatus = OS_remove(DeleteCmd->Filename);

         if (SysStatus == OS_SUCCESS)
         {
            
            RetStatus = true;
            CFE_EVS_SendEvent(FILE_DELETE_EID, CFE_EVS_EventType_DEBUG, "Deleted file %s", DeleteCmd->Filename);
            
         }
         else
         {
            
            CFE_EVS_SendEvent(FILE_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
               "Delete directory %s failed: Parameters validated but OS_remove() failed with status=%d",
               DeleteCmd->Filename, (int)SysStatus);

         }

   } /* End if file closed */
   else
   {
      
      CFE_EVS_SendEvent(FILE_DELETE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Delete file command for %s rejected due to %s.",
         DeleteCmd->Filename, FileUtil_FileStateStr(FileInfo.State));
      
   }

   return RetStatus;

} /* End of FILE_DeleteCmd() */


/******************************************************************************
** Function: FILE_MoveCmd
**
** Notes:
**   1. FileUtil_GetFileInfo() verifies filename prior to checking state.
**
*/
bool FILE_MoveCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_MoveFile_CmdPayload_t *MoveCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_MoveFile_t);
   FileUtil_FileInfo_t FileInfo;
   int32  SysStatus;   
   bool   PerformMove = false;
   bool   RetStatus   = false;

   if (CMDMGR_ValidBoolArg(MoveCmd->Overwrite))
   {
      
      FileInfo = FileUtil_GetFileInfo(MoveCmd->SourceFilename, OS_MAX_PATH_LEN, false);
    
      if (FILEUTIL_FILE_EXISTS(FileInfo.State))
      {
            
         FileInfo = FileUtil_GetFileInfo(MoveCmd->TargetFilename, OS_MAX_PATH_LEN, false);
            
         if (MoveCmd->Overwrite == true)
         {
            
            if (FileInfo.State == FILEUTIL_FILE_CLOSED)
            {
               
               PerformMove = true;
               
            }
            else
            {
            
               CFE_EVS_SendEvent(FILE_MOVE_ERR_EID, CFE_EVS_EventType_ERROR,
                  "Move file from %s to %s failed: Attempt to overwrite an open file",
                  MoveCmd->SourceFilename, MoveCmd->TargetFilename);
            }
         
         } /* End if overwrite true */
         else
         {
         
            if (FileInfo.State == FILEUTIL_FILE_NONEXISTENT)
            {
               
               PerformMove = true;
               
            }
            else
            {
            
               CFE_EVS_SendEvent(FILE_MOVE_ERR_EID, CFE_EVS_EventType_ERROR,
                  "Move file from %s to %s failed: Target file exists and overwrite is false",
                  MoveCmd->SourceFilename, MoveCmd->TargetFilename);
            }
         
         } /* End if overwrite false */
            
      } /* End if source file exists */
      else
      {
          
         CFE_EVS_SendEvent(FILE_MOVE_ERR_EID, CFE_EVS_EventType_ERROR,
            "Move file from %s to %s failed: Source file doesn't exist",
            MoveCmd->SourceFilename, MoveCmd->TargetFilename);

      } /* End if source file doesn't exists */
   } /* End if valid Overwrite arg */
   else
   {
   
      CFE_EVS_SendEvent(FILE_MOVE_ERR_EID, CFE_EVS_EventType_ERROR,
         "Move file from %s to %s failed: Invalid overwrite flag %d. Must be True(%d) or False(%d)",
         MoveCmd->SourceFilename, MoveCmd->TargetFilename, MoveCmd->Overwrite, true, false);
   
   } /* End if invalid Overwrite arg */
      
   
   if (PerformMove)
   {
      
      SysStatus = OS_mv(MoveCmd->SourceFilename, MoveCmd->TargetFilename);

      if (SysStatus == OS_SUCCESS)
      {
      
         RetStatus = true;      
         CFE_EVS_SendEvent(FILE_MOVE_EID, CFE_EVS_EventType_DEBUG, "Move file from %s to %s",
                           MoveCmd->SourceFilename, MoveCmd->TargetFilename);
      }
      else
      {
         
         CFE_EVS_SendEvent(FILE_COPY_ERR_EID, CFE_EVS_EventType_ERROR,
            "Move file %s to %s failed: Parameters validated but OS_mv() failed with status=%d",
            MoveCmd->SourceFilename, MoveCmd->TargetFilename, (int)SysStatus);
      }
      
   } /* End if Perform move */
   
   return RetStatus;

} /* End of FILE_MoveCmd() */


/******************************************************************************
** Function: FILE_RenameCmd
**
** Notes:
**   1. FileUtil_GetFileInfo() verifies filename prior to checking state.
**
*/
bool FILE_RenameCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_RenameFile_CmdPayload_t *RenameCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_RenameFile_t);
   FileUtil_FileInfo_t FileInfo;
   int32  SysStatus;   
   bool   RetStatus = false;
   

   FileInfo = FileUtil_GetFileInfo(RenameCmd->SourceFilename, OS_MAX_PATH_LEN, false);
   
   if (FILEUTIL_FILE_EXISTS(FileInfo.State))
   {
   
      FileInfo = FileUtil_GetFileInfo(RenameCmd->TargetFilename, OS_MAX_PATH_LEN, false);
   
      if (FileInfo.State == FILEUTIL_FILE_NONEXISTENT)
      {
      
         SysStatus = OS_rename(RenameCmd->SourceFilename, RenameCmd->TargetFilename);
        
         if (SysStatus == OS_SUCCESS)
         {
            
            RetStatus = true;
            CFE_EVS_SendEvent(FILE_RENAME_EID, CFE_EVS_EventType_DEBUG, "Renamed file from %s to %s",
               RenameCmd->SourceFilename, RenameCmd->TargetFilename);
            
         }
         else
         {
            
            CFE_EVS_SendEvent(FILE_RENAME_ERR_EID, CFE_EVS_EventType_ERROR,
               "Renamed file %s to %s failed: Parameters validated but OS_rename() failed with status=%d",
               RenameCmd->SourceFilename, RenameCmd->TargetFilename, (int)SysStatus);

         }

      
      } /* End if target file doesn't exist */
      else
      {
       
         CFE_EVS_SendEvent(FILE_RENAME_ERR_EID, CFE_EVS_EventType_ERROR,
            "Renamed file %s to %s failed: Target file exists",
            RenameCmd->SourceFilename, RenameCmd->TargetFilename);
      
      }
   } /* End if source file exists */
   else
   {
    
      CFE_EVS_SendEvent(FILE_RENAME_ERR_EID, CFE_EVS_EventType_ERROR,
         "Renamed file %s to %s failed: Source file doesn't exist",
         RenameCmd->SourceFilename, RenameCmd->TargetFilename);
   
   }
   
   return RetStatus;

} /* End of FILE_RenameCmd() */


/******************************************************************************
** Function:  FILE_ResetStatus
**
*/
void FILE_ResetStatus()
{
 
   File->CmdWarningCnt = 0;
   
} /* End FILE_ResetStatus() */


/******************************************************************************
** Function: FILE_SendInfoTlmCmd
**
** Notes:
**   1. FileUtil_GetFileInfo() verifies filename prior to checking state.
**   2. Specifying an invalid CRC type or not being able to compute a valid
**      CRC are both considered errors as opposed to warnings so the command
**      fails and no information telemetry packet is sent. 
**
*/
bool FILE_SendInfoTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_SendFileInfoTlm_CmdPayload_t *SendInfoTlmCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_SendFileInfoTlm_t);
   FileUtil_FileInfo_t FileInfo;
   uint32  Crc;
   bool    RetStatus = false;
   
   FileInfo = FileUtil_GetFileInfo(SendInfoTlmCmd->Filename, OS_MAX_PATH_LEN, true);
  
   if (FILEUTIL_FILE_EXISTS(FileInfo.State))
   {

      strncpy(File->InfoTlm.Payload.Filename, SendInfoTlmCmd->Filename, OS_MAX_PATH_LEN - 1);
      File->InfoTlm.Payload.Filename[OS_MAX_PATH_LEN - 1] = '\0';

      File->InfoTlm.Payload.State = FileInfo.State;
      File->InfoTlm.Payload.Size  = FileInfo.Size;
      File->InfoTlm.Payload.Time  = FileInfo.Time;
      File->InfoTlm.Payload.Mode  = FileInfo.Mode;
      File->InfoTlm.Payload.Crc   = 0;
      File->InfoTlm.Payload.CrcComputed = false;
      
      if (SendInfoTlmCmd->ComputeCrc == true)
      {

         if (SendInfoTlmCmd->CrcType == CFE_MISSION_ES_CRC_16)
         {
  
            if (ComputeFileCrc("Send File Info", SendInfoTlmCmd->Filename,
                               &Crc, SendInfoTlmCmd->CrcType))
            {

               File->InfoTlm.Payload.Crc = Crc;
               File->InfoTlm.Payload.CrcComputed = true;
               RetStatus = true;
            
            }
         }
         else
         {
            
            CFE_EVS_SendEvent(FILE_SEND_INFO_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Send file info failed: Invalid CRC type %d. See cFE ES for valid types.",
                              SendInfoTlmCmd->CrcType);
            
         } /* End if invalid CrcType */
         
      } /* End if compute CRC */
      else
      {
         
         RetStatus = true;
      
      } /* End if don't compute CRC */
      
      if (RetStatus == true)
      {

         CFE_SB_TimeStampMsg(CFE_MSG_PTR(File->InfoTlm.TelemetryHeader));
         CFE_SB_TransmitMsg(CFE_MSG_PTR(File->InfoTlm.TelemetryHeader), true);            
         
         CFE_EVS_SendEvent(FILE_SEND_INFO_EID, CFE_EVS_EventType_DEBUG,
                           "Sent info pkt for file %s with state %s",
                           SendInfoTlmCmd->Filename, FileUtil_FileStateStr(FileInfo.State));

      }
   
   } /* End if file exists */
   else
   {
      
      CFE_EVS_SendEvent(FILE_SEND_INFO_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Send file info failed: %s doesn't exist",
                        SendInfoTlmCmd->Filename);
   
   } /* End if file doesn't exist */
  
  
   return RetStatus;

} /* FILE_SendInfoTlmCmd() */


/******************************************************************************
** Function: FILE_SetPermissionsCmd
**
** Notes:
**   1. FileUtil_GetFileInfo() verifies filename prior to checking state.
**
*/
bool FILE_SetPermissionsCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_MGR_SetFilePermissions_CmdPayload_t *SetPermissionsCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_MGR_SetFilePermissions_t);
   FileUtil_FileInfo_t FileInfo;
   int32  SysStatus;   
   bool   RetStatus = false;
   
   
   if (FileUtil_VerifyFilenameStr(SetPermissionsCmd->Filename))
   {
      
      FileInfo = FileUtil_GetFileInfo(SetPermissionsCmd->Filename, OS_MAX_PATH_LEN, false);
      if (FILEUTIL_FILE_EXISTS(FileInfo.State))
      {

         SysStatus = OS_chmod(SetPermissionsCmd->Filename, SetPermissionsCmd->Mode);

         if (SysStatus == OS_SUCCESS)
         {
            
            RetStatus = true;
            CFE_EVS_SendEvent(FILE_SET_PERMISSIONS_EID, CFE_EVS_EventType_DEBUG,
               "Set permissions for file %s to octal %03o",
               SetPermissionsCmd->Filename, SetPermissionsCmd->Mode);
               
         }
         else
         {
               
            CFE_EVS_SendEvent(FILE_SET_PERMISSIONS_ERR_EID, CFE_EVS_EventType_ERROR,
               "Set file permissions failed: Parameters validated but OS_chmod() failed with status=%d",
               (int)SysStatus);
         }
         
      } /* End if file exists */
      else
      {
         
         CFE_EVS_SendEvent(FILE_SET_PERMISSIONS_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Set file permissions failed: Nonexistent file %s",
                           SetPermissionsCmd->Filename);

      }

   } /* End if valid filename */
   else
   {

      CFE_EVS_SendEvent(FILE_SET_PERMISSIONS_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Set file permissions failed: Invalid filename %s",
                        SetPermissionsCmd->Filename);
   
   } /* End if invalid filename */
   
   return RetStatus;

} /* End of FILE_SetPermissionsCmd() */


/******************************************************************************
** Function: ComputeFileCrc
**
** Notes:
**   1. TaskBlockCnt is the count of "task blocks" performed. A task block is 
**      is group of instructions that is CPU intensive and may need to be 
**      periodically suspended to prevent CPU hogging.
**
*/

static bool ComputeFileCrc(const char *CmdName, const char *Filename, uint32 *Crc, uint8 CrcType)
{
   
   int32         SysStatus;
   os_err_name_t OsErrStr;
   osal_id_t     FileHandle;
   int32         FileBytesRead;

   
   uint16  TaskBlockCnt = 0;   /* See prologue */
   uint32  CurrentCrc   = 0;
   bool    CrcComputed  = false;
   bool    ComputingCrc = true;
   

   *Crc = 0;
   SysStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_NONE, OS_READ_ONLY);
   
   if (SysStatus == OS_SUCCESS)
   {
   
      while (ComputingCrc)
      {
         
         FileBytesRead = OS_read(FileHandle, File->FileTaskBuf, FILE_MGR_TASK_FILE_BLOCK_SIZE);

         if (FileBytesRead == 0) /* Successfully finished reading file */ 
         {  
            
            ComputingCrc = false;
            OS_close(FileHandle);

            *Crc = CurrentCrc;
            CrcComputed = true;
           
         }
         else if (FileBytesRead < 0) /* Error reading file */ 
         {  
            
            ComputingCrc = false;
            OS_close(FileHandle);
            
            CFE_EVS_SendEvent(FILE_COMPUTE_FILE_CRC_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Concatenate file cmd error: File read error, OS_read status = %d", FileBytesRead);

         }
         else
         {
                
            CurrentCrc = CFE_ES_CalculateCRC(File->FileTaskBuf, FileBytesRead,
                                             CurrentCrc, CrcType);
         
            CHILDMGR_PauseTask(&TaskBlockCnt, INITBL_GetIntConfig(File->IniTbl, CFG_TASK_FILE_BLOCK_CNT),
                               INITBL_GetIntConfig(File->IniTbl, CFG_TASK_FILE_BLOCK_DELAY), 
                               INITBL_GetIntConfig(File->IniTbl, CFG_CHILD_TASK_PERF_ID));
         
         } /* End if still reading file */

      } /* End while computing CRC */
   
   }
   else
   {
      OS_GetErrorName(SysStatus, &OsErrStr);
      CFE_EVS_SendEvent(FILE_COMPUTE_FILE_CRC_ERR_EID, CFE_EVS_EventType_ERROR,
                        "%s failed: Error opening file %s, OS_open status %s", CmdName, Filename, OsErrStr);
   
   } /* End if file open */
   
   return CrcComputed;
   
} /* End ComputeCrc() */


/******************************************************************************
** Function: ConcatenateFiles
**
*/
static bool ConcatenateFiles(const char *SrcFile1, const char *SrcFile2, const char *TargetFile)
{
   
   int32         SysStatus;
   os_err_name_t OsErrStr;
   osal_id_t     SourceFileHandle;
   osal_id_t     TargetFileHandle;
   int32         BytesRead;
   int32         BytesWritten;
      
   char    EventErrStr[256] = "\0";
   uint16  TaskBlockCnt = 0;
   bool    PerformingCatenation = true;
   bool    ConcatenatedFiles    = false;
  
   sprintf(EventErrStr,"Unhandled concatenate file error");
  
   SysStatus = OS_cp(SrcFile1, TargetFile);
   if (SysStatus == OS_SUCCESS) {
   
      SysStatus = OS_OpenCreate(&SourceFileHandle, SrcFile2, OS_FILE_FLAG_NONE, OS_READ_ONLY);
      
      if (SysStatus == OS_SUCCESS)
      {
   
         SysStatus = OS_OpenCreate(&TargetFileHandle, SrcFile2, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_READ_WRITE);
   
         if (SysStatus == OS_SUCCESS)
         {
         
            OS_lseek(TargetFileHandle, 0, OS_SEEK_END);
        
            while (PerformingCatenation)
            {
               
               BytesRead = OS_read(SourceFileHandle, File->FileTaskBuf, FILE_MGR_TASK_FILE_BLOCK_SIZE);
               if (BytesRead == 0)
               {
               
                  PerformingCatenation = false;
                  ConcatenatedFiles    = true;
               
               }
               else if (BytesRead < 0)
               {
                  
                  PerformingCatenation = false;

                  sprintf(EventErrStr,"Concatenate file cmd error: File read error. OS_read status = %d", BytesRead);

               }
               else
               {
                  
                  BytesWritten = OS_write(TargetFileHandle, File->FileTaskBuf, BytesRead);

                  if (BytesWritten != BytesRead)
                  {

                     PerformingCatenation = false;
                     sprintf(EventErrStr,"Concatenate file cmd error: File read/write error. BytesRead %d  not equal to BytesWritten %d",
                             BytesRead, BytesWritten);
                  
                  }
           
                  CHILDMGR_PauseTask(&TaskBlockCnt, INITBL_GetIntConfig(File->IniTbl, CFG_TASK_FILE_BLOCK_CNT), 
                                     INITBL_GetIntConfig(File->IniTbl, CFG_TASK_FILE_BLOCK_DELAY),
                                     INITBL_GetIntConfig(File->IniTbl, CFG_CHILD_TASK_PERF_ID));
               
               }
               
            } /* End while performing concatenation */

            OS_close(TargetFileHandle);
            
         } /* End if opened target file */
         else
         {
            OS_GetErrorName(SysStatus,&OsErrStr);         
            sprintf(EventErrStr,"Concatenate file cmd error: Error opening target file %s. Open status %s", SrcFile2, OsErrStr);
            
         } /* End if  failed to open target file */
         
         OS_close(SourceFileHandle);
         
      } /* End if opened source file */
      else
      {
         OS_GetErrorName(SysStatus, &OsErrStr);
         sprintf(EventErrStr,"Concatenate file cmd error: Error opening source file %s. Open status %s", SrcFile2, OsErrStr);
         
      } /* End if  failed to open source file */
  
      if (ConcatenatedFiles == false) OS_remove(TargetFile);  /* remove partial target file */
 
   } /* End if copied first source file to target */
   else
   {
      OS_GetErrorName(SysStatus, &OsErrStr);
      sprintf(EventErrStr,"Concatenate file cmd error: Error copying first source file to target file. OS_cp status %s", OsErrStr);
      
   } /* End if failed to copy first source file to target */

   if (ConcatenatedFiles == false) CFE_EVS_SendEvent(FILE_CONCATENATE_ERR_EID, CFE_EVS_EventType_ERROR,"%s",EventErrStr);
   
   return ConcatenatedFiles;

} /* End of ConcatenateFiles() */



