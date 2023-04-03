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
**    Implement the "File Outut Transfer Protocol" (FOTP)
**
**  Notes:
**    1. See fotp.h file prologue for protocol overview and functions
**       below for protocol details.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
*/


/*
** Include Files:
*/

#include "fotp.h"

typedef enum
{

  SEND_DATA_SEGMENT_ACTIVE = 1,
  SEND_DATA_SEGMENT_FINISHED,
  SEND_DATA_SEGMENT_ABORTED

} SendDataSegmentState_t;

/*******************************/
/** Local Function Prototypes **/
/*******************************/

static void DestructorCallback(void);
const char* FileTransferStateStr(FOTP_FileTransferState_t  FileTransferState);
static SendDataSegmentState_t SendDataSegments(void);
static bool SendFileTransferTlm(FOTP_FileTransferState_t FileTransferState);
static bool StartTransfer(const FILE_XFER_StartFotp_Payload_t *StartTransferCmd);


/**********************/
/** Global File Data **/
/**********************/

static FOTP_Class_t* Fotp = NULL;
static uint8 DataSegBuf[FITP_DATA_SEG_MAX_LEN];

/******************************************************************************
** Function: FOTP_Constructor
**
*/
void FOTP_Constructor(FOTP_Class_t *FotpPtr, INITBL_Class_t *IniTbl)
{

   Fotp = FotpPtr;

   /* All booleans and counters set to zero. States have non-zero defaults and must be explicitly set */ 
   CFE_PSP_MemSet((void*)Fotp, 0, sizeof(FOTP_Class_t));
  
   Fotp->IniTbl = IniTbl;
   Fotp->FileTransferState = FOTP_IDLE;
   Fotp->PausedFileTransferState = FOTP_IDLE;
   
   FOTP_ResetStatus();

   CFE_MSG_Init(CFE_MSG_PTR(Fotp->StartTransferPkt.TelemetryHeader), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(Fotp->IniTbl, CFG_FILE_XFER_FOTP_START_TRANSFER_TLM_TOPICID)),
                sizeof(FILE_XFER_StartFotpTlm_t));

   CFE_MSG_Init(CFE_MSG_PTR(Fotp->DataSegmentPkt.TelemetryHeader),
               CFE_SB_ValueToMsgId(INITBL_GetIntConfig(Fotp->IniTbl, CFG_FILE_XFER_FOTP_DATA_SEGMENT_TLM_TOPICID)),
               sizeof(FILE_XFER_FotpDataSegmentTlm_t));

   CFE_MSG_Init(CFE_MSG_PTR(Fotp->FinishTransferPkt.TelemetryHeader),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(Fotp->IniTbl, CFG_FILE_XFER_FOTP_FINISH_TRANSFER_TLM_TOPICID)),
                sizeof(FILE_XFER_FinishFotpTlm_t));

   OS_TaskInstallDeleteHandler(DestructorCallback); /* Called when application terminates */

} /* End FOTP_Constructor() */

 
/******************************************************************************
** Function: FOTP_CancelTransferCmd
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
**   2. Receiving a cancel command without a transfer in progress is not
**      considered an error. A cancel command may be sent in the blind.
*/
bool FOTP_CancelTransferCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = true;
   
   if (Fotp->FileTransferState != FOTP_IDLE)
   {
      OS_close(Fotp->FileHandle);
      Fotp->FileTransferState = FOTP_IDLE;
      
      CFE_EVS_SendEvent(FOTP_CANCEL_TRANSFER_CMD_EID, CFE_EVS_EventType_INFORMATION,
                       "Cancelled file transfer for %s before sending segment %d",
                       Fotp->SrcFilename, Fotp->NextDataSegmentId);

   }
   else 
   {
      CFE_EVS_SendEvent(FOTP_CANCEL_TRANSFER_CMD_EID, CFE_EVS_EventType_INFORMATION,
                        "Cancel transfer received with no transfer in progress");
   }
   
   return RetStatus;

} /* End FOTP_CancelTransferCmd() */


