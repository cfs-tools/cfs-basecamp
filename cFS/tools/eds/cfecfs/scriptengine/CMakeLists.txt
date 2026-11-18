project(CFE_SCRIPTENGINE C)

# Create the app module
add_cfe_app(scriptengine
  fsw/src/scriptengine.c
  $<TARGET_OBJECTS:edslib_lua_pic>
  $<TARGET_OBJECTS:cfe_missionlib_lua_softwarebus_pic>
)

target_include_directories(scriptengine PUBLIC fsw/inc)
target_include_directories(scriptengine PRIVATE
  $<TARGET_PROPERTY:edslib_lua_pic,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:cfe_missionlib_lua_softwarebus_pic,INTERFACE_INCLUDE_DIRECTORIES>
)

if (ENABLE_UNIT_TESTS)
  add_subdirectory(ut-stubs)
endif (ENABLE_UNIT_TESTS)

# Install the executables to a staging area for test in cross environments
if (INSTALL_TARGET_LIST)
  foreach(TGT ${INSTALL_TARGET_LIST})
    install(FILES scripts/testscript.lua DESTINATION ${TGT}/${INSTALL_SUBDIR})
  endforeach()
endif()
