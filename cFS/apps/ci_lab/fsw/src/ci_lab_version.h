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
 *  The CI Lab App header file containing version information
 */
#ifndef CI_LAB_VERSION_H
#define CI_LAB_VERSION_H

/* Development Build Macro Definitions */

#define CI_LAB_BUILD_NUMBER    1        /*!< Development Build: Number of commits since baseline */
#define CI_LAB_BUILD_BASELINE  "v7.0.1" /*!< Development Build: git tag that is the base for the current development */
#define CI_LAB_BUILD_DEV_CYCLE "v7.0.1" /**< @brief Development: Release name for current development cycle */
#define CI_LAB_BUILD_CODENAME  "Draco"  /**< @brief: Development: Code name for the current build */

/*
 * Version Macros, see \ref cfsversions for definitions.
 */
#define CI_LAB_MAJOR_VERSION 7 /*!< @brief Major version number */
#define CI_LAB_MINOR_VERSION 0 /*!< @brief Minor version number */
#define CI_LAB_REVISION      1 /*!< @brief Revision version number. Value of 0 indicates a development version.*/

/**
 * @brief Last official release.
 */
#define CI_LAB_LAST_OFFICIAL "v7.0.1"

/*!
 * @brief Mission revision.
 *
 * Reserved for mission use to denote patches/customizations as needed.
 * Values 1-254 are reserved for mission use to denote patches/customizations as needed. NOTE: Reserving 0 and 0xFF for
 * cFS open-source development use (pending resolution of nasa/cFS#440)
 */
#define CI_LAB_MISSION_REV 0xFF

#define CI_LAB_STR_HELPER(x) #x /*!< @brief Helper function to concatenate strings from integer macros */
#define CI_LAB_STR(x)        CI_LAB_STR_HELPER(x) /*!< @brief Helper function to concatenate strings from integer macros */

/*! @brief Development Build Version Number.
 * @details Baseline git tag + Number of commits since baseline. @n
 * See @ref cfsversions for format differences between development and release versions.
 */
#define CI_LAB_VERSION CI_LAB_BUILD_BASELINE "+dev" CI_LAB_STR(CI_LAB_BUILD_NUMBER)

/**
 * @brief Max Version String length.
 *
 * Maximum length that a CI Lab version string can be.
 *
 */
#define CI_LAB_CFG_MAX_VERSION_STR_LEN 256

#endif
