project(CFS_FILE_XFER C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(${osk_c_fw_MISSION_DIR}/fsw/app_inc)
include_directories(${osk_c_fw_MISSION_DIR}/fsw/platform_inc)
include_directories(${osk_c_fw_MISSION_DIR}/fsw/mission_inc)


aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(file_xfer ${APP_SRC_FILES})

