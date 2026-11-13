project(CFE_@TEMPLATE@ C)

set(APP_SRC_FILES
  fsw/src/@template@_app.c
  fsw/src/@template@_cmds.c
  fsw/src/@template@_utils.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/@template@_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/@template@_dispatch.c
  )
endif()

# Create the app module
add_cfe_app(@template@ ${APP_SRC_FILES})

#EX1,3,
target_include_directories(@template@ PUBLIC fsw/inc)

# Add table
add_cfe_tables(@template@ fsw/tables/@template@_tbl.c)
#EX1

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