/******************************************************************************
** Function: FOTP_Execute
**
** Notes:
**   1. After initialization, Fotp->FileTransferState is only set in the this
**      function and the FOTP_xxxTransferCmd() functions.
*/
void FOTP_Execute(void)
{
   SendDataSegmentState_t SendDataSegmentState;
   
   CFE_EVS_SendEvent(FOTP_EXECUTE_EID, CFE_EVS_EventType_DEBUG,
                     "Executing state %s",
                     FileTransferStateStr(Fotp->FileTransferState));
                     
   switch (Fotp->FileTransferState)
   {   
        
      case FOTP_START:
         if (SendFileTransferTlm(FOTP_START))
         {
            Fotp->FileTransferState = FOTP_SEND_DATA;
         }
         break;
         
      case FOTP_SEND_DATA:
         SendDataSegmentState = SendDataSegments();
         if (SendDataSegmentState == SEND_DATA_SEGMENT_FINISHED)
         {
            Fotp->FileTransferState = FOTP_FINISH;
         }
         else if (SendDataSegmentState == SEND_DATA_SEGMENT_ABORTED)
         {
            Fotp->FileTransferState = FOTP_IDLE;
         }
         break;
         
      case FOTP_FINISH:
         if (SendFileTransferTlm(FOTP_FINISH))
         {
            Fotp->FileTransferCnt++;
            Fotp->FileTransferState = FOTP_IDLE;
            CFE_EVS_SendEvent(FOTP_EXECUTE_EID, CFE_EVS_EventType_INFORMATION,
                              "Completed %d byte file transfer of %s",
                              Fotp->FileTransferByteCnt,
                              Fotp->SrcFilename);
         }
         break;
         
      default:
         break;   

   } /* End state switch */
   
} /* End FOTP_Execute() */
 
 
/******************************************************************************
** Function: FOTP_PauseTransferCmd
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
*/
bool FOTP_PauseTransferCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool RetStatus = false;

   if (Fotp->FileTransferState == FOTP_IDLE)
   {
      CFE_EVS_SendEvent(FOTP_PAUSE_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Pause file transfer received while no transfer is in progress");
   }
   else
   {
      RetStatus = true;
      Fotp->PausedFileTransferState = Fotp->FileTransferState;
      Fotp->FileTransferState       = FOTP_PAUSED;
      CFE_EVS_SendEvent(FOTP_PAUSE_TRANSFER_CMD_EID, CFE_EVS_EventType_INFORMATION,
                        "Paused file transfer in %s state with next data segment ID %d",
                        FileTransferStateStr(Fotp->PausedFileTransferState),
                        Fotp->NextDataSegmentId);
   }

   return RetStatus;

} /* End FOTP_PauseTransferCmd() */


/******************************************************************************
** Function:  FOTP_ResetStatus
**
*/
void FOTP_ResetStatus(void)
{

   if (Fotp->FileTransferState == FOTP_IDLE)
   {

      Fotp->FileTransferByteCnt = 0;
      Fotp->DataTransferLen     = 0;
      Fotp->DataSegmentLen      = 0;
      Fotp->DataSegmentOffset   = 0;
      Fotp->FileLen             = 0;
      Fotp->FileByteOffset      = 0;
      Fotp->FileRunningCrc      = 0;
      Fotp->NextDataSegmentId   = FOTP_DATA_SEGMENT_ID_START;
      Fotp->FileTransferCnt     = 0;
      Fotp->LastDataSegment     = false;
      Fotp->PrevSendDataSegmentFailed = 0;
      Fotp->PausedFileTransferState   = FOTP_IDLE;
      strcpy(Fotp->SrcFilename, FILE_XFER_UNDEF_TLM_STR);

   } /* End if not idle */
      
} /* End FOTP_ResetStatus() */


/******************************************************************************
** Function: FOTP_ResumeTransferCmd
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
*/
bool FOTP_ResumeTransferCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool RetStatus = false;

   if (Fotp->FileTransferState == FOTP_IDLE)
   {
      CFE_EVS_SendEvent(FOTP_PAUSE_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                        "Resume file transfer received while no transfer is in progress");
   }
   else
   {
      RetStatus = true;
      Fotp->FileTransferState = Fotp->PausedFileTransferState;
      CFE_EVS_SendEvent(FOTP_PAUSE_TRANSFER_CMD_EID, CFE_EVS_EventType_INFORMATION,
                        "Resumed file transfer for %s with next data segment ID %d",
                         Fotp->SrcFilename, Fotp->NextDataSegmentId);
   }
   
   return RetStatus;

} /* End FOTP_ResumeTransferCmd() */


