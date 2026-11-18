##################################################################
#
# Sub-script to capture the table compile/generation environment
#
# This small script runs at build time (as opposed to prep time)
# which captures a set of environment metadata
#
# It must be done this way such that generator expressions will
# be evaluated now, during the arch build process, rather than
# deferring the evaluation to the parent build where they may
# have different values.
#
##################################################################

set(STAGING_DIR staging ${TARGET_NAME} ${INSTALL_SUBDIR})
string(REPLACE ";" "/" STAGING_DIR  "${STAGING_DIR}")

# In case the INCLUDE_DIRS is a CMake list, convert to a space-separated list
list(REMOVE_DUPLICATES INCLUDE_DIRS)
string(REPLACE ";" " " INCLUDE_DIRS "${INCLUDE_DIRS}")

list(REMOVE_DUPLICATES COMPILE_DEFS)
string(REPLACE ";" " " COMPILE_DEFS "${COMPILE_DEFS}")

set(TABLE_BINARY "${STAGING_DIR}/${TABLE_NAME}.tbl")
set(TMP_DIR "eds/${TARGET_NAME}")
set(TABLE_RULES)

foreach(TBL_SRC ${SOURCES})

    if ($ENV{VERBOSE})
        message(STATUS "Running generate_eds_table_rules on ${TBL_SRC}")
    endif()

    if (TBL_SRC MATCHES "\\.c$")
        # Legacy C source files need a compile step before they can be used
        set(TABLE_LEGACY_FILE "${TMP_DIR}/${APP_NAME}_${TABLE_NAME}")
        string(APPEND TABLE_RULES
            "${TABLE_LEGACY_FILE}.o: C_INCLUDE_DIRS += ${INCLUDE_DIRS}\n"
            "${TABLE_LEGACY_FILE}.o: C_COMPILE_DEFS += ${COMPILE_DEFS}\n"
            "${TABLE_LEGACY_FILE}.source: ${TBL_SRC}\n"
            "\n")
        set(DEP_FILE "${TABLE_LEGACY_FILE}.so")
    else()
        # Other source files (e.g. Lua) can be used directly by the tool
        set(DEP_FILE "${TBL_SRC}")
    endif()

    string(APPEND TABLE_RULES
        "${TABLE_BINARY}: ${DEP_FILE}\n"
        "\n"
    )

endforeach()

configure_file(${TEMPLATE_FILE} ${OUTPUT_FILE})
