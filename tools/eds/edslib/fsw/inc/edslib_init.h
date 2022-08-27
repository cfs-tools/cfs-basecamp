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
 * \file     edslib_init.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Prototype for the combined EDS library init function
 */

#ifndef _EDSLIB_INIT_H_
#define _EDSLIB_INIT_H_

/**
 * Combined EDS library initializer function.
 *
 * Initializes the complete library, including the DataTypeDB,
 * DisplayDB, and Binding object API.
 */
void EdsLib_Initialize(void);

#endif  /* _EDSLIB_INIT_H_ */

