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

# Build script for EdsLib-JSON binding library
project(EDSLIB_JSONBINDINGS C)

# This uses the "json-c" library to do JSON manipulation
if(NOT JSON_C_FOUND)

  # By default, try to find any json-c version if no version specified...
  #  This simplifies building as a simple local library.

  # But do not assume this default when cross-compiling or building CFS,
  # because if the toolchain isn't configured correctly, searching could
  # find the wrong version and break the build
  # (in particular, pkg-config might find the host version instead of
  # the cross version and try to use it)

  # In any sort of cross-compile or CFS build, then the user needs to
  # explicitly do something to enable JSON bindings.

  if (NOT DEFINED EDSLIB_JSON_C_SEARCH_VERSION AND
      NOT CMAKE_CROSSCOMPILING AND
      NOT IS_CFS_ARCH_BUILD)
    set(EDSLIB_JSON_C_SEARCH_VERSION json-c json)
  endif()

  if (EDSLIB_JSON_C_SEARCH_VERSION)
    find_edslib_dependency(JSON_C json.h ${EDSLIB_JSON_C_SEARCH_VERSION})
  endif(EDSLIB_JSON_C_SEARCH_VERSION)

endif(NOT JSON_C_FOUND)


# If not found then this module cannot be built, however
# this is an optional feature therefore not a fatal error.
if (NOT JSON_C_FOUND)
    message("    JSON support not configured, skipping EdsLib JSON bindings")
else(NOT JSON_C_FOUND)
    include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
    include_directories(${JSON_C_INCLUDE_DIRS})
    include_directories(inc)

    set(EDSLIB_JSON_SOURCES src/edslib_json_objects.c)

    # The "edslib_json_static" target is a simple static library
    add_library(edslib_json_static STATIC EXCLUDE_FROM_ALL
        ${EDSLIB_JSON_SOURCES})
    target_link_libraries(edslib_json_static ${JSON_C_LIBRARIES})

    # The "edslib_json_pic" target is the same code complied as PIC
    # it can be used as the basis of a module or shared lib
    add_library(edslib_json_pic    OBJECT EXCLUDE_FROM_ALL
        ${EDSLIB_JSON_SOURCES})
    set_target_properties(edslib_json_pic PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE)


endif (NOT JSON_C_FOUND)

