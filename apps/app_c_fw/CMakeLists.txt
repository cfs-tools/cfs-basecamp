project(CFS_APP_C_FW C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/app_inc)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_cfe_app(app_c_fw ${LIB_SRC_FILES})
