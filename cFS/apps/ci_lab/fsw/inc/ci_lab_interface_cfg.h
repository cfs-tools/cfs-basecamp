/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
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
 *   CI_LAB Application Public Definitions
 *
 * This provides default values for configurable items that affect
 * the interface(s) of this module.  This includes the CMD/TLM message
 * interface, tables definitions, and any other data products that
 * serve to exchange information with other entities.
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef CI_LAB_INTERFACE_CFG_H
#define CI_LAB_INTERFACE_CFG_H

#include "ci_lab_interface_cfg_values.h"

/**
 * @brief The base UDP port where CI_LAB will listen for incoming messages
 *
 * In order to allow multiple instances of CFE to run on the same host, the
 * processor number - 1 is added to this value.  This, if this is set to
 * "1234",  then the following ports will be used at runtime:
 *
 * Processor 1: port 1234
 * Processor 2: port 1235
 * Processor 3: port 1236
 *
 * And so forth for however many processor numbers exist in the system
 */
#define CI_LAB_MISSION_BASE_UDP_PORT         CI_LAB_MISSION_CFGVAL(BASE_UDP_PORT)
#define DEFAULT_CI_LAB_MISSION_BASE_UDP_PORT 1234

#endif
