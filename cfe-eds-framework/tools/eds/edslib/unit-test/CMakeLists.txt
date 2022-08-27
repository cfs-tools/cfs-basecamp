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

#
# EDS Unit Test Build script
#
# For consistent unit testing we will use a custom XML data definition, NOT the one from the active mission
# This gives a couple advantages -
#  - The custom XML files can be crafted to exercise as many features as possible, regardless of
#    what is actively in use in the current mission
#  - The edslib unit test should not break when the active mission XML is updated.
if (DEFINED MISSION_BINARY_DIR)
  set(UTM_BINARY_DIR ${MISSION_BINARY_DIR}/edslib/unit-test)
else ()
  set(UTM_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif ()

include_directories(${EDSLIB_SOURCE_DIR}/inc)
include_directories(${UTM_BINARY_DIR}/inc)

# Add the components to the EDS UT top level target
file(GLOB_RECURSE UTXML FOLLOW_SYMLINKS ${CMAKE_CURRENT_SOURCE_DIR}/eds/*.xml)
eds_start_toplevel(UTM)
foreach(XMLFILE ${UTXML})
  get_filename_component(EDSFILE ${XMLFILE} NAME_WE)
  eds_add_component(UTM ${EDSFILE} ${EDSFILE})
endforeach()

if (COMMAND eds_add_generator)
  eds_add_generator(UTM "UTHDR/BasePacket" "" ${UTXML})
else()
  include_directories(${OSAL_SOURCE_DIR}/ut_assert/inc)
  aux_source_directory(../fsw/src EDSLIB_SRCS)
  add_unit_test_lib(edslib_test ${EDSLIB_SRCS})
  add_unit_test_exe(edslib_test edslib_test.c edslib_basic_test.c edslib_full_test.c)
  target_link_libraries(edslib_test UTM_eds)
endif()


