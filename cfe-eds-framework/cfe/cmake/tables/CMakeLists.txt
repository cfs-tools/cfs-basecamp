# the passed-in argument should be the name of a generated config file
# with the detail of table name and definition/source files
# because this is inside a function all vars defined here get function scope

include(${ARCH_BINARY_DIR}/table_configs/${TBLCFG}.cmake)

set(TABLE_BINARY "${TABLE_CPUNAME}_${TABLE_BASENAME}")
set(TABLE_DEPS)
set(TABLE_CMDLINE)

if (TABLE_CSRC)
    add_library(${TBLCFG}_legacy SHARED ${TABLE_CSRC})
    target_include_directories(${TBLCFG}_legacy PRIVATE ${TABLE_INCLUDES})
    add_dependencies(${TBLCFG}_legacy mission-prebuild)
    list(APPEND TABLE_DEPS ${TBLCFG}_legacy)
    list(APPEND TABLE_CMDLINE $<TARGET_FILE:${TBLCFG}_legacy>)
endif()

list(APPEND TABLE_DEPS ${TABLE_LUASRC})
list(APPEND TABLE_CMDLINE ${TABLE_LUASRC})

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TABLE_BASENAME}.tbl"
    COMMAND eds2cfetbl
        -e MISSION_DEFS=\"${MISSION_DEFS}\"
        -e CPUNAME=\"${TABLE_CPUNAME}\"
        -e APPNAME=\"${TABLE_APPNAME}\"
        -e TABLENAME=\"${TABLE_BASENAME}\"
        ${TABLE_CMDLINE}
    DEPENDS eds2cfetbl ${TABLE_DEPS}
    VERBATIM
)

add_custom_target(${TABLE_CPUNAME}_${TABLE_BASENAME}_tbl ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${TABLE_BASENAME}.tbl")

# Create the install targets for all the tables
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${TABLE_BASENAME}.tbl" DESTINATION "${TABLE_INSTDIR}")
