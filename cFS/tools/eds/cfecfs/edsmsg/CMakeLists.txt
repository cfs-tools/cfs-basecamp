##################################################################
#
# cFE message module CMake build recipe
#
# This CMakeLists.txt adds source files for
# message module included in the cFE distribution.  Selected
# files are built into a static library that in turn
# is linked into the final executable.
#
# Note this is different than applications which are dynamically
# linked to support runtime loading.  The core applications all
# use static linkage.
#
##################################################################

# Module library
add_library(edsmsg STATIC
    fsw/src/edsmsg_init.c
    fsw/src/edsmsg_integrity.c
    fsw/src/edsmsg_msgid.c
    fsw/src/edsmsg_commonhdr.c
    fsw/src/edsmsg_sechdr_cmd.c
    fsw/src/edsmsg_sechdr_tlm.c
    fsw/src/edsmsg_dispatcher.c
)

target_include_directories(edsmsg PUBLIC fsw/inc)
target_link_libraries(edsmsg PRIVATE core_private)

if (ENABLE_UNIT_TESTS)
    add_subdirectory(fsw/ut-stubs)
endif()
