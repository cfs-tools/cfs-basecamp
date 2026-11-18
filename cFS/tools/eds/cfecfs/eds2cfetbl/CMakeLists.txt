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

# This utility implements a tool to generate a CFE/CFS table
# data file from an EDS-based table definition
project(EDS2CFETBL C)

include_directories(${LUALIB_INCLUDE_DIRS})
include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
include_directories(${EDSLIB_LUABINDINGS_SOURCE_DIR}/inc)
include_directories(${EDS_CFECFS_MISSIONLIB_FSW_SOURCE_DIR}/inc)
include_directories(${EDS_CFECFS_MISSIONLIB_LUABINDINGS_SOURCE_DIR}/inc)

add_executable(eds2cfetbl eds2cfetbl.c)
add_dependencies(eds2cfetbl edstool-execute)
target_link_libraries(eds2cfetbl
    edslib_lua
    cfe_missionlib_lua_softwarebus
    edslib_runtime_static
    cfe_missionlib_sb_dispatchdb_static
    cfe_edsdb_static
    dl
)
set_target_properties(eds2cfetbl PROPERTIES ENABLE_EXPORTS TRUE)

# Export relevant information so the parent script can invoke the tool
set(CFS_TABLETOOL_SCRIPT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts" CACHE INTERNAL "CFS table tool script directory")

set(TBLTOOL_EXTRA_FLAGS)
if (SIMULATION)
    string(APPEND TBLTOOL_EXTRA_FLAGS " -e SIMULATION=${SIMULATION}")
    if (${SIMULATION} STREQUAL "native")
        string(APPEND TBLTOOL_EXTRA_FLAGS " -p")
    endif()
endif()

add_custom_target(tabletool-execute
    COMMAND $(MAKE)
        CC="${CMAKE_C_COMPILER}"
        CFLAGS="${CMAKE_C_FLAGS}"
        AR="${CMAKE_AR}"
        TBLTOOL="$<TARGET_FILE:eds2cfetbl>"
        TBLTOOL_EXTRA_FLAGS=${TBLTOOL_EXTRA_FLAGS}
        cfetables
    WORKING_DIRECTORY
        "${MISSION_BINARY_DIR}/tables"
    DEPENDS
        mission-cfetables
        eds2cfetbl
)

add_dependencies(mission-all tabletool-execute)
add_dependencies(mission-install tabletool-execute)
add_dependencies(mission-prebuild eds2cfetbl)

install(DIRECTORY ${CMAKE_BINARY_DIR}/tables/staging/ DESTINATION .)
