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
** File: to_lab_events.h
**
** Purpose:
**  Define TO Lab Event messages
**
** Notes:
**
*************************************************************************/
#ifndef _to_lab_events_h_
#define _to_lab_events_h_

/*****************************************************************************/

/* Event message ID's */
#define TO_EVM_RESERVED 0

#define TO_INIT_INF_EID          1
#define TO_CRCMDPIPE_ERR_EID     2
#define TO_TLMOUTENA_INF_EID     3
#define TO_SUBSCRIBE_ERR_EID     4
#define TO_TLMPIPE_ERR_EID       5
#define TO_TLMOUTSOCKET_ERR_EID  6
#define TO_TLMOUTSTOP_ERR_EID    7
#define TO_MSGID_ERR_EID         8
#define TO_FNCODE_ERR_EID        9
#define TO_ADDPKT_ERR_EID        10
#define TO_REMOVEPKT_ERR_EID     11
#define TO_REMOVEALLPTKS_ERR_EID 12
#define TO_ADDPKT_INF_EID        15
#define TO_REMOVEPKT_INF_EID     16
#define TO_REMOVEALLPKTS_INF_EID 17
#define TO_NOOP_INF_EID          18
#define TO_TBL_ERR_EID           19

/******************************************************************************/

#endif /* _to_lab_events_h_ */
