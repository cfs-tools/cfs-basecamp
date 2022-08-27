/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: to_lab_msg.h
**
** Purpose:
**  Define TO Lab Messages and info
**
** Notes:
**
*************************************************************************/
#ifndef _to_lab_msg_h_
#define _to_lab_msg_h_

/*
 * EDS-defined function codes (*_CC)
 */
#include "to_lab_eds_cc.h"

/*
 * EDS-defined message data types
 */
#include "to_lab_eds_typedefs.h"

/*
 * In some circumstances the EDS tool does not generate a symbol
 * name identically to the historical name due to naming inconsistencies
 *
 * For those cases, create a local define from the historic name to the EDS name
 */
#define TO_NOP_CC             TO_LAB_NOOP_CC
#define TO_RESET_STATUS_CC    TO_LAB_RESET_COUNTERS_CC
#define TO_ADD_PKT_CC         TO_LAB_ADD_PACKET_CC
#define TO_SEND_DATA_TYPES_CC TO_LAB_SEND_DATA_TYPES_CC
#define TO_REMOVE_PKT_CC      TO_LAB_REMOVE_PACKET_CC
#define TO_REMOVE_ALL_PKT_CC  TO_LAB_REMOVE_ALL_CC
#define TO_OUTPUT_ENABLE_CC   TO_LAB_ENABLE_OUTPUT_CC

#endif /* _to_lab_msg_h_ */

/************************/
/*  End of File Comment */
/************************/
