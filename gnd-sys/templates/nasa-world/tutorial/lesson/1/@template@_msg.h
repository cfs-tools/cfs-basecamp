//EX1
/*
** There are no code changes in this exercise. The point of the exercise
** is to make you aware of the following:
**
**   1. This file is replaced by code generated from the EDS file
**         @template@.xml 
**
**   2. The @template@_app.h includes the EDS generated header files
**         @template@_eds_cc.h
**         @template@_eds_typedefs.h
**
*/
//EX1
/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *
 * Define SAMPLE App  Messages and info
 */

#ifndef @TEMPLATE@_MSG_H
#define @TEMPLATE@_MSG_H


/*
** @TEMPLATE@ command codes
*/
#define @TEMPLATE@_NOOP_CC           0
#define @TEMPLATE@_RESET_COUNTERS_CC 1
#define @TEMPLATE@_EXAMPLE_PARAM_CC  2

/*************************************************************************/

/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader; /**< \brief Command header */
} @TEMPLATE@_NoArgsCmd_t;

/*
** The following commands all share the "NoArgs" format
**
** They are each given their own type name matching the command name, which
** allows them to change independently in the future without changing the prototype
** of the handler function
*/
typedef @TEMPLATE@_NoArgsCmd_t @TEMPLATE@_NoopCmd_t;
typedef @TEMPLATE@_NoArgsCmd_t @TEMPLATE@_ResetCountersCmd_t;

/*************************************************************************/
/*
** Type definition (SAMPLE App housekeeping)
*/

typedef struct
{
    uint8 CommandCounter;
    uint8 CommandErrorCounter;
    uint8 spare[2];
} @TEMPLATE@_HkTlm_Payload_t;

typedef struct
{
    CFE_MSG_TelemetryHeader_t  TelemetryHeader; /**< \brief Telemetry header */
    @TEMPLATE@_HkTlm_Payload_t Payload;         /**< \brief Telemetry payload */
} @TEMPLATE@_HkTlm_t;

#endif /* @TEMPLATE@_MSG_H */
