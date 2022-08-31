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
**    Define a class that provides a mechanism for objects to report boolean
**    states represented by a single bit that are aggregated into an app state
**    report packet.
**
**  Notes:
**    1. This code was originally acquired from the NASA cFS External Code
**       Interface (ECI) NOSA release and that was named FaultRep because it
**       was exicitly used to report faults using one bit for each fault
**       which made it very easy to configure Limit Checker to logically
**       combine faults. For OSK it was generalized to be a boolean state
**       reporter and it remains functionally very similar.
**    2. This code must be reentrant and no global data can be used. 
**    3. STATEREP_Constructor() must be called prior to any other STATEREP_ 
**       functions
**    4. The telemetry generation requires that an even number of state
**       16-bit state words be defined.
**    5. This object does not associate any meaning to the state bit IDs.
**    6. Typically multiple state definition points are "ORed" together to
**       an meta-state especially when states represents faults. For example,
**       if fault A or fault B occur then take corrective action X. This
**       utility assumes the combining of state definition points is 
**       performed by the received of this utility's telemetry packet.
**    7. The ReportMode flag has the following definitions
**       - STATEREP_NEW_REPORT - The ID notifications for an app's current
**         execution cycle are copied into the message.
**       - STATEREP_MERGE_REPORT - The ID notifications for an app's current
**         execution cycle are merged(logically ORed) with the message
**    8. STATEREP_BIT_ID_MAX must be defined prior to including this header.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide.
**    2. cFS Application Developer's Guide.
**
*/

#ifndef  _staterep_
#define  _staterep_

/*
** Include Files
*/

#include "osk_c_fw_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Service 
*/

#define  STATEREP_INVALID_ID_EID      (STATEREP_BASE_EID + 0)
#define  STATEREP_CONFIG_CMD_ERR_EID  (STATEREP_BASE_EID + 1)



/*
** The following macros define all possible state definition points. Applications
** must use these definitions when using package functions that require a
** StateId.
*/

#define  STATEREP_SELECT_ALL  0xFFFF  /* Used by functions to select all IDs */
                                      /* Must not be a valid state ID        */

/*
** Define constants that are based on the total number of state points:
**
** - STATEREP_ID_MAX         = Total number of IDs (1..LIMIT). This must
**                             be a multiple of STATEREP_BITS_PER_WORD.
**
** - STATEREP_BITFIELD_WORDS = Number of words that are used to hold
**                             bit information
*/

#define STATEREP_BITS_PER_WORD    16
#define STATEREP_BITFIELD_WORDS   (STATEREP_BIT_ID_MAX/STATEREP_BITS_PER_WORD)

#if (STATEREP_BIT_ID_MAX % STATEREP_BITS_PER_WORD) != 0
   #error STATEREP_BIT_ID_MAX must be a multiple of STATEREP_BITS_PER_WORD
#endif

/**********************/
/** Type Definitions **/
/**********************/


/*
** Report types used by STATEREP_SendTlmMsg()
*/

typedef enum
{

   STATEREP_NEW_REPORT   = 1,  /* Only report new state since last report    */
   STATEREP_MERGE_REPORT = 2   /* Boolean OR new states with previous report */

} STATEREP_TlmMode_t;



/*
** Report packet typically monitored by another Application
*/

typedef struct
{

   uint16  Word[STATEREP_BITFIELD_WORDS];  /* Bit packed status */

} STATEREP_Bits_t;


typedef struct
{

   CFE_MSG_TelemetryHeader_t TlmHeader;
   STATEREP_Bits_t  Bits;

} STATEREP_TlmMsg_t;
#define STATEREP_TLM_PKT_LEN  sizeof(STATEREP_TlmMsg_t)


/******************************************************************************
** Command Messages
*/

typedef struct
{

   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16   Id;        /* Single identifier: 0..(STATEREP_BIT_ID_MAX-1) or STATEREP_SELECT_ALL */

} STATEREP_ClearBitCmdMsg_t;
#define STATEREP_CLEAR_BIT_CMD_DATA_LEN  (sizeof(STATEREP_ClearBitCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))


typedef struct
{

   CFE_MSG_CommandHeader_t  CmdHeader;
   uint16   Id;           /* Single identifier: 0..(STATEREP_BIT_ID_MAX-1) or STATEREP_SELECT_ALL */
   uint16   Enable;       /* TRUE - Enable an ID, FALSE - Disable an ID (keep word aligned)       */

} STATEREP_ConfigBitCmdMsg_t;
#define STATEREP_CONFIG_BIT_CMD_DATA_LEN  (sizeof(STATEREP_ConfigBitCmdMsg_t) - sizeof(CFE_MSG_CommandHeader_t))



/*
** Data structures for the State Reporter Status
**
** - A single bit is used for each state definition point for the enable/disable
**   configuration and for latched status.
**
** - A latched bit is set when an enabled state ID notifies StateRep of 
**   a true state. The bit remains set until a command is received to clear the bit.
**
*/

