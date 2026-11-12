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
**    Provide general file management utilties
**
**  Notes:
**    None
**
*/

/*
** Includes
*/

#include <ctype.h>
#include <string.h>

#include "cfe.h"
#include "fileutil.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define EVENT_ERR_STR_LEN 132


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static void CountOpenFiles(osal_id_t ObjId, void *CallbackArg);
static void LoadOpenFileData(osal_id_t ObjId, void *CallbackArg);
static CFE_ES_TaskId_t TaskId_FromOSAL(osal_id_t id);
static bool ValidPathFilename(const char *PathFilename, uint32 Length, char *EventErrStr);


/**********************/
/** Global File Data **/
/**********************/

static const char InvalidFileNameChars[] = {"`!@#$^&+=\0"};


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: FileUtil_AppendPathSep
**
** Append a path separator to a directory path. 
** 
** Returns false if invalid string length or appending the separator would
** exceed the BufferLen.
**
*/
bool FileUtil_AppendPathSep(char *DirName, uint16 BufferLen)
{

   uint16 StringLen;
   bool   RetStatus = false;
   
   StringLen = strlen(DirName);

   if (( StringLen > 0) && (StringLen < BufferLen))
   {

      if (DirName[StringLen - 1] != FILEUTIL_DIR_SEP_CHAR)
      {
         
         if (StringLen < BufferLen-1)
         {
         
            strcat(DirName, FILEUTIL_DIR_SEP_STR);
            RetStatus = true;        
   
         }      
      } /* End if no path separator */
      else {
          RetStatus = true;
      }
      
   } /* End if valid string length */

   return RetStatus;
   
} /* End FileUtil_AppendPathSep() */


/******************************************************************************
** Function: FileUtil_FileStateStr
**
** Type checking should enforce valid parameter but check just to be safe.
*/
const char *FileUtil_FileStateStr(APP_C_FW_FileState_Enum_t  FileState)
{

   static const char *FileStateStr[] = 
   {
      "Undefined", 
      "Invalid Filename",    /* APP_C_FW_FileState_FILENAME_INVALID */
      "Nonexistent File",    /* APP_C_FW_FileState_FILE_NONEXISTENT */
      "File Open",           /* APP_C_FW_FileState_FILE_OPEN        */
      "File Closed",         /* APP_C_FW_FileState_FILE_CLOSED      */
      "File is a Directory"  /* APP_C_FW_FileState_FILE_IS_DIR      */
   };

   uint8 i = 0;
   
   if ( FileState >= APP_C_FW_FileState_FILENAME_INVALID &&
        FileState <= APP_C_FW_FileState_FILE_IS_DIR)
   {
   
      i =  FileState;
   
   }
        
   return FileStateStr[i];

} /* End FileUtil_FileStateStr() */


/******************************************************************************
** Function: FileUtil_GetFileInfo
**
** Return file state (FileUtil_FileInfo_t) and optionally include the file size
** and time for existing files.
*/
FileUtil_FileInfo_t FileUtil_GetFileInfo(const char *Filename, uint16 FilenameBufLen, bool IncludeSizeTime)
{
   
   os_fstat_t                FileStatus;
   FileUtil_FileInfo_t       FileInfo;
    
   FileInfo.IncludeSizeTime = IncludeSizeTime;
   FileInfo.Size  = 0;
   FileInfo.Time  = 0;
   FileInfo.Mode  = 0;
   FileInfo.State = APP_C_FW_FileState_FILENAME_INVALID;

   /* TODO - Fix all file utilities to accept a length parameter with a OS_MAX_PATH_LEN check */
   if (FilenameBufLen != OS_MAX_PATH_LEN)
   {
      CFE_EVS_SendEvent(FILEUTIL_MAX_PATH_LEN_CONFLICT_EID, CFE_EVS_EventType_ERROR, 
         "FileUtil_GetFileInfo() checking a filename buffer len=%d using a utility hard coded with OS_MAX_PATH_LEN=%d",
         FilenameBufLen, OS_MAX_PATH_LEN);
   }

   if (FileUtil_VerifyFilenameStr(Filename))
   {
      
      /* Check to see if Filename exists */
      if (OS_stat(Filename, &FileStatus) == OS_SUCCESS)
      {
         
         FileInfo.Mode = OS_FILESTAT_MODE(FileStatus);
         
         if (OS_FILESTAT_ISDIR(FileStatus))
         {
            
            FileInfo.State = APP_C_FW_FileState_FILE_IS_DIR;
         
         }
         else
         {

            FileInfo.State = APP_C_FW_FileState_FILE_CLOSED;
            if (OS_FileOpenCheck(Filename) == OS_SUCCESS)
            {
               FileInfo.State = APP_C_FW_FileState_FILE_OPEN;
            }

            if (IncludeSizeTime)
            {
               
               FileInfo.Size  = OS_FILESTAT_SIZE(FileStatus);
               FileInfo.Time  = OS_FILESTAT_TIME(FileStatus);

            }
         }
         
      } /* End if file exists */
      else
      {
         
         FileInfo.State = APP_C_FW_FileState_FILE_NONEXISTENT;

      } /* End if file doesn't exist */
      
      
   } /* End if valid filename */

   return (FileInfo);

} /* End FileUtil_GetFileInfo() */


