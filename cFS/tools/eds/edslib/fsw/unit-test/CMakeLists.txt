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

##################################################################
#
# functional test build recipe for EdsLib APIs
#
##################################################################

enable_testing()

# The EDS tool is in the same repo, so the relative path should be consistent
include (../../../tool/cmake/edstool-execute-target.cmake)

# Scrub the source list (adjust pathnames)
set(UT_EDSLIB_SOURCES)
foreach(SRC ${EDSLIB_BASE_SOURCES} ${EDSLIB_RUNTIME_SOURCES} src/edslib_init.c)
  list(APPEND UT_EDSLIB_SOURCES "../${SRC}")
endforeach()

# Compile the source unit(s) under test as a separate library
# This is done so that special coverage-specific compile flags can be used on these files
add_library(ut_edslib OBJECT ${UT_EDSLIB_SOURCES})
add_library(ut_eds_vectors STATIC
  edslib_packed_vectors.c
  edslib_native_vectors.c
)

add_dependencies(ut_eds_vectors edstool-execute-ut)

# Include the same set of include dirs/definitions that is used from the app target
target_include_directories(ut_edslib PUBLIC
    $<TARGET_PROPERTY:edslib_runtime_static,INCLUDE_DIRECTORIES>
)
target_compile_definitions(ut_edslib PUBLIC
    $<TARGET_PROPERTY:edslib_runtime_static,COMPILE_DEFINITIONS>
)

target_link_libraries(ut_edslib PRIVATE ut_coverage_compile)

foreach (PART global datatypedb displaydb intfdb pack unpack)

  # Add executable
  add_executable(functional-edslib-${PART}-testrunner
    edslib_test_common.c
    edslib_${PART}_test.c
    $<TARGET_OBJECTS:ut_edslib>
  )

  # Include the same set of include dirs/definitions that is used from the app target
  target_include_directories(functional-edslib-${PART}-testrunner PUBLIC
    $<TARGET_PROPERTY:edslib_runtime_static,INCLUDE_DIRECTORIES>
  )
  target_compile_definitions(functional-edslib-${PART}-testrunner PUBLIC
    $<TARGET_PROPERTY:edslib_runtime_static,COMPILE_DEFINITIONS>
  )

  target_compile_features(functional-edslib-${PART}-testrunner PUBLIC c_std_99)

  # Add it to the set of tests to run as part of "make test"
  add_test(functional-edslib-${PART} functional-edslib-${PART}-testrunner)

  # Install the executables to a staging area for test in cross environments
  if (DEFINED INSTALL_TARGET_LIST)
    foreach(TGT ${INSTALL_TARGET_LIST})
        install(TARGETS functional-edslib-${PART}-testrunner DESTINATION ${TGT})
    endforeach()
  endif()

  target_link_libraries(functional-edslib-${PART}-testrunner PUBLIC
    ut_coverage_link
    ut_eds_db
    ut_assert
  )
endforeach()

# The pack and unpack tests need to be linked with the vectors
target_link_libraries(functional-edslib-unpack-testrunner PUBLIC ut_eds_vectors)
target_link_libraries(functional-edslib-pack-testrunner PUBLIC ut_eds_vectors)