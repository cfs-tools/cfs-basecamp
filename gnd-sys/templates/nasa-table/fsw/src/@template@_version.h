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
 *
 *  The @TEMPLATE@ App header file containing version information
 */

#ifndef @TEMPLATE@_VERSION_H
#define @TEMPLATE@_VERSION_H

/* Development Build Macro Definitions */

#define @TEMPLATE@_BUILD_NUMBER 1
#define @TEMPLATE@_BUILD_BASELINE \
    "v7.0.0" /*!< Development Build: git tag that is the base for the current development */
#define @TEMPLATE@_BUILD_DEV_CYCLE "v7.0.1" /**< @brief Development: Release name for current development cycle */
#define @TEMPLATE@_BUILD_CODENAME  "Draco"  /**< @brief: Development: Code name for the current build */

/*
 * Version Macros, see \ref cfsversions for definitions.
 */
#define @TEMPLATE@_MAJOR_VERSION 7 /*!< @brief Major version number. */
#define @TEMPLATE@_MINOR_VERSION 0 /*!< @brief Minor version number. */
#define @TEMPLATE@_REVISION      1 /*!< @brief Revision version number. Value of 0 indicates a development version.*/

/**
 * @brief Last official release.
 */
#define @TEMPLATE@_LAST_OFFICIAL "v7.0.1"

/*!
 * @brief Mission revision.
 *
 * Reserved for mission use to denote patches/customizations as needed.
 * Values 1-254 are reserved for mission use to denote patches/customizations as needed. NOTE: Reserving 0 and 0xFF for
 * cFS open-source development use (pending resolution of nasa/cFS#440)
 */
#define @TEMPLATE@_MISSION_REV 0xFF

#define @TEMPLATE@_STR_HELPER(x) #x /*!< @brief Helper function to concatenate strings from integer macros */
#define @TEMPLATE@_STR(x) \
    @TEMPLATE@_STR_HELPER(x) /*!< @brief Helper function to concatenate strings from integer macros */

/*! @brief Development Build Version Number.
 * @details Baseline git tag + Number of commits since baseline. @n
 * See @ref cfsversions for format differences between development and release versions.
 */
#define @TEMPLATE@_VERSION @TEMPLATE@_BUILD_BASELINE "+dev" @TEMPLATE@_STR(@TEMPLATE@_BUILD_NUMBER)

/**
 * @brief Max Version String length.
 *
 * Maximum length that an OSAL version string can be.
 *
 */
#define @TEMPLATE@_CFG_MAX_VERSION_STR_LEN 256

#endif /* @TEMPLATE@_VERSION_H */