/******************************************************************************
** Function: FileUtil_GetFilenameLen
**
** Notes:
**  1. Does not issue an event message which can be useful in cases when a zero
**     length string is not considered an error. FileUtil_VerifyFilenameStr()
**     can be called for non-zero string lengths. 
*/
uint16 FileUtil_GetFilenameLen(const char *Filename)
{

   int16  Len = 0;

   /* Search file system name buffer for a string terminator */
   while (Len < OS_MAX_PATH_LEN)
   {
      if (Filename[Len] == '\0') break;
      Len++;
   }
   
   return Len;

} /* End FileUtil_GetFilenameLen() */


/******************************************************************************
** Function: FileUtil_GetOpenFileCount
**
*/

uint16 FileUtil_GetOpenFileCount(void)
{
    uint16 OpenFileCount = 0;
    
    OS_ForEachObject(OS_OBJECT_CREATOR_ANY, CountOpenFiles, (void*)&OpenFileCount);

    return OpenFileCount;

} /* End FileUtil_GetOpenFileCount() */


/******************************************************************************
** Function: FileUtil_GetOpenFileList
**
** Type checking should enforce valid parameter but check just to be safe.
*/

uint16 FileUtil_GetOpenFileList(APP_C_FW_OpenFileList_t *OpenFileList)
{
    OpenFileList->Count = 0;
    
    OS_ForEachObject(OS_OBJECT_CREATOR_ANY, LoadOpenFileData, (void*)OpenFileList);

    return (OpenFileList->Count);

} /* End FileUtil_GetOpenFileList() */


/******************************************************************************
** Function: FileUtil_ReadLine
**
** Read a line from a text file.
**
*/
bool FileUtil_ReadLine (int FileHandle, char *DestBuf, int MaxChar) 
{

   char    c, *DestPtr;
   int32   ReadStatus;
   bool    RetStatus = false;
   
   /* Decrement MaxChar to leave space for termination character */
   for (DestPtr = DestBuf, MaxChar--; MaxChar > 0; MaxChar--)
   {
      
      ReadStatus = OS_read(FileHandle, &c, 1);

      if (ReadStatus == 0  || ReadStatus == OS_ERROR)
         break;
      
      *DestPtr++ = c;
      
      if (c == '\n')
      {
         RetStatus = true;
         break;
      }

   } /* End for loop */
   
   *DestPtr = 0;

   return RetStatus;
   
} /* End FileUtil_ReadLine() */


/******************************************************************************
** Function: FileUtil_VerifyDirForWrite
**
** Notes:
**  1. Verify file name is valid and that the directory exists.
*/
bool FileUtil_VerifyDirForWrite(const char *Filename)
{

   bool RetStatus = false;
   
   if (FileUtil_VerifyFilenameStr(Filename))
   {
      
      /* TODO - Find last \ and check if directory */
      RetStatus = true;  
      
   } /* End if valid filename */
   
  return RetStatus;

} /* End FileUtil_VerifyDirForWrite() */


