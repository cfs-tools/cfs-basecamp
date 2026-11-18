#
# LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
#
# Copyright (c) 2020 United States Government as represented by
# the Administrator of the National Aeronautics and Space Administration.
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Get a reference to the top of the git repo for the whole EdsLib submodule.
# All sub-directory refs under this path should be at a consistent relative path
# because they are all controlled together.
get_filename_component(EDS_REPO_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

function (edstool_find_eds_sources EDSTOOL_PROJECT_NAME EDSTOOL_OUTPUT_DIR)

  # Sanity check: The directory should have been created already
  if (NOT IS_DIRECTORY "${EDSTOOL_OUTPUT_DIR}")
    message(FATAL_ERROR "Output directory does not exist: ${EDSTOOL_OUTPUT_DIR}")
  endif()

  set(CURRENT_EDSTOOL_SCRIPTLIST)
  set(CURRENT_EDSTOOL_FILELIST)


  file(GLOB_RECURSE CURRENT_EDSTOOL_SCRIPTLIST FOLLOW_SYMLINKS
    ${EDS_REPO_SOURCE_DIR}/tool/scripts/*.lua
  )

  foreach(DIR ${ARGN})
    if (IS_DIRECTORY ${DIR}/eds)
      file(GLOB_RECURSE DIRXML FOLLOW_SYMLINKS ${DIR}/eds/*.xml)
      file(GLOB_RECURSE DIRSCRIPTS FOLLOW_SYMLINKS ${DIR}/eds/*.lua)
      list(APPEND CURRENT_EDSTOOL_FILELIST ${DIRXML})
      list(APPEND CURRENT_EDSTOOL_SCRIPTLIST ${DIRSCRIPTS})
      unset(DIRXML)
      unset(DIRSCRIPTS)
    endif()
  endforeach()

  # EDS toolchain plugin scripts may have ordering dependencies between them,
  # so the assembled set of available scripts should be sorted by its filename
  # This allows a numeric prefix (NN-) to be prepended in cases where
  # the execution order is a concern (similar to UNIX SysV init system approach)
  set(EDS_SCRIPT_BASENAMES)
  foreach(SCRIPTFILE ${CURRENT_EDSTOOL_SCRIPTLIST})
    get_filename_component(BASENAME ${SCRIPTFILE} NAME_WE)
    set(${BASENAME}_SCRIPT_LOCATION ${SCRIPTFILE})
    list(APPEND EDS_SCRIPT_BASENAMES ${BASENAME})
  endforeach(SCRIPTFILE ${CURRENT_EDSTOOL_SCRIPTLIST})

  # Now rebuild the script list sorted by basename
  list(SORT EDS_SCRIPT_BASENAMES)
  set(CURRENT_EDSTOOL_SCRIPTLIST)
  foreach(BASENAME ${EDS_SCRIPT_BASENAMES})
    list(APPEND CURRENT_EDSTOOL_SCRIPTLIST ${${BASENAME}_SCRIPT_LOCATION})
    unset(${BASENAME}_SCRIPT_LOCATION)
  endforeach(BASENAME ${EDS_SCRIPT_BASENAMES})
  unset(EDS_SCRIPT_BASENAMES)

  # Export the set of EDS source files
  configure_file(
      ${EDS_REPO_SOURCE_DIR}/tool/cmake/edstool-sources.mk.in
      ${EDSTOOL_OUTPUT_DIR}/edstool-sources-${EDSTOOL_PROJECT_NAME}.mk
  )

endfunction()


function(edstool_setup_toplevel TARGET_NAME EDSTOOL_PROJECT_NAME EDSTOOL_OUTPUT_DIR)

  # Sanity check: The directory should have been created already
  if (NOT IS_DIRECTORY "${EDSTOOL_OUTPUT_DIR}")
    message(FATAL_ERROR "Output directory does not exist: ${EDSTOOL_OUTPUT_DIR}")
  endif()

  # Capture critical variables from the build environment so they can be
  # used in custom makefiles that are invoked from here as custom targets
  string(TOLOWER ${EDSTOOL_PROJECT_NAME}_eds EDS_FILE_PREFIX)
  string(TOUPPER ${EDSTOOL_PROJECT_NAME} EDS_SYMBOL_PREFIX)

  configure_file(
    ${EDS_REPO_SOURCE_DIR}/tool/cmake/edstool-namespace.mk.in
    ${EDSTOOL_OUTPUT_DIR}/edstool-namespace-${EDSTOOL_PROJECT_NAME}.mk
  )

  # This target builds the full set of artifacts
  add_custom_target(${TARGET_NAME}
    COMMAND "\$(MAKE)"
      EDSTOOL=$<TARGET_FILE:sedstool>
      EDSTOOL_PROJECT_NAME="${EDSTOOL_PROJECT_NAME}"
      O=obj
      S=src
      -f ${EDS_REPO_SOURCE_DIR}/tool/cmake/edstool-execute-all.mk
      all
    WORKING_DIRECTORY
      ${EDSTOOL_OUTPUT_DIR}
    DEPENDS
      ${EDSTOOL_OUTPUT_DIR}/edstool-namespace-${EDSTOOL_PROJECT_NAME}.mk
      ${EDSTOOL_OUTPUT_DIR}/edstool-sources-${EDSTOOL_PROJECT_NAME}.mk
      sedstool
  )

endfunction()