/******************************************************************************
** Function: FOTP_StartBinTransferCmd
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
**   2. All command parameters are validated prior to updating and FOTP state
**      data.
*/
bool FOTP_StartBinTransferCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const FILE_XFER_StartFotp_Payload_t *StartTransferCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_XFER_StartFotp_t);   
   
   Fotp->BinFile = true;   
   return StartTransfer(StartTransferCmd);
   
} /* FOTP_StartBinTransferCmd() */


/******************************************************************************
** Function: FOTP_StartTransferCmd
**
** Notes:
**   1. Must match CMDMGR_CmdFuncPtr_t function signature
**   2. All command parameters are validated prior to updating and FOTP state
**      data.
*/
bool FOTP_StartTransferCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const FILE_XFER_StartFotp_Payload_t *StartTransferCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, FILE_XFER_StartFotp_t);   
   
   Fotp->BinFile = false;
   return StartTransfer(StartTransferCmd);
   
} /* FOTP_StartTransferCmd() */


/******************************************************************************
** Function: DestructorCallback
**
** This function is called when the app is terminated. This should
** never occur but if it does this will close an open file. 
*/
static void DestructorCallback(void)
{
 
   if (Fotp->FileTransferState != FOTP_IDLE)
   {
      OS_close(Fotp->FileHandle);
   }
   
} /* End DestructorCallback() */


/******************************************************************************
** Function: FileTransferStateStr
**
** Type checking should enforce valid parameter but check just to be safe.
*/
const char *FileTransferStateStr(FOTP_FileTransferState_t  FileTransferState)
{

   static const char *TransferStateStr[] = {
      FILE_XFER_UNDEF_TLM_STR, 
      "Idle",       /* FOTP_IDLE      */
      "Start",      /* FOTP_START     */
      "Send Data",  /* FOTP_SEND_DATA */
      "Finished",   /* FOTP_FINISH    */
      "Paused"      /*  FOTP_PAUSED   */
   };

   uint8 i = 0;
   
   if ( FileTransferState >= FOTP_IDLE &&
        FileTransferState <= FOTP_PAUSED)
   {
      i = FileTransferState;
   }
        
   return TransferStateStr[i];

} /* End FileTransferStateStr() */


