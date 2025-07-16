project(CFE_@TEMPLATE@ C)
# This file is from https://github.com/cfs-apps/proxy_sample_app

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)

aux_source_directory(fsw/src APP_SRC_FILES)
aux_source_directory(fsw/tables APP_TABLE_FILES)

# Create the app module
add_cfe_app(@template@ ${APP_SRC_FILES})


