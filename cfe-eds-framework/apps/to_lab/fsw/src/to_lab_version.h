/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
*************************************************************************/
#ifndef TO_LAB_VERSION_H
#define TO_LAB_VERSION_H

/*! @file to_lab_version.h
 * @brief Purpose:
 *
 *  The TO Lab Application header file containing version information
 *
 */

/* Development Build Macro Definitions */
#define TO_LAB_BUILD_NUMBER 58 /*!< Development Build: Number of commits since baseline */
#define TO_LAB_BUILD_BASELINE \
    "v2.4.0-rc1" /*!< Development Build: git tag that is the base for the current development */

/* Version Macro Definitions */

#define TO_LAB_MAJOR_VERSION 2  /*!< @brief ONLY APPLY for OFFICIAL releases. Major version number. */
#define TO_LAB_MINOR_VERSION 3  /*!< @brief ONLY APPLY for OFFICIAL releases. Minor version number. */
#define TO_LAB_REVISION      99 /*!< @brief ONLY APPLY for OFFICIAL releases. Revision version number. */
#define TO_LAB_MISSION_REV   0  /*!< @brief ONLY USED by MISSION Implementations. Mission revision */

#define TO_LAB_STR_HELPER(x) #x /*!< @brief Helper function to concatenate strings from integer macros */
#define TO_LAB_STR(x)        TO_LAB_STR_HELPER(x) /*!< @brief Helper function to concatenate strings from integer macros */

/*! @brief Development Build Version Number.
 * @details Baseline git tag + Number of commits since baseline. @n
 * See @ref cfsversions for format differences between development and release versions.
 */
#define TO_LAB_VERSION TO_LAB_BUILD_BASELINE "+dev" TO_LAB_STR(TO_LAB_BUILD_NUMBER)

/*! @brief Development Build Version String.
 * @details Reports the current development build's baseline, number, and name. Also includes a note about the latest
 * official version. @n See @ref cfsversions for format differences between development and release versions.
 */
#define TO_LAB_VERSION_STRING                   \
    " TO Lab DEVELOPMENT BUILD " TO_LAB_VERSION \
    ", Last Official Release: v2.3.0" /* For full support please use this version */

#endif /* TO_LAB_VERSION_H */

/************************/
/*  End of File Comment */
/************************/
