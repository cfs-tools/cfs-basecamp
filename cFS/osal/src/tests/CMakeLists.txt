#
# CMake build snippet for OSAL tests
#

# Indicates that this should output a ctest script
enable_testing()

# Each test module is stored within its own subdir
file(GLOB OSAL_TESTS *-test)

# if using the address sanitizer tool, this can affect some aspects
# of the test cases - in particular where things appear in memory - 
# and this can sometimes cause a test to fail, depending on the context.
if (ENABLE_ASAN)
    add_compile_definitions(OSAL_UT_ASAN_ENABLED)
endif (ENABLE_ASAN)

foreach(OSTEST ${OSAL_TESTS})
  get_filename_component(TESTNAME ${OSTEST} NAME)
  set(TESTFILES)
  aux_source_directory(${OSTEST} TESTFILES)
  add_osal_ut_exe(${TESTNAME} ${TESTFILES})
endforeach(OSTEST ${OSAL_TESTS})