/******************************************************************************
** Function: FileUtil_VerifyFileForRead
**
** Notes:
**  1. Verify file name is valid and that the file exists for a read operation.
**  2. The file is opened/closed to make sure it's valid for a read operation.
**     The file descriptor is not returned to the caller function because there
**     are scenarios when the user must stil open the file.  For example when
**     they pass the filename to a third party library. 
*/
bool FileUtil_VerifyFileForRead(const char *Filename)
{

   bool       RetStatus = false;
   osal_id_t  FileHandle;
   int32      OsStatus;
   os_err_name_t OsErrStr;
      
   if (FileUtil_VerifyFilenameStr(Filename))
   {
      
      OsStatus = OS_OpenCreate(&FileHandle, Filename, OS_FILE_FLAG_NONE, OS_READ_ONLY);
      if (OsStatus == OS_SUCCESS)
      {   
         OS_close (FileHandle);
         RetStatus = true;  
      }
      else
      {   
         OS_GetErrorName(OsStatus, &OsErrStr);
         CFE_EVS_SendEvent(FILEUTIL_FILE_READ_OPEN_ERR_EID, CFE_EVS_EventType_ERROR, 
                           "Read file open failed for %s. Status = %s", Filename, OsErrStr);
      }
      
   } /* End if valid filename */
   
  return RetStatus;

} /* End FileUtil_VerifyFileForRead() */


/******************************************************************************
** Function: FileUtil_VerifyFilenameStr
**
** Notes:
**  1. Verifies the filename is properly terminated, length of path/filename 
**     length of filename length, and allowable filename characters
*/
bool FileUtil_VerifyFilenameStr(const char *Filename)
{

   int16   Len;
   bool    RetStatus=false;
   char    EventErrStr[EVENT_ERR_STR_LEN];
 
   Len = FileUtil_GetFilenameLen(Filename);
   if (Len == 0)
   {
      CFE_EVS_SendEvent(FILEUTIL_INVALID_FILENAME_EID, CFE_EVS_EventType_ERROR,
                        "Invalid path/filename: Length is 0");
   } 
   else if (Len == OS_MAX_PATH_LEN)
   {
      CFE_EVS_SendEvent(FILEUTIL_INVALID_FILENAME_EID, CFE_EVS_EventType_ERROR, 
                        "Invalid path/filename: No NUL termination character within max len of %d",OS_MAX_PATH_LEN);     
   }
   else
   {
      
      if (ValidPathFilename(Filename, Len, EventErrStr))
      {  
         RetStatus = true;  
      }
      else
      {
         CFE_EVS_SendEvent(FILEUTIL_INVALID_FILENAME_EID, CFE_EVS_EventType_ERROR,
                           "Invalid path/filename: %s", EventErrStr);     
      }

   } /* End if valid length */
   
   return RetStatus;

} /* End FileUtil_VerifyFilenameStr() */


/******************************************************************************
** Function: CountOpenFiles
**
** Notes:
**  1. Callback function for OS_ForEachObject()
*/
static void CountOpenFiles(osal_id_t ObjId, void *CallbackArg)
{

   uint16* OpenFileCount = (uint16*)CallbackArg;

   if (OS_IdentifyObject(ObjId) == OS_OBJECT_TYPE_OS_STREAM)
   {
      (*OpenFileCount)++;
   }

   return;

} /* End CountOpenFiles() */


