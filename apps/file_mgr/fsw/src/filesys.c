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
**    Implement the File System Table
**
**  Notes:
**    1. Refactored from NASA's FM FreeSpace table.
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

static int32 ValidateTbl(void* TblPtr);


/**********************/
/** File Global Data **/
/**********************/

static FILESYS_Class_t*  FileSys = NULL;


/******************************************************************************
** Function: FILESYS_Constructor
**
*/
void FILESYS_Constructor(FILESYS_Class_t *FileSysPtr, const INITBL_Class_t *IniTbl)
{
 
   FileSys = FileSysPtr;
   const char* DefTblFilename = INITBL_GetStrConfig(IniTbl, CFG_TBL_DEF_FILENAME);

   CFE_PSP_MemSet((void*)FileSys, 0, sizeof(FILESYS_Class_t));
   
   FileSys->IniTbl         = IniTbl;
   FileSys->CfeTbl.DataPtr = (FILESYS_TblData_t *) NULL;
   FileSys->CfeTblName     = INITBL_GetStrConfig(FileSys->IniTbl, CFG_TBL_CFE_NAME);

   FileSys->CfeTbl.Status = CFE_TBL_Register(&FileSys->CfeTbl.Handle, FileSys->CfeTblName,
                                             sizeof(FILESYS_TblData_t), CFE_TBL_OPT_DEFAULT, 
                                             (CFE_TBL_CallbackFuncPtr_t)ValidateTbl);
    
   FileSys->CfeTbl.Registered = (FileSys->CfeTbl.Status == CFE_SUCCESS);
   
   /* DataPtr will remain NULL if data not loaded. */
   if (FileSys->CfeTbl.Registered)
   {
   
      FileSys->CfeTbl.Status = CFE_TBL_Load(FileSys->CfeTbl.Handle, CFE_TBL_SRC_FILE, DefTblFilename);
      if (FileSys->CfeTbl.Status == CFE_SUCCESS)
      {
         CFE_TBL_GetAddress((void **)&(FileSys->CfeTbl.DataPtr), FileSys->CfeTbl.Handle);
      }
   }
   else
   {
      
      CFE_EVS_SendEvent(FILESYS_TBL_REGISTER_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Error registering table %s, CFE_TBL_Register() status = 0x%08X",
                        DefTblFilename, FileSys->CfeTbl.Status);                        
   }

   CFE_MSG_Init(CFE_MSG_PTR(FileSys->TblTlm.TelemetryHeader), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(FileSys->IniTbl, CFG_FILE_MGR_FILE_SYS_TBL_TLM_TOPICID)),
                sizeof(FILE_MGR_FileSysTblTlm_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(FileSys->OpenFileTlm.TelemetryHeader),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(FileSys->IniTbl, CFG_FILE_MGR_OPEN_FILE_TLM_TOPICID)),
                sizeof(FILE_MGR_OpenFileTlm_t));

} /* End FILESYS_Constructor() */


/******************************************************************************
** Function: FILESYS_ManageTbl
**
*/
void FILESYS_ManageTbl(void)
{

   if (FileSys->CfeTbl.Registered)
   {

      FileSys->CfeTbl.Status = CFE_TBL_ReleaseAddress(FileSys->CfeTbl.Handle);
      
      CFE_TBL_Manage(FileSys->CfeTbl.Handle);
      
      FileSys->CfeTbl.Status = CFE_TBL_GetAddress((void **)&(FileSys->CfeTbl.DataPtr), FileSys->CfeTbl.Handle);

      if (FileSys->CfeTbl.Status == CFE_TBL_ERR_NEVER_LOADED)
      {

         FileSys->CfeTbl.DataPtr = (FILESYS_TblData_t *) NULL;
      
      }
      
   } /* End if table registered */
   
} /* End FILESYS_ManageTbl() */


