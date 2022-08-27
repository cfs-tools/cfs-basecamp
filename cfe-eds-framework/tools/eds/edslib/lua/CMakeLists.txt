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

# LUA bindings for EdsLib
project(EDSLIB_LUABINDINGS C)

# This library depends on a Lua library on the system
if(NOT LUALIB_FOUND)

  # By default, try to find any 5.x version if no version specified...
  #  This simplifies building as a simple local library.

  # But do not assume this default when cross-compiling or building CFS,
  # because if the toolchain isn't configured correctly, searching could
  # find the wrong version and break the build
  # (in particular, pkg-config might find the host version instead of
  # the cross version and try to use it)

  # In any sort of cross-compile or CFS build, then the user needs to
  # explicitly do something to enable Lua bindings.

  if (NOT DEFINED EDSLIB_LUA_SEARCH_VERSION AND
      NOT CMAKE_CROSSCOMPILING AND
      NOT IS_CFS_ARCH_BUILD)
    set(EDSLIB_LUA_SEARCH_VERSION lua-5.3 lua5.3 lua-5.2 lua5.2 lua-5.1 lua5.1 lua>=5.1)
  endif()

  if (EDSLIB_LUA_SEARCH_VERSION)
    find_edslib_dependency(LUALIB lua.h ${EDSLIB_LUA_SEARCH_VERSION})
  endif(EDSLIB_LUA_SEARCH_VERSION)

endif(NOT LUALIB_FOUND)


# If not found then this module cannot be built, however
# this is an optional feature therefore not a fatal error.
if (NOT LUALIB_FOUND)
    message("    Lua support not configured, skipping EdsLib Lua bindings")
else (NOT LUALIB_FOUND)
    include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
    include_directories(${LUALIB_INCLUDE_DIRS})
    include_directories(inc)

    add_library(edslib_lua STATIC
        src/edslib_lua_objects.c
    )
    target_link_libraries(edslib_lua ${LUALIB_LIBRARIES})

endif(NOT LUALIB_FOUND)

