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
# EDS PYTHON BINDINGS
#
# This code works with both Python 2 and Python 3.x
# Version 3.x is preferred.
#
# However some old software may only provide a Python2.7 module
# and as such this needs to be built with version 2.7 for that.
#
# In order to accommodate this the EDSLIB_PYTHON_SEARCH_VERSION
# should be defined in the configuration somewhere (toolchain etc)
# which in turn is passed to pkg-config to find Python.
#
# Alternatively, the toolchain can also directly specify where
# Python headers/libs can be found by setting cachevars directly:
#  PYTHON_FOUND = TRUE
#  PYTHON_INCLUDE_DIRS = directory containing Python headers
#  PYTHON_LIBS = libraries to put on link line (-l switches)
#

project(EDSLIB_PYTHONBINDINGS C)

if(NOT PYTHON_FOUND)

  # By default, try to find Python3>=3.4 if no version specified...
  #  This simplifies building as a simple local library.

  # But do not assume this default when cross-compiling or building CFS,
  # because if the toolchain isn't configured correctly, searching could
  # find the wrong version and break the build
  # (in particular, pkg-config might find the host version instead of
  # the cross version and try to use it)

  # In any sort of cross-compile or CFS build, then the user needs to
  # explicitly do something to enable Python bindings.
  if (NOT DEFINED EDSLIB_PYTHON_SEARCH_VERSION AND
      NOT CMAKE_CROSSCOMPILING AND
      NOT IS_CFS_ARCH_BUILD)
    set(EDSLIB_PYTHON_SEARCH_VERSION python3>=3.4)
  endif()

  if (EDSLIB_PYTHON_SEARCH_VERSION)
    find_edslib_dependency(PYTHON Python.h ${EDSLIB_PYTHON_SEARCH_VERSION})
  endif (EDSLIB_PYTHON_SEARCH_VERSION)

endif(NOT PYTHON_FOUND)

# The final decision on whether to build the standalone module
# should be made by the user or parent build script.
option(EDSLIB_PYTHON_BUILD_STANDALONE_MODULE
    "Build a generic Python module that can be imported into standalone Python code"
    OFF)

# If not found then this module cannot be built, however
# this is an optional feature therefore not a fatal error.
if (NOT PYTHON_FOUND)
    message("    Python support not configured, skipping EdsLib Python bindings")
else ()
    include_directories(inc)
    include_directories(${EDSLIB_FSW_SOURCE_DIR}/inc)
    include_directories(${PYTHON_INCLUDE_DIRS})

    set(EDSLIB_PYTHON_SOURCE_LIST
        src/edslib_python_database.c
        src/edslib_python_databaseentry.c
        src/edslib_python_packedobject.c
        src/edslib_python_buffer.c
        src/edslib_python_accessor.c
        src/edslib_python_base.c
        src/edslib_python_scalar.c
        src/edslib_python_number.c
        src/edslib_python_container.c
        src/edslib_python_array.c
        src/edslib_python_dynamicarray.c
        src/edslib_python_conversions.c
        src/edslib_python_setup.c
    )

    #
    # The "static" library target is always defined but will only be built on demand
    # it should work pretty much the same with any build (with or without CFS)
    #
    # This static library is used when building a custom Python interpreter executable
    # that includes the EdsLib module built-in.  (Otherwise Python would typically use
    # a dynamic module with the "import" logic instead - see the "objects" target instead)
    #
    add_library(edslib_python_static STATIC EXCLUDE_FROM_ALL
        ${EDSLIB_PYTHON_SOURCE_LIST})
    target_link_libraries(edslib_python_static
        ${PYTHON_LIBRARIES})

    #
    # The "edslib_python_pic" library target is the basis for a dynamically-loaded
    # Python modules.
    #
    # This isn't directly built as a complete module here because this code does
    # not include an initialization/entry point function.  Such a function needs to
    # be supplied based on the environment it is being loaded into.
    #
    # This is compiled as position-independent code (PIC) if the toolchain supports it
    #  (if not, then the static version would be used and this isn't needed anyway)
    #
    # Note that the target will be an archive but contain PIC code, so it can be
    # linked into a shared library or module.
    #
    #  - For CFE/CFS this is an init function that returns a CFE status code
    #    This init glue is supplied externally via a separate PyCFS code blob
    #  - For standalone Python then the interpreter has a specific init name it looks for
    #    This depends on whether it is Python 2 or 3 -- see edslib_python_module.c
    #
    # This approach allows one or both of these targets to be built while
    # building the core logic only once, while still creating a single module
    # file at runtime.
    add_library(edslib_python_pic OBJECT EXCLUDE_FROM_ALL
        ${EDSLIB_PYTHON_SOURCE_LIST})
    set_target_properties(edslib_python_pic PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE)

    if (EDSLIB_PYTHON_BUILD_STANDALONE_MODULE)

        #
        # Create a standalone Python module
        #
        # In this mode, the main executable/host environment is expected to be Python
        # So this needs to generate a loadable module that is compatible with
        # the Python "import" function.  This binary needs to be named to match
        # the python module name, and the file needs to be placed somewhere
        # in the Python module path so it is found when running "import".
        #
        # The "edslib_python_module.c" file provides a compatible init function.
        # This is what Python looks for as it loads the module as part of "import"
        #  (The init function is a different signature depending on Python 2 or 3)
        #
        add_library(edslib_python_module MODULE
            src/edslib_python_module.c
            $<TARGET_OBJECTS:edslib_python_pic>
            $<TARGET_OBJECTS:edslib_runtime_pic>
        )

        # Per Python naming conventions, the output file should be called only "EdsLib"
        # to match the name of the module it defines.  It will be installed into a
        # Python-specific subdirectory to avoid name conflicts with the non-Python EdsLib.
        set_target_properties(edslib_python_module PROPERTIES PREFIX "" OUTPUT_NAME "EdsLib")

        install(TARGETS edslib_python_module DESTINATION "lib/python")

    endif (EDSLIB_PYTHON_BUILD_STANDALONE_MODULE)

endif()

