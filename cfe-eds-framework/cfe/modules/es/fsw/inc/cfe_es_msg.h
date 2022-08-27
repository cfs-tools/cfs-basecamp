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
 *  Purpose:
 *  cFE Executive Services (ES) Command and Telemetry packet definition file.
 *
 *  References:
 *     Flight Software Branch C Coding Standard Version 1.0a
 *     cFE Flight Software Application Developers Guide
 *
 *  Notes:
 *
 */

#ifndef CFE_ES_MSG_H
#define CFE_ES_MSG_H

/*
** Includes
*/
#include "common_types.h" /* Basic data types */
#include "cfe_msg_hdr.h"  /* for header definitions */
#include "cfe_es_extern_typedefs.h"

/*
 * EDS-defined function codes (*_CC)
 */
#include "cfe_es_eds_cc.h"

/*
 * In some circumstances the EDS tool does not generate a symbol
 * name identically to the historical name due to naming inconsistencies
 *
 * For those cases, create a local define from the historic name to the EDS name
 */
#define CFE_ES_CLEAR_SYSLOG_CC      CFE_ES_CLEAR_SYS_LOG_CC
#define CFE_ES_WRITE_SYSLOG_CC      CFE_ES_WRITE_SYS_LOG_CC
#define CFE_ES_OVER_WRITE_SYSLOG_CC CFE_ES_OVER_WRITE_SYS_LOG_CC

#endif /* CFE_ES_MSG_H */
