/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 *
 * Copyright (c) 2020 United States Government as represented by
 * the Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file     cfe_missionlib_internal.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Defines a runtime API for CFE to obtain information about
 * the software bus interfaces/routing and correlate with EDS.
 */

#ifndef CFE_MISSIONLIB_INTERNAL_H
#define CFE_MISSIONLIB_INTERNAL_H

#include <stdint.h>
#include <stddef.h>

#include "edslib_database_types.h"
#include "edslib_database_ops.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_database_types.h"

const CFE_MissionLib_TopicId_Entry_t *CFE_MissionLib_Lookup_Topic(const CFE_MissionLib_SoftwareBus_Interface_t *SBDB,
                                                                  uint16_t TopicId);
int32_t                               CFE_MissionLib_ExportTopicInfo(const CFE_MissionLib_TopicId_Entry_t *TopicPtr,
                                                                     CFE_MissionLib_TopicInfo_t           *TopicInfo);

#endif /* CFE_MISSIONLIB_INTERNAL_H */
