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
*************************************************************************/

/**
 * @file
 *
 * Define Sample App Message IDs
 *
 * \note The Sample App assumes default configuration which uses V1 of message id implementation
 */

#ifndef @TEMPLATE@_MSGIDS_H
#define @TEMPLATE@_MSGIDS_H

#include "cfe_msgids.h"

#define @TEMPLATE@_CMD_MID     CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_@TEMPLATE@_CMD_TOPICID)
#define @TEMPLATE@_SEND_HK_MID CFE_PLATFORM_CMD_TOPICID_TO_MID(CFE_MISSION_BC_SCH_4_SEC_TOPICID)
#define @TEMPLATE@_HK_TLM_MID  CFE_PLATFORM_TLM_TOPICID_TO_MID(CFE_MISSION_@TEMPLATE@_HK_TLM_TOPICID)

#endif /* @TEMPLATE@_MSGIDS_H */