typedef struct
{

   uint16   IdLimit;
   
   uint16   BitfieldWords;
   uint16   BitfieldRemMask;

   uint16   Enabled[STATEREP_BITFIELD_WORDS];   /* 0 = Disabled, 1 = Enabled */
   uint16   Latched[STATEREP_BITFIELD_WORDS];   /* 0 = Never set to 1(true), 1 = Set to 1 since last cleared */

} STATEREP_BitConfig_t;


/*
** State Reporter Class Definition
*/

typedef struct
{

   STATEREP_TlmMode_t    TlmMode;
   STATEREP_BitConfig_t  BitConfig;
   STATEREP_Bits_t       CurrBits;   /* Collected between SendTlmMsg() calls */
   STATEREP_TlmMsg_t     TlmMsg;     /* Last TLM message sent                */

} STATEREP_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: STATEREP_Constructor
**
** Purpose:  Initialize a StateRep object.
**
** Notes:
**   1. This function must be called prior to any other STATEREP_ functions
**   2. The StateRep object is initialized to a known state. If the
**      StateRep state should be preserved across some event then
**      the object managing this object should control when the
**      constructor routine is called. The telemetry reporting mode
**      default is STATEREP_NEW_REPORT.
**
*/
void STATEREP_Constructor(STATEREP_Class_t*  StateRep, 
                          uint16             IdCnt);      /* Number of state definition IDs used (not an index, but a count) */
                             


/******************************************************************************
** Function: STATEREP_ClearBitCmd
**
** Purpose: Clear the latched status flag and the status flag in the report
**          packet for a single state bit or for all of the state bits.
**
** Note:
**   1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
**   2. If the command message parameter Id is set to STATEREP_SELECT_ALL
**      then all state IDs are affected otherwise it's interpreted as a
**      single ID.
**
** Return:
**   TRUE  - Command accepted purpose achieved.
**   FALSE - Command rejected: An event message is issued describing the
**           cause of the failure.
*/
bool STATEREP_ClearBitCmd(                  void  *ObjDataPtr,  /* Pointer to an instance of a STATEREP_Class */
                          const CFE_MSG_Message_t *MsgPtr);     /* Pointer to STATEREP_ClearBitCmd struct     */
                                 


/******************************************************************************
** Function: STATEREP_ConfigBitCmd
**
** Purpose:  Configure a state definition bit to be enabled or disabled.
**
** Note:
**   1. This function must comply with the CMDMGR_CmdFuncPtr_t definition
**   2. If the command parameter bit ID is set to STATEREP_SELECT_ALL then
**      all bit IDs are affected otherwise it's interpreted as a  single ID.
**   3. The EnableFlag is defined as:
**         TRUE  - Enable the state bit
**         FALSE - Disable the state bit
**
** Return:
**   TRUE  - Command accepted purpose achieved.
**   FALSE - Command rejected: An event message is issued describing the
**           cause of the failure.
*/
bool STATEREP_ConfigBitCmd(                void    *ObjDataPtr,  /* Pointer to an instance of a STATEREP_Class */
                           const CFE_MSG_Message_t *MsgPtr);     /* Pointer to STATEREP_ConfigBitCmd struct    */
                                      

/******************************************************************************
** Function: STATEREP_GenTlmMsg
**
** Purpose: Update the state report telemetry message.
**
** Notes:
**   1. This function MUST only be called once for each Application control
**      cycle. It MUST be called after all state definition points have
**      executed and prior to ground command processing.
**   2. An Applicaton can use STATEREP_SetTlmMode to change the behavior 
**      of this function. See STATEREP_SetTlmMode prologue.
*/
void STATEREP_GenTlmMsg(STATEREP_Class_t  *StateRep,
                        STATEREP_TlmMsg_t *TlmMsg);
                           
                           
/******************************************************************************
** Function: STATEREP_SetBit
**
** Set a state identifier bit to 1
**
** Notes:
**   1. Limit checking is performed on the Id but this type of error should
**      only occur during integration.
**
*/
void STATEREP_SetBit(STATEREP_Class_t *StateRep,
                     uint16            Id);  /* Integer identifier (not a bit bit mask) */



/******************************************************************************
** Function: STATEREP_SetTlmMode
**
** Set the telemetry reporting mode.
**
** Notes:
**   1. If the source of a state bit couldn't not execute during the current
**      state reporting cycle then you should consider setting ReportMode to
**      STATEREP_MERGE_REPORT which allows consecutive counters in an external
**      monitoring application to continue to count for a consecutive state.
**      This is most useful when a state represents a fault so the fault can
**      be assumed to persist if the source that reported the fault loses
**      communication.
**
*/
void STATEREP_SetTlmMode(STATEREP_Class_t   *StateRep,
                         STATEREP_TlmMode_t TlmMode);


/******************************************************************************
** Function: STATEREP_TlmModeStr
**
** Purpose: Return a pointer to a string describing the enumerated type
**
** Notes:
**   None
*/
const char* STATEREP_TlmModeStr(STATEREP_TlmMode_t  TlmMode);


#endif  /* _staterep_ */
