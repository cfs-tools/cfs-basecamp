##################################################################
#
# TESTRUNNER CMake build 
#
# This does not build any code - it only installs the TESTRUNNER scripts
# into the CFS staging directory
#
##################################################################

cmake_minimum_required(VERSION 3.10)
project(EDS_CFS_TESTRUNNER NONE)

set(TESTRUNNER_INSTALL_SYMLINK $ENV{TESTRUNNER_DEBUG} 
  CACHE BOOL "Whether to install library as a symlink for debugging")

# In normal (non-debug) deployment, install the library to the python staging dir
# The debug mode will be handled by the installation script below
if (NOT TESTRUNNER_INSTALL_SYMLINK)
  install(DIRECTORY src/ DESTINATION lib/python/testrunner
    PATTERN __pycache__ EXCLUDE
    PATTERN *.py
  )
endif ()

install(FILES common/cfs_test_connection.py DESTINATION lib/python)
install(PROGRAMS examples/runtest.py DESTINATION host)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/stage_app_tests.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/stage_app_tests.cmake @ONLY)
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/stage_app_tests.cmake)
