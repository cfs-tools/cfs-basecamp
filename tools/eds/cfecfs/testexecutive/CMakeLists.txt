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

# Functional test (black box) framework

include_directories(${MISSION_BINARY_DIR}/inc)
include_directories(${osal_MISSION_DIR}/src/os/inc)
include_directories(${osal_MISSION_DIR}/ut_assert/inc)
include_directories(${OSAL_INCLUDEDIR})
include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
include_directories(${EDSLIB_LUABINDINGS_SOURCE_DIR}/inc)
include_directories(${EDS_CFECFS_MISSIONLIB_FSW_SOURCE_DIR}/inc)
include_directories(${EDS_CFECFS_MISSIONLIB_LUABINDINGS_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${LUALIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-Wall -Werror)
set(TESTEXEC_INTF_MODULES Remote)

if (DEFINED to_lab_MISSION_DIR AND DEFINED ci_lab_MISSION_DIR)
    list(APPEND TESTEXEC_INTF_MODULES CI_TO_LAB)
endif()

set(TESTEXEC_LIBS
    edslib_runtime_static
    cfe_missionlib
    cfe_missionlib_runtime_static
    cfe_edsdb_static
    cfe_missionlib_interfacedb_static
)

set(TESTEXEC_INTF_SRCFILES)
set(TESTEXEC_INTF_MODLIST)
foreach (MOD ${TESTEXEC_INTF_MODULES})
    string(TOLOWER ${MOD} LMOD)
    list(APPEND TESTEXEC_INTF_SRCFILES modules/${LMOD}_interface.c)
    list(APPEND TESTEXEC_INTF_MODLIST "TESTEXEC_MODULE(${MOD})\n")
endforeach (MOD ${TESTEXEC_INTF_MODULES})
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/testexec_compiledin_modules.h.tmp"
    ${TESTEXEC_INTF_MODLIST})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/testexec_compiledin_modules.h.tmp"
    "${CMAKE_CURRENT_BINARY_DIR}/testexec_compiledin_modules.h")


add_executable(testexec src/testexec.c src/test_interface.c ${TESTEXEC_INTF_SRCFILES})
add_dependencies(testexec edstool-execute)
target_link_libraries(testexec
    ut_bsp
    ut_assert
    osal
    edslib_lua
    cfe_missionlib_lua_softwarebus
    ${TESTEXEC_LIBS}
)

add_executable(testctrl src/testctrl.c)
add_dependencies(testctrl edstool-execute)
target_link_libraries(testctrl
    osal
    ${TESTEXEC_LIBS}
)

install(TARGETS testexec testctrl DESTINATION host)
