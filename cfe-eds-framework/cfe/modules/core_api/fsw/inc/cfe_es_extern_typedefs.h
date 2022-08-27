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
 * Declarations and prototypes for cfe_es_extern_typedefs module
 */

#ifndef CFE_ES_EXTERN_TYPEDEFS_H
#define CFE_ES_EXTERN_TYPEDEFS_H

/* Use the EDS generated version of these types */
#include "cfe_es_eds_typedefs.h"

/*
 * Provide mappings for any software names not directly
 * provided by EDS generated headers
 */

/**
 * @brief Type used for memory sizes and offsets in commands and telemetry
 *
 * For backward compatibility with existing CFE code this should be uint32,
 * but all telemetry information will be limited to 4GB in size as a result.
 *
 * On 64-bit platforms this can be a 64-bit value which will allow larger
 * memory objects, but this will break compatibility with existing control
 * systems, and may also change the alignment/padding of messages.
 *
 * In either case this must be an unsigned type.
 */
typedef CFE_ES_MemAddress_Atom_t CFE_ES_MemOffset_t;

/*
 * A converter macro to use when initializing an CFE_ES_MemOffset_t
 * from an integer value of a different type.
 */
#define CFE_ES_MEMOFFSET_C(x) ((CFE_ES_MemOffset_t)(x))

/**
 * @brief Type used for memory addresses in command and telemetry messages
 *
 * For backward compatibility with existing CFE code this should be uint32,
 * but if running on a 64-bit platform, addresses in telemetry will be
 * truncated to 32 bits and therefore will not be valid.
 *
 * On 64-bit platforms this can be a 64-bit address which will allow the
 * full memory address in commands and telemetry, but this will break
 * compatibility with existing control systems, and may also change
 * the alignment/padding of messages.
 *
 * In either case this must be an unsigned type.
 *
 * FSW code should access this value via the macros provided, which
 * converts to the native "cpuaddr" type provided by OSAL.  This macro
 * provides independence between the message representation and local
 * representation of a memory address.
 */
typedef CFE_ES_MemAddress_Atom_t CFE_ES_MemAddress_t;

/*
 * A converter macro to use when initializing an CFE_ES_MemAddress_t
 * from a pointer value of a different type.
 *
 * @note on a 64 bit platform, this macro will truncate the address such
 * that it will fit into a 32-bit telemetry field.  Obviously, the resulting
 * value is no longer usable as a memory address after this.
 */
#define CFE_ES_MEMADDRESS_C(x) ((CFE_ES_MemAddress_t)((cpuaddr)(x)))

#endif /* CFE_ES_EXTERN_TYPEDEFS_H */
