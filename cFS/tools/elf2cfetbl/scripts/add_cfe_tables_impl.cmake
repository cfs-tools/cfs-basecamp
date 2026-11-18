##################################################################
#
# FUNCTION: do_add_cfe_tables_impl
#
# Simplified routine to add CFS tables to be built with an app
#
# For apps with just a single table, the TABLE_FQNAME may be the
# same as the app name, which is simple.
#
# For apps with multiple tables, the TABLE_FQNAME may be of the
# form "${ADDTBL_ARG_APP_NAME}.${TABLE_NAME}" where ${ADDTBL_ARG_APP_NAME} refers to an
# app target that was registered via the "add_cfe_app" function.
#
# Note that for backward compatibility, any name will be accepted
# for TABLE_FQNAME.  However if this function cannot determine which
# app the table is associated with, it will only have a default set
# of INCLUDE_DIRECTORIES when building the C source file(s).  By
# associating a table with an app using the conventions above, the
# INCLUDE_DIRECTORIES from the parent app will be used when building the
# tables.
#
# This function produces one or more library targets in CMake, using names
# of the form: "tblobj_${ADDTBL_ARG_TARGET_NAME}_{TABLE_FQNAME}" where TGT reflects the name
# of the target from targets.cmake and TABLE_FQNAME reflects the first
# parameter to this function.
#

# This interface target gives a hook where the user can add any other compile
# definitions or build options to the tables, by calling target_compile_definitions
# or target_include_directories, etc.
add_library(cfe-platform-table-build INTERFACE)

