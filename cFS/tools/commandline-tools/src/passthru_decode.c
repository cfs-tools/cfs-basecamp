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

/*
 * System includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "tlm_recv.h"

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
void *PassThru_Instantiate(void)
{
    static int NOT_USED;
    return &NOT_USED; /* must return non-null */
}

/*----------------------------------------------------------------
 *
 * API function
 *
 *-----------------------------------------------------------------*/
bool PassThru_DisplayObject(void *arg, const void *Data, size_t Size)
{
    ConsoleUtils_Hexdump(Data, Size);
    printf("\n");
    return true;
}

/*----------------------------------------------------------------
 *
 * API structure
 *
 *-----------------------------------------------------------------*/
/* clang-format off */
const TlmRecv_Display_API_t PassThru_API =
{
    .Name = "PassThrough",
    .Instantiate = PassThru_Instantiate,
    .DisplayObject = PassThru_DisplayObject
};