/******************************************************************************
** Function: SendDataSegments
**
** Notes:
**   1. Event messages are issued for error cases. The app should configure
**      filters for these events so they don't flood telemetry. It's helpful
**      to get one message that contains information about the error situation.
**   2. NextDataSegmentId, FileTransferByteCnt, and FileRunningCrc are based on
**      successful telemetry packet send. If the send fails then the stats are
**      not updated.
**   TODO - Replace DataSegmentsSent logic with a TO semaphore handshake
*/
static SendDataSegmentState_t SendDataSegments(void)
{
   
   uint16  DataSegmentsSent = 0;
   uint16  FileBytesRead;
   uint32  RemainingBytes;
   bool    ContinueSend = true;
   bool    CloseFile = false;
   SendDataSegmentState_t SendDataSegmentState = SEND_DATA_SEGMENT_ACTIVE;
   const uint8 *CrcBufPtr;

   if (!Fotp->PrevSendDataSegmentFailed)
   {
      while (ContinueSend)
      {
         
         Fotp->DataSegmentPkt.Payload.Id = Fotp->NextDataSegmentId;
         
         RemainingBytes = Fotp->FileLen - Fotp->FileTransferByteCnt;
         if (RemainingBytes <= Fotp->DataSegmentLen)
         {
            Fotp->DataSegmentPkt.Payload.Len = RemainingBytes;
            Fotp->LastDataSegment = true;
         }
         else
         {
            Fotp->DataSegmentPkt.Payload.Len = Fotp->DataSegmentLen;
         }
         
         memset(Fotp->DataSegmentPkt.Payload.Data, 0, FOTP_DATA_SEG_MAX_LEN);
         if (Fotp->BinFile)
         {
            FileBytesRead = OS_read(Fotp->FileHandle, DataSegBuf, Fotp->DataSegmentPkt.Payload.Len);
            PktUtil_HexEncode(Fotp->DataSegmentPkt.Payload.Data, DataSegBuf, Fotp->DataSegmentPkt.Payload.Len, false);
            CrcBufPtr = (const uint8 *)DataSegBuf;
         }
         else
         {
            FileBytesRead = OS_read(Fotp->FileHandle, Fotp->DataSegmentPkt.Payload.Data, Fotp->DataSegmentPkt.Payload.Len);
            CrcBufPtr = (const uint8 *)Fotp->DataSegmentPkt.Payload.Data;
         }
         if (FileBytesRead == Fotp->DataSegmentPkt.Payload.Len)
         {
            Fotp->FileRunningCrc = CRC_32c(Fotp->FileRunningCrc, CrcBufPtr, FileBytesRead);
            //TODO - Always send full packet: CFE_SB_SetUserDataLength((CFE_MSG_Message_t *)&Fotp->DataSegmentPkt, (FOTP_DATA_SEGMENT_NON_DATA_TLM_LEN + Fotp->DataSegmentPkt.Payload.Len));
          
            if (SendFileTransferTlm(FOTP_SEND_DATA))
            {
               Fotp->PrevSendDataSegmentFailed = false;
               Fotp->NextDataSegmentId++;
               Fotp->FileTransferByteCnt += Fotp->DataSegmentPkt.Payload.Len;
               if (Fotp->LastDataSegment)
               {
                  CloseFile    = true;
                  ContinueSend = false;
                  SendDataSegmentState = SEND_DATA_SEGMENT_FINISHED;
               }
               else
               {
                  DataSegmentsSent++;
                  if (DataSegmentsSent >= 1) ContinueSend = false; //TODO - Add flow control logic
               }
            }/* End if sent data segment telemetry */
            else
            {
               Fotp->PrevSendDataSegmentFailed = true;
               ContinueSend = false;
            }
            
         } /* End if successful file read */
         else
         {
           CloseFile = true;
           ContinueSend = false;
           SendDataSegmentState = SEND_DATA_SEGMENT_ABORTED;
           CFE_EVS_SendEvent(FOTP_SEND_DATA_SEGMENT_ERR_EID, CFE_EVS_EventType_ERROR, 
                             "File transfer aborted: Error reading data from file %s. Attempted %d bytes, read %d",
                             Fotp->SrcFilename, Fotp->DataSegmentPkt.Payload.Len, FileBytesRead);
         }
         
      } /* End while send DataSegment */
   } /* End if PrevSendDataSegmentFailed */
   else
   {
      /* 
      ** When in an error state, only attempt a single send. If SB sends are
      ** not working then there are probably much bigger issues. 
      ** This repeats some of the logic in the send loop above which is not
      ** desirable but the logic is short and a function seems cumbersome at
      ** best.     
      */
      
      if (SendFileTransferTlm(FOTP_SEND_DATA))
      {
         
         Fotp->PrevSendDataSegmentFailed = false;
         Fotp->NextDataSegmentId++;
         Fotp->FileTransferByteCnt += Fotp->DataSegmentPkt.Payload.Len;
         if (Fotp->LastDataSegment)
         {
            CloseFile = true;
            SendDataSegmentState = SEND_DATA_SEGMENT_FINISHED;
         }

      }/* End if sent data egment telemetry */
      else
      {
         Fotp->PrevSendDataSegmentFailed = true;
      }
      
   } /* End if previous data segment failed */
   
   if (CloseFile)
   {
      OS_close(Fotp->FileHandle);   
   }
   
   return SendDataSegmentState;
   
} /* SendDataSegments() */