/******************************************************************************
** Function:  FILESYSTBL_ResetStatus
**
*/
void FILESYS_ResetStatus()
{

   /* Nothing to do */

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
   FileUtil_OpenFileList_t  OpenFileList;

   /* Don't assume utility will null char strings */
   memset(&FileSys->OpenFileTlm.Payload,0,sizeof(FILE_MGR_OpenFileTlm_Payload_t));
   
   /* EDS is not coupled with FileUtil, so kept it safe an didn't cast */
   FileUtil_GetOpenFileList(&OpenFileList);
   FileSys->OpenFileTlm.Payload.OpenCount = OpenFileList.OpenCount;
   for (i=0; i < FILE_MGR_OS_MAX_NUM_OPEN_FILES; i++)
   {
       strncpy(FileSys->OpenFileTlm.Payload.OpenFile[i].AppName,  OpenFileList.Entry[i].AppName,  OS_MAX_API_NAME);
       strncpy(FileSys->OpenFileTlm.Payload.OpenFile[i].Filename, OpenFileList.Entry[i].Filename, OS_MAX_API_NAME);
   }

   CFE_SB_TimeStampMsg(CFE_MSG_PTR(FileSys->OpenFileTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(FileSys->OpenFileTlm.TelemetryHeader), true);
   
   CFE_EVS_SendEvent(FILESYS_SEND_OPEN_FILES_CMD_EID, CFE_EVS_EventType_DEBUG,
                     "Sent open files telemetry packets with %d file reported as open", 
                     FileSys->OpenFileTlm.Payload.OpenCount);
   
   return true;
   
} /* End FILESYS_SendOpenFileTlmCmd() */


/******************************************************************************
** Function: FILESYS_SendTblTlmCmd
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
*/
bool FILESYS_SendTblTlmCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool    RetStatus = true;
   int32   SysStatus;
   uint16  i;
   uint64  FreeSpace64;
   OS_statvfs_t  FileSysStats;
   
   
   if (FileSys->CfeTbl.DataPtr == (FILESYS_TblData_t *) NULL)
   {
      CFE_EVS_SendEvent(FILESYS_SEND_TLM_ERR_EID, CFE_EVS_EventType_ERROR,
                       "Send %s table packet command error: File system free space table is not loaded",
                       FileSys->CfeTblName);
   }
   else
   {

      memset (FileSys->TblTlm.Payload, 0, sizeof(FileSys->TblTlm.Payload));
      
      for (i=0; i < FILE_MGR_FILESYS_TBL_VOL_CNT; i++)
      {

         if (FileSys->CfeTbl.DataPtr->Volume[i].State == FILESYS_TBL_ENTRY_ENABLED)
         {

            strcpy(FileSys->TblTlm.Payload[i].Name, FileSys->CfeTbl.DataPtr->Volume[i].Name);

            /* Get file system free space */
            FreeSpace64 = 0;
            SysStatus = OS_FileSysStatVolume(FileSys->TblTlm.Payload[i].Name, &FileSysStats);
              
            if (SysStatus == OS_SUCCESS)
            {
             
               /* TODO - Fix this */  
               FreeSpace64 = (uint64)FileSysStats.blocks_free;
               CFE_PSP_MemCpy(&FileSys->TblTlm.Payload[i].FreeSpace_A,
                              &FreeSpace64, sizeof(uint64));
            }
            else
            { 
               CFE_EVS_SendEvent(FILESYS_SEND_TLM_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "Send %s table packet command error: Error retreiving free space data, status=%d",
                                 FileSys->CfeTblName, SysStatus); 
            }
              
         } /* Entry enabled */
      } /* File system loop */

      CFE_SB_TimeStampMsg(CFE_MSG_PTR(FileSys->TblTlm.TelemetryHeader));
      CFE_SB_TransmitMsg(CFE_MSG_PTR(FileSys->TblTlm.TelemetryHeader), true);
      
      CFE_EVS_SendEvent(FILESYS_SEND_TLM_CMD_EID, CFE_EVS_EventType_DEBUG,
                       "Sent %s table telemetry packet", FileSys->CfeTblName);
   
   } /* End if table loaded */

   return RetStatus;


} /* End FILESYS_SendTblTlmCmd() */


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
   uint16 CmdVolumeIndex = SetFileSysTblStateCmd->TblVolumeIndex;
   uint16 CmdVolumeState = SetFileSysTblStateCmd->TblVolumeState;

   if (FileSys->CfeTbl.DataPtr == (FILESYS_TblData_t *) NULL)
   {

      CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_LOAD_ERR_EID, CFE_EVS_EventType_ERROR,
         "Set %s Table State Command Error: File system free space table is not loaded",
         FileSys->CfeTblName);
   
   }
   else if (CmdVolumeIndex >= FILE_MGR_FILESYS_TBL_VOL_CNT)
   {
      
      CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_ARG_ERR_EID, CFE_EVS_EventType_ERROR,
         "Set %s Table State Command Error: Commanded index %d is not in valid range of 0..%d",
         FileSys->CfeTblName, CmdVolumeIndex, (FILE_MGR_FILESYS_TBL_VOL_CNT-1));
        
   }
   else if ((CmdVolumeState != FILESYS_TBL_ENTRY_ENABLED) &&
            (CmdVolumeState != FILESYS_TBL_ENTRY_DISABLED))
   {
               
      CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_ARG_ERR_EID, CFE_EVS_EventType_ERROR,
         "Set %s Table State Command Error: Commanded state %d is not in (%d=Enabled, %d=Disabled)",
         FileSys->CfeTblName, CmdVolumeState, FILESYS_TBL_ENTRY_ENABLED, FILESYS_TBL_ENTRY_DISABLED);

   }
   else if (FileSys->CfeTbl.DataPtr->Volume[CmdVolumeIndex].State == FILESYS_TBL_ENTRY_UNUSED)
   {
      
      CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_UNUSED_ERR_EID, CFE_EVS_EventType_ERROR,
         "Set %s Table State Command Error: Attempt to change state of unused table entry at index %d",
         FileSys->CfeTblName, CmdVolumeIndex);
        
   }
   else
   {

      FileSys->CfeTbl.DataPtr->Volume[CmdVolumeIndex].State = CmdVolumeState;

      CFE_TBL_Modified(FileSys->CfeTbl.Handle);

      CFE_EVS_SendEvent(FILESYS_SET_TBL_STATE_CMD_EID, CFE_EVS_EventType_INFORMATION,
         "Set %s Table State Command: Set table index %d state to %d (%d=Enabled,%d=Disabled)",
         FileSys->CfeTblName, CmdVolumeIndex, CmdVolumeState, FILESYS_TBL_ENTRY_ENABLED, FILESYS_TBL_ENTRY_DISABLED);
   } 

   return RetStatus;

} /* End of FILESYS_SetTableStateCmd() */


