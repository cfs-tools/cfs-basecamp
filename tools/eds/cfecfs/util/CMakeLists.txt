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


# This directory provides command-line utilities which are
# useful when debugging CFS applications that use EDS

include_directories(${MISSION_BINARY_DIR}/inc)
include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
include_directories(${EDS_CFECFS_MISSIONLIB_FSW_SOURCE_DIR}/inc)

set(UTIL_LINK_LIBS
    edslib_runtime_static
    cfe_missionlib
    cfe_missionlib_runtime_static
    cfe_edsdb_static
    cfe_missionlib_interfacedb_static
)

# CMake snippet for building cmdUtil

add_executable(cmdutil cmdUtil.c)
target_link_libraries(cmdutil ${UTIL_LINK_LIBS})
install(TARGETS cmdutil DESTINATION host)


# CMake snippet for building EDS tlm decoder tool

add_executable(tlm_decode tlm_decode.c)
target_link_libraries(tlm_decode ${UTIL_LINK_LIBS})
install(TARGETS tlm_decode DESTINATION host)


