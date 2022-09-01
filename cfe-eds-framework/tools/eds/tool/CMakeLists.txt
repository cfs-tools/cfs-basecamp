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
# CFE/CFS build utility tools makefile
# This will build cFE/CFS EDS generator tools.
#
# These tools require the "expat" library, so the "-dev" or "-devel"
# package for the host machine should be installed.
#
# Note this is built natively on the build machine for use during compilation -
# none of this code is loaded directly onto the target.  The output of these
# tools are linked into the final executables, but there is no dependency
# on expat in the final binaries.
#
project(EDSTOOL C)

add_definitions(-Wall -Werror -std=c99 -pedantic)

# expat is required for the EDS XML conversion tools
# note this is just for the build step - no XML is used on the target (at least for EDS)
find_library(EXPAT_LIB expat)
if (NOT EXPAT_LIB)
    message(FATAL_ERROR "The Expat library is required for the build - install the expat development packages")
endif ()

if (NOT TARGET edslib_runtime_static)
    add_subdirectory("../edslib" edslib)
endif()

if (NOT TARGET edslib_lua)
    message(FATAL_ERROR "Lua bindings are required to execute the EDS tool - install the Lua development packages")
endif()


include_directories(inc)
include_directories(${LUALIB_INCLUDE_DIRS})
include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
include_directories(${EDSLIB_LUABINDINGS_SOURCE_DIR}/inc)

add_executable(sedstool
    src/seds_xmlparser.c
    src/seds_preprocess.c
    src/seds_generic_props.c
    src/seds_checksum.c
    src/seds_tree_node.c
    src/seds_instance_node.c
    src/seds_memreq.c
    src/seds_outputfile.c
    src/seds_plugin.c
    src/seds_user_message.c
    src/seds_tool_main.c)

file(GLOB LUA_CODE src/*.lua)
set_source_files_properties(src/seds_tool_main.c PROPERTIES OBJECT_DEPENDS ${LUA_CODE})

# Note -- the contents of edslib_lua_static need to be linked into this EXE
# The base tool does not use it, but future loadable modules may need it
# the --whole-archive linker flag is for this purpose however this is GNU-specific syntax
set_target_properties(sedstool PROPERTIES ENABLE_EXPORTS TRUE)
target_link_libraries(sedstool
    edslib_lua
    edslib_runtime_static
    ${EXPAT_LIB}
    dl
)