/******************************************************************************
** Function: SendFileTransferTlm
**
** Notes:
**   1. Sends a telemetry packet unique to each file transfer state. The START
**      and FINISH telemetry data is loaded in this function. The SEND_DATA
**      telemetry data is loaded by the calling function. 
**   2. This should only be called in a state that should send a message. If
**      no message is sent (wrong state or SB send failure) then it returns
**      false.
**   3. Event FOTP_SEND_FILE_TRANSFER_ERR_EID should be filtered even though
**      it should never occur. 
*/
static bool SendFileTransferTlm(FOTP_FileTransferState_t FileTransferState)
{
   
   CFE_MSG_TelemetryHeader_t *TlmHeader = NULL;
   int32 SbStatus;
   bool  RetStatus = false;
   
   switch (Fotp->FileTransferState)
   {   
      case FOTP_START:
         Fotp->StartTransferPkt.Payload.BinFile = Fotp->BinFile;
         Fotp->StartTransferPkt.Payload.DataLen = Fotp->DataTransferLen;
         strncpy(Fotp->StartTransferPkt.Payload.SrcFilename, Fotp->SrcFilename, FOTP_FILENAME_LEN);
         TlmHeader = &Fotp->StartTransferPkt.TelemetryHeader;
         break;
         
      case FOTP_SEND_DATA:
         /* Segment ID is loaded prior to this call */
         TlmHeader = &Fotp->DataSegmentPkt.TelemetryHeader;
         break;
         
      case FOTP_FINISH:
         Fotp->FinishTransferPkt.Payload.FileLen           = Fotp->FileLen;
         Fotp->FinishTransferPkt.Payload.FileCrc           = Fotp->FileRunningCrc;
         Fotp->FinishTransferPkt.Payload.LastDataSegmentId = Fotp->NextDataSegmentId-1;
         TlmHeader = &Fotp->FinishTransferPkt.TelemetryHeader;
         break;
         
      default:
         break;   

   } /* End state switch */
   
   if (TlmHeader != NULL)
   {
   
      CFE_SB_TimeStampMsg(CFE_MSG_PTR(*TlmHeader));
      SbStatus = CFE_SB_TransmitMsg(CFE_MSG_PTR(*TlmHeader), true);
   
      if (SbStatus == CFE_SUCCESS)
      {
         RetStatus = true;
      }
      else
      {
         CFE_EVS_SendEvent(FOTP_SEND_FILE_TRANSFER_ERR_EID, CFE_EVS_EventType_ERROR,
                           "Error sending telemetry packet in the %s state",
                           FileTransferStateStr(Fotp->FileTransferState));
      }
   } /* End if have a message to send */
   
   return RetStatus;
   
} /* End SendFileTransferTlm() */


