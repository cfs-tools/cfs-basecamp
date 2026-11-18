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
 * \file
 *
 * A PSP module to add a scratch pad to be used with the "RAM" API.
 */

#include "cfe_psp.h"
#include "cfe_psp_module.h"

/*
** global memory
*/
#define CFE_PSP_SCRATCH_PAD_LEN 1024

uint32 CFE_PSP_DebugScratchPad[CFE_PSP_SCRATCH_PAD_LEN];

CFE_PSP_MODULE_DECLARE_SIMPLE(ram_debug);

void ram_debug_Init(uint32 PspModuleId)
{
    int32 Status;

    /* Inform the user that this module is in use */
    printf("CFE_PSP: Created Memory Mapped RAM scratchpad implementation\n");

    /* Add memory range for scratch pad */
    Status = CFE_PSP_MemRangeSet(2,
                                 CFE_PSP_MEM_RAM,
                                 (cpuaddr)&CFE_PSP_DebugScratchPad[0],
                                 sizeof(CFE_PSP_DebugScratchPad),
                                 CFE_PSP_MEM_SIZE_DWORD,
                                 CFE_PSP_MEM_ATTR_READWRITE);

    OS_printf("CFE_PSP: CFE_PSP_DebugScratchPad Range (3) created: Start Address = %08lX, Size = %08X Status = %d\n",
              (unsigned long)&CFE_PSP_DebugScratchPad[0],
              (unsigned int)sizeof(CFE_PSP_DebugScratchPad),
              (int)Status);
}
