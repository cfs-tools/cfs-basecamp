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

##################################################################
#
# Standalone EdsLib Build Recipe
#
# This builds the EDS library as a separate entity, not directly
# integrated into any other project.  The artifacts of this build
# can then be installed into a directory and used in a separate 
# project.
#
##################################################################

cmake_minimum_required(VERSION 3.10)
project(EDS C)

# The build options will be turned on
option(EDSLIB_BUILD_MODULE
    "Build a general-purpose EdsLib module for dynamically-linked applications"
    ON)

option(EDSLIB_PYTHON_BUILD_STANDALONE_MODULE
    "Build a generic Python module that can be imported into standalone Python code"
    ON)


# If unit testing is enabled, this requires the UT assert library which
# is part of the NASA OSAL.  This should be built separately and the path
# to this should be provided via the CMAKE_PREFIX_PATH configuration variable.
if (ENABLE_UNIT_TESTS)
  enable_testing()

  # This currently does not support cross compiling because building the unit tests
  # requires execution of the sedstool on the host.  Running unit tests on a target
  # is possible, but it requires SEPARATELY building the sedstool for the host and 
  # then using that in the cross build.  This is not yet implemented.
  if (CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "Cannot build unit tests in a cross compile environment using this script")
  endif()

  find_package(NasaOsal REQUIRED)
  message(STATUS "Using external OSAL ${NasaOsal_VERSION} for unit testing")

endif()

add_compile_definitions(_XOPEN_SOURCE=600)

# The EDS tool is in the same repo, so the relative path should be consistent
include (tool/cmake/edstool-execute-target.cmake)

# Allow code to easily reference the generated include files
include_directories(${CMAKE_BINARY_DIR}/inc)

add_subdirectory(edslib)
add_subdirectory(tool)

if (ENABLE_UNIT_TESTS)
  message(STATUS "Set up EDS validation test for STANDALONE build")

  edstool_find_eds_sources("ut" ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/edslib
    ${CMAKE_CURRENT_SOURCE_DIR}/validation
  )

  edstool_setup_toplevel(edstool-execute-ut "ut" ${CMAKE_BINARY_DIR})

  add_library(ut_eds_db STATIC IMPORTED GLOBAL)
  set_target_properties(ut_eds_db PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/obj/ut_eds_db${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )

  add_dependencies(ut_eds_db edstool-execute-ut)

endif()