/******************************************************************************
** Function: StartTransfer
**
** Notes:
**   1. All command parameters are validated prior to updating and FOTP state
**      data.
*/
static bool StartTransfer(const FILE_XFER_StartFotp_Payload_t *StartTransferCmd)
{
   
   FileUtil_FileInfo_t  FileInfo;
   os_err_name_t        OsErrStr;
   int32   OsStatus;
   uint16  i;
   uint32  FileByteOffset=0;
   uint16  DataSegmentReadLen;
   uint8   DataSegment[FOTP_DATA_SEG_MAX_LEN];
   bool    ValidCmdParams = false;
   bool    RetStatus = false;
   
   OS_printf("FOTP_StartTransferCmd: %s, DataSegLen %d, DataSegOffset %d\n", StartTransferCmd->SrcFilename, StartTransferCmd->DataSegLen, StartTransferCmd->DataSegOffset);
   if (Fotp->FileTransferState == FOTP_IDLE)
   {
            
      /* FileUtil_GetFileInfo() validates the filename */
      FileInfo = FileUtil_GetFileInfo(StartTransferCmd->SrcFilename, FOTP_FILENAME_LEN, true);
      if (FILEUTIL_FILE_EXISTS(FileInfo.State) && FileInfo.State == FILEUTIL_FILE_CLOSED)
      {
         
         if ((StartTransferCmd->DataSegLen >= FOTP_DATA_SEG_MIN_LEN) &&
             (StartTransferCmd->DataSegLen <= FOTP_DATA_SEG_MAX_LEN) )
         {
            FileByteOffset = StartTransferCmd->DataSegLen * StartTransferCmd->DataSegOffset;
            if (FileByteOffset < FileInfo.Size)
            {
               ValidCmdParams = true;   
            }
            else
            {
               CFE_EVS_SendEvent(FOTP_START_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                                 "Start transfer command rejected: File byte offset %d (seg len*offset %d*%d) not less than file length %d.",
                                 FileByteOffset, StartTransferCmd->DataSegLen,
                                 StartTransferCmd->DataSegOffset, FileInfo.Size);
            }
         } /* End if valid DataSegLen */
         else
         {
            CFE_EVS_SendEvent(FOTP_START_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                              "Start transfer command rejected: Invalid data segement length %d. It must be between %d and %d",
                              StartTransferCmd->DataSegLen, FOTP_DATA_SEG_MIN_LEN, FOTP_DATA_SEG_MAX_LEN);
         }
         
         if (ValidCmdParams)
         {

            OsStatus = OS_OpenCreate(&Fotp->FileHandle, StartTransferCmd->SrcFilename, OS_FILE_FLAG_NONE, OS_READ_ONLY);
            
            if (OsStatus == OS_SUCCESS)
            {      
            
               strncpy(Fotp->SrcFilename, StartTransferCmd->SrcFilename, FOTP_FILENAME_LEN);            
   
               // Fotp->BinFile has been set by caller
               Fotp->DataSegmentLen      = StartTransferCmd->DataSegLen;                  
               Fotp->DataSegmentOffset   = StartTransferCmd->DataSegOffset;
               Fotp->DataTransferLen     = FileInfo.Size - FileByteOffset;
               Fotp->FileByteOffset      = FileByteOffset;
               Fotp->FileLen             = FileInfo.Size;
               Fotp->NextDataSegmentId   = FOTP_DATA_SEGMENT_ID_START;
               Fotp->FileTransferByteCnt = 0;
               Fotp->FileRunningCrc      = 0;
               Fotp->FileTransferState   = FOTP_IDLE;
               Fotp->LastDataSegment     = false;
               Fotp->PrevSendDataSegmentFailed = false;
               
               for (i=0; i < StartTransferCmd->DataSegOffset; i++)
               {
                  DataSegmentReadLen = OS_read(Fotp->FileHandle, DataSegment, Fotp->DataSegmentLen);
                  if (DataSegmentReadLen == Fotp->DataSegmentLen)
                  {
                     Fotp->FileTransferByteCnt += DataSegmentReadLen;
                     Fotp->FileRunningCrc = CRC_32c(Fotp->FileRunningCrc, DataSegment, DataSegmentReadLen);
                  }
                  else
                  {
                     CFE_EVS_SendEvent(FOTP_START_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                                       "Start transfer command rejected: Error advancing to file offset at segment %d. Read %d bytes, expected %d",
                                       i, DataSegmentReadLen, Fotp->DataSegmentLen);
                     break;
                  }                  
               }
OS_printf("i=%d, StartTransferCmd->DataSegOffset=%d\n",i,StartTransferCmd->DataSegOffset);               
               if (i == StartTransferCmd->DataSegOffset)
               {
                  Fotp->FileTransferState = FOTP_START;
                  RetStatus = true;
                  CFE_EVS_SendEvent(FOTP_START_TRANSFER_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                                    "Start file transfer command accepted for %s, Segment length %d and offset %d",
                                    Fotp->SrcFilename, StartTransferCmd->DataSegLen, StartTransferCmd->DataSegOffset);
               }
               else
               {
                  OS_close(Fotp->FileHandle);                
               }
         
            } /* End if file opened */
            else
            {

               OS_GetErrorName(OsStatus, &OsErrStr);
               CFE_EVS_SendEvent(FOTP_START_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                              "Start transfer command rejected: Open %s failed, status = %s",
                              StartTransferCmd->SrcFilename, OsErrStr);
                                 
            }
            
         } /* End if valid command parameters */
         
      } /* End if file exists and closed */
      else
      {
         
         CFE_EVS_SendEvent(FOTP_START_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                           "Start transfer command rejected: %s is either an invalid filename or the file is open",
                           StartTransferCmd->SrcFilename);

      }
      
   }/* End if idle */
   else
   {
      CFE_EVS_SendEvent(FOTP_START_TRANSFER_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Start transfer command rejected: %s transfer in progress",
                        Fotp->SrcFilename);
   }

   return RetStatus;
   
} /* StartTransfer() */