function(do_add_cfe_tables_impl TABLE_FQNAME)

    cmake_parse_arguments(ADDTBL_ARG "" "APP_NAME;TARGET_NAME;INSTALL_SUBDIR" "" ${ARGN})

    set(TEMPLATE_FILE   "${CFS_TABLETOOL_SCRIPT_DIR}/table_rule_template.d.in")
    set(TABLE_GENSCRIPT "${CFS_TABLETOOL_SCRIPT_DIR}/generate_elf_table_rules.cmake")
    set(TABLE_LIBNAME   "tblobj_${ADDTBL_ARG_TARGET_NAME}_${TABLE_FQNAME}")

    # determine processor ID and spacecraft ID to use for table.
    # If these are not defined, pass 0 instead of a blank (elf2cfetbl will error on non-integer)
    if (DEFINED ${ADDTBL_ARG_TARGET_NAME}_PROCESSORID)
        set(TABLE_PRID ${${ADDTBL_ARG_TARGET_NAME}_PROCESSORID})
    else()
        set(TABLE_PRID 0)
    endif()

    if (DEFINED SPACECRAFT_ID)
        set(TABLE_SCID ${SPACECRAFT_ID})
    else()
        set(TABLE_SCID 0)
    endif()

    set(TABLE_CMD_OPTS
      -DTEMPLATE_FILE="${TEMPLATE_FILE}"
      -DTARGET_SCID="${TABLE_SCID}"
      -DTARGET_PRID="${TABLE_PRID}"
      -DAPP_NAME="${ADDTBL_ARG_APP_NAME}"
      -DTARGET_NAME="${ADDTBL_ARG_TARGET_NAME}"
      -DARCHIVE_FILE="\"$<TARGET_FILE:${TABLE_LIBNAME}>\""
    )

    if (ADDTBL_ARG_INSTALL_SUBDIR)
      list(APPEND TABLE_CMD_OPTS
        -DINSTALL_SUBDIR="${ADDTBL_ARG_INSTALL_SUBDIR}"
      )
    endif()

    # If there is an ${ADDTBL_ARG_APP_NAME}.table target defined, make
    # things depend on that.  Otherwise just depend on core_api.
    set(TABLE_DEPENDENCIES core_api)
    if (TARGET ${ADDTBL_ARG_APP_NAME}.table)
        list(APPEND TABLE_DEPENDENCIES ${ADDTBL_ARG_APP_NAME}.table)
    endif()

    # Note that the file list passed in are just a default - we now need
    # to find the active source, which typically comes from the MISSION_DEFS dir.
    # The TABLE_SELECTED_SRCS will become this list of active/selected source files
    set(TABLE_SELECTED_SRCS)
    foreach(TBL ${ADDTBL_ARG_UNPARSED_ARGUMENTS})

        # The file source basename (without directory or ext) should be the same as the table
        # binary filename with a ".tbl" extension (this is the convention assumed by elf2cfetbl)
        get_filename_component(TABLE_SRC_NEEDED ${TBL} NAME)
        get_filename_component(TABLE_BASENAME   ${TBL} NAME_WE)
        set(TABLE_RULEFILE "${MISSION_BINARY_DIR}/tables/${ADDTBL_ARG_TARGET_NAME}_${TABLE_FQNAME}.${TABLE_BASENAME}.d")


        # Check if an override exists at the mission level (recommended practice)
        # This allows a mission to implement a customized table without modifying
        # the original - this also makes for easier merging/updating if needed.
        # Note this path list is in reverse-priority order, and only a single file
        # will be end up being selected.
        cfe_locate_implementation_file(TBL_SRC "${TABLE_SRC_NEEDED}"
            FALLBACK_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${TBL}"
            PREFIX "${ADDTBL_ARG_TARGET_NAME}"
            SUBDIR tables
        )

        list(APPEND TABLE_SELECTED_SRCS ${TBL_SRC})

        set_property(SOURCE "${TBL_SRC}" APPEND PROPERTY COMPILE_DEFINITIONS
            CFE_TABLE_NAME=${TABLE_BASENAME}
        )

        # Note the table is not generated directly here, as it may require the native system compiler, so
        # the call to the table tool (elf2cfetbl in this build) is deferred to the parent scope.  Instead, this
        # generates a file that captures the state (include dirs, source files, targets) for use in a future step.
        add_custom_command(
            OUTPUT "${TABLE_RULEFILE}"
            COMMAND ${CMAKE_COMMAND}
                ${TABLE_CMD_OPTS}
                -DOUTPUT_FILE="${TABLE_RULEFILE}"
                -DTABLE_NAME="${TABLE_BASENAME}"
                -DSOURCES="${TBL_SRC}"
                -DOBJEXT="${CMAKE_C_OUTPUT_EXTENSION}"
                -P "${TABLE_GENSCRIPT}"
            WORKING_DIRECTORY
                ${MISSION_BINARY_DIR}/tables
            DEPENDS
                ${TABLE_TEMPLATE}
                ${TABLE_GENSCRIPT}
                ${TABLE_DEPENDENCIES}
          )

          # Add a custom target to generate the config file
          add_custom_target(generate_table_${ADDTBL_ARG_TARGET_NAME}_${ADDTBL_ARG_APP_NAME}_${TABLE_BASENAME}
              DEPENDS "${TABLE_RULEFILE}" ${TABLE_LIBNAME}
          )
          add_dependencies(cfetables generate_table_${ADDTBL_ARG_TARGET_NAME}_${ADDTBL_ARG_APP_NAME}_${TABLE_BASENAME})

    endforeach(TBL ${TBL_DEFAULT_SRC_FILES} ${ARGN})

    # NOTE: On newer CMake versions this should become an OBJECT library which makes this simpler.
    # On older versions one may not reference the TARGET_OBJECTS property from the custom command.
    # As a workaround this is built into a static library, and then the desired object is extracted
    # before passing to elf2cfetbl.  It is roundabout but it works.
    add_library(${TABLE_LIBNAME} STATIC EXCLUDE_FROM_ALL ${TABLE_SELECTED_SRCS})
    target_compile_definitions(${TABLE_LIBNAME} PRIVATE
        CFE_TABLE_BUILD
        CFE_CPU_NAME=${ADDTBL_ARG_TARGET_NAME}
    )
    target_link_libraries(${TABLE_LIBNAME} cfe-platform-table-build ${TABLE_DEPENDENCIES})


endfunction(do_add_cfe_tables_impl)
