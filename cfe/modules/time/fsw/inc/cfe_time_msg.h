/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/**
 * @file
 *
 * Purpose:  cFE Time Services (TIME) SB message definitions header file
 *
 * Author:   S.Walling/Microtel
 *
 * Notes:
 *
 */

#ifndef CFE_TIME_MSG_H
#define CFE_TIME_MSG_H

/*
** Required header files...
*/
#include "common_types.h" /* Basic data types */
#include "cfe_msg_hdr.h"  /* for header definitions */
#include "cfe_time_extern_typedefs.h"

/*
 * EDS-defined function codes (*_CC)
 */
#include "cfe_time_eds_cc.h"

/*
 * In some circumstances the EDS tool does not generate a symbol
 * name identically to the historical name due to naming inconsistencies
 *
 * For those cases, create a local define from the historic name to the EDS name
 */
#define CFE_TIME_SEND_DIAGNOSTIC_TLM_CC CFE_TIME_SEND_DIAGNOSTIC_CC

/** \defgroup CFETIMEClkStates cFE Clock State Flag Defines
 * \{
 */

/**
 * \brief The spacecraft time has been set
 */
#define CFE_TIME_FLAG_CLKSET (0x8000 >> CFE_TIME_FlagBit_CLKSET)
/**
 * \brief This instance of Time Services is flywheeling
 */
#define CFE_TIME_FLAG_FLYING (0x8000 >> CFE_TIME_FlagBit_FLYING)
/**
 * \brief The clock source is set to "internal"
 */
#define CFE_TIME_FLAG_SRCINT (0x8000 >> CFE_TIME_FlagBit_SRCINT)
/**
 * \brief The clock signal is set to "primary"
 */
#define CFE_TIME_FLAG_SIGPRI (0x8000 >> CFE_TIME_FlagBit_SIGPRI)
/**
 * \brief The Time Server is in flywheel mode
 */
#define CFE_TIME_FLAG_SRVFLY (0x8000 >> CFE_TIME_FlagBit_SRVFLY)
/**
 * \brief This instance of Time Services was commanded into flywheel mode
 */
#define CFE_TIME_FLAG_CMDFLY (0x8000 >> CFE_TIME_FlagBit_CMDFLY)
/**
 * \brief One time STCF Adjustment is to be done in positive direction
 */
#define CFE_TIME_FLAG_ADDADJ (0x8000 >> CFE_TIME_FlagBit_ADDADJ)
/**
 * \brief 1 Hz STCF Adjustment is to be done in a positive direction
 */
#define CFE_TIME_FLAG_ADD1HZ (0x8000 >> CFE_TIME_FlagBit_ADD1HZ)
/**
 * \brief Time Client Latency is applied in a positive direction
 */
#define CFE_TIME_FLAG_ADDTCL (0x8000 >> CFE_TIME_FlagBit_ADDTCL)
/**
 * \brief This instance of Time Services is a Time Server
 */
#define CFE_TIME_FLAG_SERVER (0x8000 >> CFE_TIME_FlagBit_SERVER)
/**
 * \brief The tone received is good compared to the last tone received
 */
#define CFE_TIME_FLAG_GDTONE (0x8000 >> CFE_TIME_FlagBit_GDTONE)
/**
 * \brief GetReference read error, will be set if unable to get a consistent ref value
 */
#define CFE_TIME_FLAG_REFERR (0x8000 >> CFE_TIME_FlagBit_REFERR)

#define CFE_TIME_FLAG_UNUSED ((0x8000 >> CFE_TIME_FlagBit_Enum_t_MAX) - 1)

/** \} */

#endif /* CFE_TIME_MSG_H */
