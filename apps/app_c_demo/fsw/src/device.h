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
**    Supply simulated device data
**
**  Notes:
**    1. This is for demonstration purposes and is part of the OSK
**       code-as-you-go (CAYG) tutorial.
**
*/

#ifndef _device_
#define _device_

/*
** Includes
*/

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define DEVICE_RANDOM_DATA_EID  (DEVICE_BASE_EID + 0)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** DEVICE_Class
*/

typedef struct
{

   /*
   ** App Framework References
   */
   

   /*
   ** Class State Data
   */

   uint16 Data;
   uint16 DataCnt;
   uint16 DataMod;
   
} DEVICE_Class_t;



/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function:DEVICE_Constructor
**
** Initialize the packet log to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void DEVICE_Constructor(DEVICE_Class_t *DevicePtr, uint16 DataMod);


/******************************************************************************
** Function: DEVICE_ReadData
**
** Notes:
**   None
**
*/
uint16 DEVICE_ReadData(void);


/******************************************************************************
** Function: DEVICE_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void DEVICE_ResetStatus(void);


#endif /* _device_ */