/******************************************************************************
** Function: LoadOpenFileData
**
** Notes:
**  1. Callback function for OS_ForEachObject()
*/
static void LoadOpenFileData(osal_id_t ObjId, void *CallbackArg)
{

   APP_C_FW_OpenFileList_t  *OpenFileList = (APP_C_FW_OpenFileList_t*)CallbackArg;
   CFE_ES_TaskInfo_t  TaskInfo;
   OS_file_prop_t     FdProp;


   if (OS_IdentifyObject(ObjId) == OS_OBJECT_TYPE_OS_STREAM)
   {
   
      if (OpenFileList != (APP_C_FW_OpenFileList_t*) NULL) 
      {
         
         if (OS_FDGetInfo(ObjId, &FdProp) == OS_SUCCESS)
         {
            
            if (OpenFileList->Count < OS_MAX_NUM_OPEN_FILES)
            {
               strncpy(OpenFileList->Entry[OpenFileList->Count].Filename,
                       FdProp.Path, OS_MAX_PATH_LEN);

               // Get the name of the application that opened the file
               memset(&TaskInfo, 0, sizeof(CFE_ES_TaskInfo_t));

               if (CFE_ES_GetTaskInfo(&TaskInfo, TaskId_FromOSAL(FdProp.User)) == CFE_SUCCESS)
               {
                  strncpy(OpenFileList->Entry[OpenFileList->Count].AppName,
                          (char*)TaskInfo.AppName, OS_MAX_API_NAME);
               }
            
               ++OpenFileList->Count;
            }
            else
            {
               CFE_EVS_SendEvent(FILEUTIL_FILE_READ_OPEN_ERR_EID, CFE_EVS_EventType_INFORMATION, 
                                 "Load open file data reached maximum open files of %d", OS_MAX_NUM_OPEN_FILES);
            }
            
         } /* End OS_FDGetInfo() */
      } /* End if valid OpenFileList arg */
   } /* End if OS_OBJECT_TYPE_OS_STREAM */

   return;

} /* End LoadOpenFileData() */


/******************************************************************************
** Function: TaskId_FromOSAL
**
** Notes:
**  1. Copied from private function CFE_ES_TaskId_FromOSAL(osal_id_t id). Not
**     define in local function block at top of file because this is only used
**     below and should be removed ASAP. 
**  2. TODO - Create long term solution for TaskId_FromOSAL()
*/
static CFE_ES_TaskId_t TaskId_FromOSAL(osal_id_t id)
{
    CFE_ResourceId_t Result;
    unsigned long    Val;

    Val    = OS_ObjectIdToInteger(id);
    Result = CFE_ResourceId_FromInteger(Val ^ CFE_RESOURCEID_MARK);

    return CFE_ES_TASKID_C(Result);

} /* End TaskId_FromOSAL() */


/******************************************************************************
** Function: ValidPathFilename
**
** Notes:
**  1. 
**
*/
static bool ValidPathFilename(const char *PathFilename, uint32 Length, char *EventErrStr)
{
   
   bool        Valid = true;
   int32       i;
   const char  *StrPtr;
   
   /* Test for zero length */
   if (PathFilename[0] == '\0')
   { 
      Valid = false;
   }
   else
   {
      
      /* checks to see if there is a '/' somewhere in the path */
      StrPtr = strrchr(PathFilename, FILEUTIL_DIR_SEP_CHAR);
      if (StrPtr == NULL)
      {
         sprintf(EventErrStr,"Path does not contain directory separator character '%s'",FILEUTIL_DIR_SEP_STR);
         return false;
      }

      /* strrchr returns a pointer to the last '/' char, so we advance one char */
      StrPtr += 1;
      if (memchr(StrPtr, 0, OS_MAX_FILE_NAME) == NULL)
      {
         sprintf(EventErrStr,"Filename length (excluding path & including NUL char) exceeds max filename length %d",OS_MAX_FILE_NAME);
         return false;
      }
      
      /*
      ** Scan string for disallowed/discouraged characters
      ** - This doesn't guarantee a well formed path/filename but
      **   enforces naming conventions
      */
      for (i=0; i < Length; i++)
      {
         if ( !isalnum((int)PathFilename[i]) && 
              strchr(InvalidFileNameChars, PathFilename[i]) != NULL)
         {
            sprintf(EventErrStr,"Invalid character %c at filename position %d", PathFilename[i], i);
            Valid = false;
            break;
         }
      
      } /* End for */
   
   } /* End if not null */

   return (Valid);
   
} /* End ValidPathFilename */
