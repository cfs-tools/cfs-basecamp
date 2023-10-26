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
**    Define the EXOBJ_Class 
**
**  Notes:
**    None
**
*/
#ifndef _exobj_
#define _exobj_

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

#define EXOBJ_SET_MODE_CMD_EID (EXOBJ_BASE_EID + 0)
#define EXOBJ_EXECUTE_EID      (EXOBJ_BASE_EID + 1)
//EX1
/*
** Counter Limits
*/

//#define EXOBJ_COUNTER_HI_LIM  100
//#define EXOBJ_COUNTER_LO_LIM    0
//EX1
/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Command Packets
** - See EDS command definitions in @template@.xml
*/


/******************************************************************************
** EXOBJ_Class
*/

typedef struct
{
   //EX2
   /*
   ** State Data
   */
   
   @TEMPLATE@_CounterMode_Enum_t CounterMode;
   uint16  CounterValue;
   
   uint16  CounterLoLim;
   uint16  CounterHiLim;
   //EX2
       
   /*
   ** Contained Objects
   */

   
} EXOBJ_Class_t;



/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: EXOBJ_Constructor
**
** Initialize the example object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void EXOBJ_Constructor(EXOBJ_Class_t *ExObjPtr,
                       const INITBL_Class_t *IniTbl);


/******************************************************************************
** Function: EXOBJ_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void EXOBJ_ResetStatus(void);


/******************************************************************************
** Function: EXOBJ_SetModeCmd
**
*/
bool EXOBJ_SetModeCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: EXOBJ_Execute
**
** Perform periodic processing that is trigger by an 'execute' request message
** from the scheduler app.
*/
void EXOBJ_Execute(void);


#endif /* _exobj_ */
