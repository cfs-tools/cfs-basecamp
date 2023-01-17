/*
** Purpose: Define platform configurations for the OpenSat Kit Scheduler application
**
** Notes:
**   1. These definitions should be minimal and only contain parameters that
**      need to be configurable and that must be defined at compile time.  
**      Use app_cfg.h for compile-time parameters that don't need to be
**      configured when an app is deployed and the JSON initialization
**      file for parameters that can be defined at runtime.
**   2. v3.0: These need to be scrubbed to determine which parameters should be
**      here versus app_cfg.h or the JSON init file. 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide and the
**      app_c_demo app that illustrates best practices with comments.  
**   2. cFS Application Developer's Guide.
**
**   Written by David McComas, licensed under the Apache License, Version 2.0
**   (the "License"); you may not use this file except in compliance with the
**   License. You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**   Unless required by applicable law or agreed to in writing, software
**   distributed under the License is distributed on an "AS IS" BASIS,
**   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**   See the License for the specific language governing permissions and
**   limitations under the License.
*/

#ifndef _kit_sch_platform_cfg_
#define _kit_sch_platform_cfg_

/*
** Includes
*/

#include "kit_sch_mission_cfg.h"


/******************************************************************************
** Platform Deployment Configurations
*/

#define KIT_SCH_PLATFORM_REV   0
#define KIT_SCH_INI_FILENAME   "/cf/kit_sch_ini.json"


/******************************************************************************
** Scheduler Table Configurations
*/

#define SCHTBL_JSON_FILE_MAX_CHAR  16384

/*
** Number of minor frame slots within each Major Frame. Must be 2 or more and less than 65536.
*/
#define SCHTBL_SLOTS      4


/*
** Maximum number of Activities per Minor Frame. Must be greater than zero.
*/
#define SCHTBL_ACTIVITIES_PER_SLOT  15


#define SCHTBL_MAX_ENTRIES (SCHTBL_SLOTS * SCHTBL_ACTIVITIES_PER_SLOT)


/******************************************************************************
** Message Table Configurations
*/

/*
** Maximum Number of Message Definitions in Message Definition Table. Must be 
** greater than zero.
*/

#define MSGTBL_MAX_ENTRIES           200
#define MSGTBL_JSON_FILE_MAX_CHAR  16384
#define MSGTBL_UNUSED_MSG_ID       (CFE_SB_INVALID_MSG_ID)

/*
** Max message length in words.  Must be at least large enough to hold the
** smallest possible message header(see #CFE_SB_TLM_HDR_SIZE and 
** #CFE_SB_CMD_HDR_SIZE)
*/
#define MSGTBL_MAX_MSG_WORDS      8
#define MSGTBL_MAX_MSG_BYTES      (MSGTBL_MAX_MSG_WORDS*2)


/******************************************************************************
** Scheduler Configurations
*/

/*
** Number of Minor Frames that will be processed in "Catch Up"
** mode before giving up and skipping ahead.
*/
#define SCHEDULER_MAX_LAG_COUNT  (SCHTBL_SLOTS / 2)


/*
** Maximum number of slots scheduler will process when trying to
** "Catch Up" to the correct slot for the current time. Must be greater than zero.
*/
#define SCHEDULER_MAX_SLOTS_PER_WAKEUP      5


/*
** Number of microseconds in a Major Frame. Used as a "wake-up" period. Must be greater than zero.
*/
#define SCHEDULER_MICROS_PER_MAJOR_FRAME    1000000


/*
** Defines the additional time allowed in the Synchronization Slot to allow
** the Major Frame Sync signal to be received and re-synchronize processing.
** Must be less than the normal slot period.
*/
#define SCHEDULER_SYNC_SLOT_DRIFT_WINDOW   5000


/*
** Defines the time allowed for the first Major Frame sync signal to arrive
** before assuming it is not going to occur and switching to a free-wheeling
** mode. Must be greater than or equal to the Major Frame Period
*/
#define SCHEDULER_STARTUP_PERIOD   (5*SCHEDULER_MICROS_PER_MAJOR_FRAME)


/*
** Defines the number of consecutive "Noisy" Major Frame Signals (i.e. -
** signals that occur outside the expected window of their occurrence)
** until the Major Frame signal is automatically ignored and the Minor
** Frame Timer is used instead. This value should never be set to less
** than two because a single "noisy" Major Frame signal is likely when
** turning on or switching the 1 Hz signal on the spacecraft.
*/
#define SCHEDULER_MAX_NOISY_MF   2



#endif /* _kit_sch_platform_cfg_ */
