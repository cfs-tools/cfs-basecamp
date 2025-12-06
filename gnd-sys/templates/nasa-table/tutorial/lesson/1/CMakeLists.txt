project(CFE_@TEMPLATE@ C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)

#EX1,2,
aux_source_directory(fsw/src APP_SRC_FILES)
aux_source_directory(fsw/tables APP_TABLE_FILES)
#EX1

#EX2,3,
# Create the app module
add_cfe_app(@template@ ${APP_SRC_FILES})
add_cfe_tables(@template@ ${APP_TABLE_FILES})
#EX2

