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
 * \file     edslib_global.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Defines a runtime API to obtain information from EDS at a global scope
 * This information applies to all DB categories (data types, display info, and interfaces)
 */

#ifndef EDSLIB_GLOBAL_H
#define EDSLIB_GLOBAL_H

#include <stdint.h>
#include <stddef.h>

#include <edslib_id.h>
#include <edslib_api_types.h>

/******************************
 * MACROS
 ******************************/

/******************************
 * TYPEDEFS
 ******************************/

/******************************
 * API CALLS
 ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Gets the number of packages defined
     *
     * This returns the size of the package name table, which in turn contains the
     * names of all the EDS packages/datasheets.  When iterating through all packages
     * this serves as the upper limit for that iteration.
     *
     * @note the value returned is the first INVALID package index, thus it should
     * not be included in the iteration loop.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @return Highest application index
     * @retval 0 if the database is empty (no applications defined)
     */
    uint16_t EdsLib_GetNumPackages(const EdsLib_DatabaseObject_t *GD);

    /**
     * Find an EDS package index by substring match
     *
     * This finds the top-level index in the EDS database.  This version of the API is intended
     * for use with multi-part names, where the match is being performed only up to the first
     * namespace separator charactor.
     *
     * @sa EdsLib_FindPackageIdxByName() for a simple complete-string match
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  MatchString the name of the package/application to find
     * @param[in]  MatchLen the length of the substring to match
     * @param[out] IdxOut output buffer for the Package/Application index value, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_FindPackageIdxBySubstring(const EdsLib_DatabaseObject_t *GD,
                                             const char                    *MatchString,
                                             size_t                         MatchLen,
                                             uint8_t                       *IdxOut);

    /**
     * Find an EDS package index by name
     *
     * This finds the top-level index in the EDS database.  In the EDS domain, each top level package file
     * has a name associated with it (the "name" attribute in the XML) and this in turn will have
     * all other definitions underneath it.
     *
     * The index that is returned/output from this function serves as the first value for the EDSLIB_MAKE_ID
     * macro and other API calls that require an AppIdx parameter.  Alternatively, each package is also
     * given a static index via the EDS_INDEX() macro that can be used if the name is known at compile
     * time.  This function can be used where the name is only known at runtime.
     *
     * If the provided name has a SEDS namespace separator (/), then the match is performed only up to the
     * first namespace separator character.  Otherwise, the entire string will be matched.  This permits this
     * function to be used either with complete names or as the first part of a multi-part name lookup.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  PkgName the name of the package/application to find
     * @param[out] IdxOut output buffer for the Package/Application index value, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_FindPackageIdxByName(const EdsLib_DatabaseObject_t *GD, const char *PkgName, uint8_t *IdxOut);

    /**
     * Gets a package name from an index
     *
     * This locates the package index in the database and obtains the name associated with that package.
     * The name is specified via the "name" attribute on the top-level EDS elements (Package or Namespace)
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  PkgIdx the Package/Application index value
     * @return String containing package name
     * @retval NULL if the package index is not valid
     */
    const char *EdsLib_GetPackageNameOrNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx);

    /**
     * Gets a package name from an index
     *
     * This locates the package index in the database and obtains the name associated with that package.
     * The name is specified via the "name" attribute on the top-level EDS elements (Package or Namespace)
     *
     * @note this does _not_ return NULL on error.  In the event that the package index is invalid or
     * undefined, then a special (printable) string value is returned.  Thus this can be safely used in
     * circumstances where NULL would be problematic, such as printf() style calls.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  PkgIdx the Package/Application index value
     * @return String containing package name, or a special string if undefined
     */
    const char *EdsLib_GetPackageNameNonNull(const EdsLib_DatabaseObject_t *GD, uint8_t PkgIdx);

    /**
     * Convert a bit count to a byte/octet count
     *
     * A generic utility function to consistently convert a bit count into a byte count
     *
     * In the event that the bit count is not a multiple of 8, this rounds up
     * to include the partial octet.  Complete messages/containers will normally
     * be a multiple of 8 bits, but the fields within a container may be partial.
     */
    static inline size_t EdsLib_BITS_TO_OCTETS(size_t Bits)
    {
        return (Bits + 7) / 8;
    }

    /**
     * Convert a byte/octet count into a bit count
     *
     * A generic utility function to consistently convert an octet count into a bit count
     */
    static inline size_t EdsLib_OCTETS_TO_BITS(size_t Octets)
    {
        return (Octets * 8);
    }

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EDSLIB_INTFDB_H */
