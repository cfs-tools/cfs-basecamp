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
 * \file     edslib_database_types.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Data Types and constants used internally to the EDS library.
 *
 * Objects of these types are defined by the generated EDS database files and
 * these types are also used throughout the runtime library.
 *
 * HOWEVER - These typedefs are internal to the EDS runtime library and are
 * not be exposed directly via public API.  All access into these database objects
 * must be done via one of the many accessor functions in the public API, but
 * no direct references or pointers to these objects should ever be returned
 * to user application code.
 */

#ifndef EDSLIB_DATABASE_OPS_H
#define EDSLIB_DATABASE_OPS_H

#ifndef _EDSLIB_BUILD_
#error "Do not include edslib_database_types.h from application code; use the edslib API instead"
#endif

#include "edslib_database_types.h"
#include "edslib_id.h"

/**
 * Converts an external EdsLib_Id_t object into the internal EdsLib_DatabaseRef_t value
 * @sa  EdsLib_Encode_StructId
 */
void EdsLib_Decode_StructId(EdsLib_DatabaseRef_t *RefObj, EdsLib_Id_t EdsId);

/**
 * Converts an internal EdsLib_DatabaseRef_t object into the external EdsLib_Id_t value
 * @sa  EdsLib_Decode_StructId
 */
EdsLib_Id_t EdsLib_Encode_StructId(const EdsLib_DatabaseRef_t *RefObj);

/**
 * Equality check for instances of EdsLib_DatabaseRef_t
 *
 * @retval true if the two refs point to the same entity in the database
 * @retval false if the two refs point to different entities in the database
 */
bool EdsLib_DatabaseRef_IsEqual(const EdsLib_DatabaseRef_t *RefObj1, const EdsLib_DatabaseRef_t *RefObj2);

#endif /* _EDSLIB_DATABASE_TYPES_H_ */
