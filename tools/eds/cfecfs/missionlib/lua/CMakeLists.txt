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

# Lua bindings for the CFE-EDS mission integration library
project(EDS_CFECFS_MISSIONLIB_LUABINDINGS C)

if (LUALIB_FOUND)
    include_directories(${LUALIB_INCLUDE_DIRS})
    include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
    include_directories(${EDSLIB_LUABINDINGS_SOURCE_DIR}/inc)
    include_directories(${EDS_CFECFS_MISSIONLIB_FSW_SOURCE_DIR}/inc)
    include_directories(inc)

    add_library(cfe_missionlib_lua_softwarebus STATIC
        src/cfe_missionlib_lua_softwarebus.c
    )
    target_link_libraries(cfe_missionlib_lua_softwarebus edslib_lua cfe_missionlib cfe_missionlib_runtime_static)
    add_dependencies(cfe_missionlib_lua_softwarebus edstool-execute)

endif()
