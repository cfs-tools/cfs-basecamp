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
**    Implement the DEVICE_Class methods
**
**  Notes:
**    1. The role of the device object is to supply data to the histogram
**       object.
**    2. This is for demonstration purposes and is part of the app
**       code-as-you-go (CAYG) tutorial.
**
*/

/*
** Include Files:
*/

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*******************************/
/** Local Function Prototypes **/
/*******************************/


/**********************/
/** Global File Data **/
/**********************/

static DEVICE_Class_t*  Device = NULL;


/******************************************************************************
** Function: DEVICE_Constructor
**
*/
void DEVICE_Constructor(DEVICE_Class_t *DevicePtr, uint16 DataMod)
{
 
   Device = DevicePtr;

   CFE_PSP_MemSet((void*)Device, 0, sizeof(DEVICE_Class_t));

   Device->DataMod = DataMod;
   
   srand((unsigned int)time(NULL));
 
} /* End DEVICE_Constructor */


/******************************************************************************
** Function:  DEVICE_ReadData
**
** Notes:
**   None
**
*/
uint16 DEVICE_ReadData(void)
{

   Device->Data = rand() % Device->DataMod;
   Device->DataCnt++;
   
   CFE_EVS_SendEvent(DEVICE_RANDOM_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "Device data modulo  %d, count.%d, new value %d",
                      Device->DataMod, Device->DataCnt, Device->Data);

   
   return Device->Data;

} /* End DEVICE_ReadData() */


/******************************************************************************
** Function: DEVICE_ResetStatus
**
*/
void  DEVICE_ResetStatus()
{
 
   Device->DataCnt = 0;
   
} /* End  DEVICE_ResetStatus() */



