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

cmake_minimum_required(VERSION 2.6)
project(EDSLIB C)

#
# A generic function to locate a library dependency
# This function should set/export three cache variables based on the DEPNAME
#
#   <DEPNAME>_FOUND  is set TRUE if the library is available
#   <DEPNAME>_INCLUDE_DIRS is set to the include directories for that library
#   <DEPNAME>_LIBRARIES is set to the link libraries to use
#
# This first tries to use the pkg-config subsystem to locate the dependency,
# and if that it not available it does a manual search.  pkg-config should be
# pretty reliable on the dev host system, but less reliable for cross-compile.
#
# In a pinch, if the library is in a nonstandard location, the toolchain file
# can set the same cache variables which will skip this search altogether.
#
function(find_edslib_dependency DEPNAME HEADER_FILE)

  # If already found then nothing to do
  if(${DEPNAME}_FOUND)
    return()
  endif(${DEPNAME}_FOUND)

  # The preferred way to find the dependency is to use the pkg-config tool
  if (NOT CMAKE_CROSSCOMPILING)
    # pkg-config should be available and working on the dev host
  elseif (CMAKE_FIND_ROOT_PATH)
    # When cross compiling, point pkg-config at the CMAKE_FIND_ROOT_PATH
    set(ENV{PKG_CONFIG_DIR} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_FIND_ROOT_PATH}/usr/lib/pkgconfig:${CMAKE_FIND_ROOT_PATH}/usr/share/pkgconfig")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_FIND_ROOT_PATH}")
  else()
    # Without a proper sysroot, this cannot locate the library
    # This is not a fatal error because the dependency might not be required
    return()
  endif()

  # First try using pkg-config
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    set(FIND_SOURCE "pkg-config")
    pkg_search_module(${DEPNAME} ${ARGN})
  endif()

  if(NOT ${DEPNAME}_FOUND AND CMAKE_FIND_ROOT_PATH)

    # Fallback method for cross-compilation or when pkgconfig is not available.
    # NOTE: This method does not verify version of the library.  In a cross-compile
    # enviornment it will just use whatever version is present in CMAKE_FIND_ROOT_PATH.

    # Attempt to get the real library name by stripping any version info from the args
    set(BASENAMES)
    foreach(NAME IN LISTS ARGN)
      string(REGEX REPLACE "[<>]=.*$" "" BASENAME ${NAME})
      list(APPEND BASENAMES ${BASENAME})
    endforeach(NAME)
    list(REMOVE_DUPLICATES BASENAMES)

    # NOTE: find_path() does not seem to work well for include files
    # buried in subdirectories.
    #
    # Using a globbing expression instead casts a much wider net, but
    # it is not anticipated that a cross-compile tree would have multiple
    # versions of the same header in its include paths.
    file(GLOB_RECURSE ${DEPNAME}_HEADER_PATH
        RELATIVE "${CMAKE_FIND_ROOT_PATH}/usr/include"
        FOLLOW_SYMLINKS
        "${CMAKE_FIND_ROOT_PATH}/usr/include/*/${HEADER_FILE}")

    find_library(${DEPNAME}_LIBRARY_FILE NAMES ${BASENAMES})
    if (${DEPNAME}_HEADER_PATH AND ${DEPNAME}_LIBRARY_FILE)

      # Set the FOUND output to true
      set(FIND_SOURCE "fallback search")
      set(${DEPNAME}_FOUND TRUE CACHE BOOL "${DEPNAME} library was found" FORCE)

      list(GET ${DEPNAME}_HEADER_PATH 0 HPATH)
      get_filename_component(HPATH ${HPATH} DIRECTORY)

      # If the file does not have any path markers then no need to add any paths
      if (HPATH MATCHES "[A-Za-z]+")
        set(${DEPNAME}_INCLUDE_DIRS "${CMAKE_FIND_ROOT_PATH}/usr/include/${HPATH}"
            CACHE PATH "Include directories for ${DEPNAME}")
      endif()

      # For the library name, get only the value to put in on the link line
      # in general this is the name without the "lib" prefix
      get_filename_component(LIBRARY_NAME ${${DEPNAME}_LIBRARY_FILE} NAME_WE)
      string(REGEX REPLACE "^lib" "" LIBRARY_NAME ${LIBRARY_NAME})
      set(${DEPNAME}_LIBRARIES "${LIBRARY_NAME}"
        CACHE STRING "Libraries for linking with ${DEPNAME}")

    endif ()
  endif()

  if(${DEPNAME}_FOUND)
    message(STATUS "Found ${DEPNAME} version ${${DEPNAME}_VERSION} using ${FIND_SOURCE}")
    if (DEFINED ENV{VERBOSE})
      message("${DEPNAME}_INCLUDE_DIRS=${${DEPNAME}_INCLUDE_DIRS}")
      message("${DEPNAME}_LIBRARIES=${${DEPNAME}_LIBRARIES}")
      message("${DEPNAME}_CFLAGS=${${DEPNAME}_CFLAGS}")
    endif()
  endif()
endfunction (find_edslib_dependency)

# Check if the target system supports the "long double" type.
# if this comes back FALSE then "double" will be used instead.
# This will work but may lose precision during encode/decode ops.
try_compile(EDSLIB_HAVE_LONG_DOUBLE
    ${CMAKE_CURRENT_BINARY_DIR}/configtest_longdouble
    ${CMAKE_CURRENT_SOURCE_DIR}/configtest/edslib_longdouble.c
    LINK_LIBRARIES m
)

# The "fsw" directory contains the runtime library (edslib)
# This is a generic library that is not directly associated with any SEDS DB
# (the association is done at runtime, not at compile time, so it can be built now)
add_subdirectory(fsw)
add_subdirectory(lua)
add_subdirectory(json)
add_subdirectory(python)