/******************************************************************************
** Function: ValidateTbl
**
** Callback function used by CFE Table service to allow a table to be validated
** prior to being committed.
**
** Function signature must match CFE_TBL_CallbackFuncPtr_t
**
*/
static int32 ValidateTbl(void *VoidTblPtr) 
{
   
   FILESYS_TblData_t* Tbl = (FILESYS_TblData_t *) VoidTblPtr;

   int32   RetStatus = INITBL_GetIntConfig(FileSys->IniTbl, CFG_TBL_ERR_CODE);
   uint16  NameLength;
   uint16  i;

   uint16 ValidEntries   = 0;
   uint16 InvalidEntries = 0;
   uint16 UnusedEntries  = 0;

   /*
   ** Verification criteria
   **
   **  1. Table volume state must be enabled or disabled or unused
   **  2. Enabled or disabled entries must have a valid file system name
   **  3. File system name for unused entries is ignored
   **
   ** FileUtil_VerifyFilenameStr() checks for null string. A null string test is 
   ** done first to separate it from an invalid character error. 
   */

   for (i = 0; i < FILE_MGR_FILESYS_TBL_VOL_CNT; i++)
   {

      /* Validate file system name if state is enabled or disabled */
      
      if ((Tbl->Volume[i].State == FILESYS_TBL_ENTRY_ENABLED) ||
          (Tbl->Volume[i].State == FILESYS_TBL_ENTRY_DISABLED))
      {

         /* Search file system name buffer for a string terminator */
         
         for (NameLength = 0; NameLength < OS_MAX_PATH_LEN; NameLength++)
         {

            if (Tbl->Volume[i].Name[NameLength] == '\0') break;
              
         }

         /* 
         ** Valid file system names must be: non-zero length, terminated and valid characters  
         ** Only send event on first error occurence 
         */
         if (NameLength == 0)
         {

            ++InvalidEntries;

            if (InvalidEntries == 1)
            {

               CFE_EVS_SendEvent(FILESYS_TBL_VERIFY_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "%s Table error: index = %d, empty name string",
                                 FileSys->CfeTblName, i);
            }

         }
         else if (NameLength == OS_MAX_PATH_LEN)
         {

            InvalidEntries++;

            if (InvalidEntries == 1) {
                    
               CFE_EVS_SendEvent(FILESYS_TBL_VERIFY_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "%s table error: index = %d, non-terminated name string",
                                 FileSys->CfeTblName, i);
            }
            
         }
         else if (!FileUtil_VerifyFilenameStr(Tbl->Volume[i].Name))
         {

            InvalidEntries++;

            if (InvalidEntries == 1)
            {

               CFE_EVS_SendEvent(FILESYS_TBL_VERIFY_ERR_EID, CFE_EVS_EventType_ERROR,
                                 "%s table error: index = %d, invalid name = %s",
                                 FileSys->CfeTblName, i, Tbl->Volume[i].Name);
            }
         }
         else
         {
            ValidEntries++;

         } /* End NameLength checks */
     
      } /* End ENABLED/DISABLED checcks */ 
      else if (Tbl->Volume[i].State == FILESYS_TBL_ENTRY_UNUSED)
      {

         /* Ignore (but count) unused table entries */
         ++UnusedEntries;
      
      }
      else
      {

         /* Invalid state */
                    
         ++InvalidEntries;   
            
         if (InvalidEntries == 1)
         {

            CFE_EVS_SendEvent(FILESYS_TBL_VERIFY_ERR_EID, CFE_EVS_EventType_ERROR,
                              "%s table error: index = %d, invalid state = %d",
                              FileSys->CfeTblName, i, Tbl->Volume[i].State);
         }

      } /* End state checks **/

   } /* End volume loop */

   CFE_EVS_SendEvent(FILESYS_TBL_VERIFIED_EID, CFE_EVS_EventType_INFORMATION,
                      "%s table entry verification: valid = %d, invalid = %d, unused = %d",
                       FileSys->CfeTblName, ValidEntries, InvalidEntries, UnusedEntries);

   if (InvalidEntries == 0) RetStatus = CFE_SUCCESS;

   return RetStatus;

  
} /* End ValidateTbl() */

