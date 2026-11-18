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
#ifndef TLM_SEND_H
#define TLM_SEND_H

#include <stdbool.h>
#include <stddef.h>

#include "console_utils.h"

#define TLMRECV_MAX_PACKET_SIZE 2048 /* Max packet size, ref: CCSDS max = 65542, IPv4 UDP max = 65507 */

typedef struct TlmRecv_Display_API
{
    const char *Name;
    void *(*Instantiate)(void);

    bool (*DisplayObject)(void *, const void *, size_t);

} TlmRecv_Display_API_t;

extern const TlmRecv_Display_API_t PassThru_API;
extern const TlmRecv_Display_API_t EDS_API;

#endif
