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
add_library(${DEP} STATIC
    fsw/src/cfe_msg_init.c
    fsw/src/cfe_msg_msgid.c
    fsw/src/cfe_msg_commonhdr.c

    fsw/src/cfe_msg_sechdr_cmd.c
    fsw/src/cfe_msg_sechdr_tlm.c

    fsw/src/cfe_msg_dispatcher.c
)

target_include_directories(${DEP} PUBLIC fsw/inc)
target_link_libraries(${DEP} PRIVATE core_private)
