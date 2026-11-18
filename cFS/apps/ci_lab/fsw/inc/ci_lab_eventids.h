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
 *  Define CI Lab Events IDs
 */
#ifndef CI_LAB_EVENTIDS_H
#define CI_LAB_EVENTIDS_H

#define CI_LAB_RESERVED_EID             0
#define CI_LAB_SOCKETCREATE_ERR_EID     1
#define CI_LAB_SOCKETBIND_ERR_EID       2
#define CI_LAB_INIT_INF_EID             3
#define CI_LAB_MID_ERR_EID              4
#define CI_LAB_NOOP_INF_EID             5
#define CI_LAB_RESET_INF_EID            6
#define CI_LAB_INGEST_INF_EID           7
#define CI_LAB_INGEST_LEN_ERR_EID       8
#define CI_LAB_INGEST_ALLOC_ERR_EID     9
#define CI_LAB_INGEST_SEND_ERR_EID      10
#define CI_LAB_CR_PIPE_ERR_EID          11
#define CI_LAB_SB_SUBSCRIBE_CMD_ERR_EID 12
#define CI_LAB_SB_SUBSCRIBE_HK_ERR_EID  13
#define CI_LAB_SB_SUBSCRIBE_UL_ERR_EID  14
#define CI_LAB_CMD_LEN_ERR_EID          16

#endif
