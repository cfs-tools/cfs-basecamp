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

# CFE/CFS EDS Mission Library Build Script
# -----------------------------------------
#
# This is a component of the larger CFE/CFS Electronic Data Sheet (EDS) integration and
# it contains the runtime database objects.  It is intended to be a component within a
# larger mission or CPU build.  This Cmake script supports both configuration as part
# of the parent build via the add_subdirectory() command, or as an external build
# that references back to the CFE/CFS build.
#
# The objective of this project is to generate the "interface" database which specifies
# how the CFE software bus components interconnect.  Primarily this includes how message
# ID values (The MsgId parameter on CFE Software Bus interfaces) map back to the data
# types defined in EDS and vice versa.
#
# To do this, additional lookup tables are generated from the EDS information using the
# EDS toolchain.  The generators of these lookup tables are contained in the "scripts"
# subdirectory.  The contents of this directory should be included on the sedstool command
# line which is handled outside of this script.
#
project(EDS_CFECFS_MISSIONLIB_FSW C)

include_directories(inc)
include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)

# The "runtime" libraries are specifically intended to provide the mapping logic required
# to convert between CFE Message IDs, which are used for routing, to CFE Topic IDs which
# are used for actually identifying message content.  These two concepts are related but
# different and they are not interchangable.  Mainly there can be many different _instances_
# of a particular component that each would have a different Message ID for routing, but
# all of these different sources would have the same _content type_ as payload.
#
# The logic to provide these mappings is implemented as a bit of C code written in terms of
# the EDS-defined interface types.  This allows the same actual code to be used in flight software
# as well as ground tools (including this one to generate the lookup tables).  This is intended
# to ensure with high confidence that the flight and ground tools will use the same mappings,
# because it will be using the same exact code
#
# As such, the EDS toolchain needs to execute at least to the point of generating the header
# files before these can be compiled.  This creates a circular dependency because the tool itself
# also needs to use this code to create the lookup tables.  This is solved by using a Lua script
# to actually compile this code.  In order to avoid having to pass pathnames to that script,
# create a wrapper around the needed header file.  This also allows a mission to override this file
# with a customized definition if the default mappings are not appropriate.
file(WRITE ${MISSION_BINARY_DIR}/src/cfe_missionlib_database_types.h.tmp
    "/* Generated wrapper based off the real source file location */\n"
    "#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/cfe_missionlib_database_types.h\"\n"
)
# Update the output only if different - avoid unnecessary rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${MISSION_BINARY_DIR}/src/cfe_missionlib_database_types.h.tmp
    ${MISSION_BINARY_DIR}/src/cfe_missionlib_database_types.h
)
file(REMOVE ${MISSION_BINARY_DIR}/src/cfe_missionlib_database_types.h.tmp)

# Missions may provide a custom version of the runtime source file.
# Use it if present and override the default one.
if (EXISTS ${MISSION_DEFS}/${MISSIONCONFIG}_cfe_missionlib_runtime.c)
    # Prefer the mission-supplied implementation
    set(RUNTIME_SOURCE ${MISSION_DEFS}/${MISSIONCONFIG}_cfe_missionlib_runtime.c)
else()
    # Fallback to using a default implementation
    set(RUNTIME_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/cfe_missionlib_runtime_default.c)
endif()

# Create a static and shared build of the runtime source file
# Note these are actually built as a sub-job within the EDS toolchain
# The "EXCLUDE_FROM_ALL" flag enables this, to avoid building as part of the top level build
add_library(EDS_${MISSION_NAME}_runtime_shared SHARED EXCLUDE_FROM_ALL
    ${RUNTIME_SOURCE}
)

add_library(EDS_${MISSION_NAME}_runtime_static STATIC EXCLUDE_FROM_ALL
    ${RUNTIME_SOURCE}
)

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MISSION_NAME}_runtime_shared_install.cmake CONTENT
    "execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different \"$<TARGET_FILE:EDS_${MISSION_NAME}_runtime_shared>\" \"${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_missionlib_runtime${CMAKE_SHARED_LIBRARY_SUFFIX}\")\n")

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MISSION_NAME}_runtime_static_install.cmake CONTENT
    "execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different \"$<TARGET_FILE:EDS_${MISSION_NAME}_runtime_static>\" \"${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_missionlib_runtime${CMAKE_STATIC_LIBRARY_SUFFIX}\")\n")

add_custom_target(missionlib-runtime-install
    COMMAND ${CMAKE_COMMAND} -P
        ${CMAKE_CURRENT_BINARY_DIR}/${MISSION_NAME}_runtime_static_install.cmake
    COMMAND ${CMAKE_COMMAND} -P
        ${CMAKE_CURRENT_BINARY_DIR}/${MISSION_NAME}_runtime_shared_install.cmake
    DEPENDS
        EDS_${MISSION_NAME}_runtime_static
        EDS_${MISSION_NAME}_runtime_shared
)

# Put the components together into a regular library
# This adds an API interface intended for use with the CFE software bus application
file(WRITE ${MISSION_BINARY_DIR}/inc/cfe_mission_eds_interface_parameters.h.tmp
    "/* Generated wrapper based off the real source file location */\n"
    "#include \"${EDS_FILE_PREFIX}_interfacedb.h\"\n"
    "#define CFE_SOFTWAREBUS_INTERFACE  ${EDS_SYMBOL_PREFIX}_SOFTWAREBUS_INTERFACE\n"
)
# Update the output only if different - avoid unnecessary rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${MISSION_BINARY_DIR}/inc/cfe_mission_eds_interface_parameters.h.tmp
    ${MISSION_BINARY_DIR}/inc/cfe_mission_eds_interface_parameters.h
)
file(REMOVE ${MISSION_BINARY_DIR}/inc/cfe_mission_eds_interface_parameters.h.tmp)

add_library(cfe_missionlib STATIC
    src/cfe_missionlib_api.c
)
target_compile_definitions(cfe_missionlib PRIVATE
    "_EDSLIB_BUILD_"
)
target_include_directories(cfe_missionlib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${EDSLIB_FSW_SOURCE_DIR}/inc
)

# CFE_MissionLib pic libraries
add_library(cfe_missionlib_pic STATIC EXCLUDE_FROM_ALL
    src/cfe_missionlib_api.c
)
set_target_properties(cfe_missionlib_pic PROPERTIES 
    POSITION_INDEPENDENT_CODE TRUE COMPILE_DEFINITIONS "_EDSLIB_BUILD_")

add_library(cfe_missionlib_runtime_pic STATIC EXCLUDE_FROM_ALL
    src/cfe_missionlib_api.c
    ${RUNTIME_SOURCE}
)
set_target_properties(cfe_missionlib_runtime_pic PROPERTIES 
    POSITION_INDEPENDENT_CODE TRUE COMPILE_DEFINITIONS "_EDSLIB_BUILD_")

# UT stubs are only needed for a CFS target build
if (ENABLE_UNIT_TESTS AND IS_CFS_ARCH_BUILD)
  add_subdirectory(ut-stubs)
endif (ENABLE_UNIT_TESTS AND IS_CFS_ARCH_BUILD)
