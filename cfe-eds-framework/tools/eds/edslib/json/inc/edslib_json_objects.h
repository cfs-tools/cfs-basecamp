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
 * \file     edslib_json_objects.h
 * \ingroup  json
 * \author   joseph.p.hickey@nasa.gov
 * 
 * API definition for EdsLib-JSON binding library
 */

#ifndef _EDSLIB_JSON_OBJECTS_H_
#define _EDSLIB_JSON_OBJECTS_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

/*
 * decouple from json-c headers;
 * json_object will be an abstract struct
 */
typedef struct json_object                      EdsLib_JsonBinding_Object_t;
typedef struct EdsLib_Binding_DescriptorObject  EdsLib_JsonBinding_DescriptorObject_t;

void EdsLib_JSON_EdsObjectFromJSON(EdsLib_JsonBinding_DescriptorObject_t *DstObject, EdsLib_JsonBinding_Object_t *SrcObject);
EdsLib_JsonBinding_Object_t *EdsLib_JSON_EdsObjectToJSON(const EdsLib_JsonBinding_DescriptorObject_t *SrcObject);

#ifdef __cplusplus
}   /* extern "C" */
#endif


#endif  /* _EDSLIB_JSON_OBJECTS_H_ */

