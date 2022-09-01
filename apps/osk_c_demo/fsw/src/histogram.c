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
**    Implement the HISTOGRAM_Class methods
**
**  Notes:
**    1. This is for demonstration purposes and is part of the OSK
**       code-as-you-go (CAYG) tutorial.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "histogram.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macro */
#define LOG_OBJ (&(Histogram->Log))  
#define TBL_OBJ (&(Histogram->Tbl))  


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static void AcceptNewTbl(void);
static void CreateBinCntStr(void);


/**********************/
/** Global File Data **/
/**********************/

static HISTOGRAM_Class_t*  Histogram = NULL;


/******************************************************************************
** Function: HISTOGRAM_Constructor
**      
** Notes:
**   1. HISTOGRAM_TBL_Constructor() must be constructed prior to 
**      TBLMGR_RegisterTblWithDef() because its table load function is called
*/
void HISTOGRAM_Constructor(HISTOGRAM_Class_t *HistogramPtr, 
                           INITBL_Class_t *IniTbl,
                           TBLMGR_Class_t *TblMgr)
{
 
   Histogram = HistogramPtr;

   CFE_PSP_MemSet((void*)Histogram, 0, sizeof(HISTOGRAM_Class_t));
 
   Histogram->DataSampleMaxValue = INITBL_GetIntConfig(IniTbl, CFG_DEVICE_DATA_MODULO)-1;
   strcpy(Histogram->BinCntStr, OSK_C_DEMO_UNDEF_TLM_STR);
   
   HISTOGRAM_LOG_Constructor(LOG_OBJ, IniTbl);
   
   HISTOGRAM_TBL_Constructor(&Histogram->Tbl, AcceptNewTbl,
                             INITBL_GetStrConfig(IniTbl, CFG_APP_CFE_NAME));

   TBLMGR_RegisterTblWithDef(TblMgr, HISTOGRAM_TBL_LoadCmd, 
                             HISTOGRAM_TBL_DumpCmd,  
                             INITBL_GetStrConfig(IniTbl, CFG_HIST_TBL_LOAD_FILE));
                             
} /* End HISTOGRAM_Constructor */


/******************************************************************************
** Function: HISTOGRAM_AddDataSample
**
** Notes:
**   None
**
*/
bool HISTOGRAM_AddDataSample(uint16 DataSample, uint16 *BinNum)
{

   uint8 i;
   bool  RetStatus = false;
   
   
   if (Histogram->Ena)
   {
      if (DataSample <= Histogram->DataSampleMaxValue)
      {
         for (i=0; i < Histogram->Tbl.Data.BinCnt; i++)
         {
            if (DataSample >= Histogram->Tbl.Data.Bin[i].LoLim &&
                DataSample <= Histogram->Tbl.Data.Bin[i].HiLim)
            {
               Histogram->Bin[i]++;
               Histogram->SampleCnt++;
               *BinNum = i;
               CreateBinCntStr();
               RetStatus = true;
            }  
         }
      }
      else
      {
         CFE_EVS_SendEvent (HISTOGRAM_DATA_SAMPLE_EID, CFE_EVS_EventType_ERROR, 
                            "Data sample %d exceeds maximum value %d", 
                            DataSample, Histogram->DataSampleMaxValue);
      }
   } /* End if enabled */

   return RetStatus;

} /* End HISTOGRAM_AddDataSample() */


/******************************************************************************
** Function:  HISTOGRAM_ResetStatus
**
*/
void HISTOGRAM_ResetStatus()
{
 
   HISTOGRAM_LOG_ResetStatus();
   HISTOGRAM_TBL_ResetStatus();
   
   
} /* End HISTOGRAM_ResetStatus() */


/******************************************************************************
** Function: HISTOGRAM_StartCmd
**
** Notes:
**   1. No command parameters
**   2. If a start command is received when a histogram is in progress the
**      command will serve as a reset. It is not considered an error.
*/
bool HISTOGRAM_StartCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool RetStatus = true;

   if (Histogram->Ena)
   {
       CFE_EVS_SendEvent (HISTOGRAM_START_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                          "Started histogram with %d bins", Histogram->Tbl.Data.BinCnt);
   }
   else
   {
       CFE_EVS_SendEvent (HISTOGRAM_START_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                          "Started histogram with %d bins", Histogram->Tbl.Data.BinCnt);
       Histogram->Ena = true;
   }
   Histogram->SampleCnt = 0;
   memset(Histogram->Bin, 0, sizeof(Histogram->Bin));
  
   return RetStatus;

} /* End HISTOGRAM_StartCmd() */


/******************************************************************************
** Function: HISTOGRAM_StopCmd
**
** Notes:
**   1. No command parameters
**   2. Receiving a stop command when the histogram is not actve is not 
**      considered an error. A stop command could be issued as part of an 
**      onboard command sequence.  
*/
bool HISTOGRAM_StopCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool RetStatus = true;

   if (Histogram->Ena)
   {
      CFE_EVS_SendEvent (HISTOGRAM_STOP_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                          "Stop histogram command received after %d data samples processed",
                          Histogram->SampleCnt);
      Histogram->Ena = false;
   }
   else
   {
      CFE_EVS_SendEvent (HISTOGRAM_STOP_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                         "Stop histogram command received when histogram disabled");
   }

   return RetStatus;

} /* End HISTOGRAM_StopCmd() */


/******************************************************************************
** Function: AcceptNewTbl
**
** Notes:
**   1. This is a HISTOGRAM_TBL table load callback function and must match the
**      HISTOGRAM_TBL_LoadFunc_t definition.
*/
static void AcceptNewTbl(void)
{

   if (Histogram->Ena)
   {
      CFE_EVS_SendEvent (HISTOGRAM_ACCEPT_NEW_TBL_EID, CFE_EVS_EventType_INFORMATION, 
                         "Histogram disabled after new table loaded. %d data samples processed.",
                          Histogram->SampleCnt);
      Histogram->Ena = false;
   }
      

} /* End AcceptNewTbl() */


/******************************************************************************
** Function: CreateBinCntStr
**
** Notes:
**   1. Create a comma separated string of the bin values
**   2. Should only be called if table has been loaded
*/
static void CreateBinCntStr(void)
{

   uint8 i;
   char  BinCntStr[16];
   
   sprintf(Histogram->BinCntStr, "%d", Histogram->Bin[0]);
   
   for (i=1; i < Histogram->Tbl.Data.BinCnt; i++)
   {
       sprintf(BinCntStr, ",%d", Histogram->Bin[i]);
       strcat(Histogram->BinCntStr, BinCntStr);
   }

} /* End CreateBinCntStr() */
