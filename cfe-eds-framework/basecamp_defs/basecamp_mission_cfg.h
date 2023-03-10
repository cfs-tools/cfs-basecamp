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
 * Purpose:
 *   This header file contains the mission configuration parameters and
 *   typedefs with mission scope.
 *
 * Notes:
 *   The impact of changing these configurations from their default value is
 *   not yet documented.  Changing these values may impact the performance
 *   and functionality of the system.
 *
 * Author:   R.McGraw/SSI
 *
 */

#ifndef SAMPLE_MISSION_CFG_H
#define SAMPLE_MISSION_CFG_H

/*
 * Pull in defintions from EDS
 */
#include "basecamp_eds_designparameters.h"

/** \name Checksum/CRC algorithm identifiers */
/** \{ */
#define CFE_MISSION_ES_CRC_8  1 /**< \brief CRC ( 8 bit additive - returns 32 bit total) (Currently not implemented) */
#define CFE_MISSION_ES_CRC_16 2 /**< \brief CRC (16 bit additive - returns 32 bit total) */
#define CFE_MISSION_ES_CRC_32                                                              \
    3 /**< \brief CRC (32 bit additive - returns 32 bit total) (Currently not implemented) \
       */
/** \} */

/**
**  \cfeescfg Mission Default CRC algorithm
**
**  \par Description:
**      Indicates the which CRC algorithm should be used as the default
**      for verifying the contents of Critical Data Stores and when calculating
**      Table Image data integrity values.
**
**  \par Limits
**      Currently only CFE_MISSION_ES_CRC_16 is supported (see #CFE_MISSION_ES_CRC_16)
*/
#define CFE_MISSION_ES_DEFAULT_CRC CFE_MISSION_ES_CRC_16


#endif /* SAMPLE_MISSION_CFG_H */
