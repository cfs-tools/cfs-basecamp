###########################################################
#
# CI_LAB platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the CI_LAB configuration
set(CI_LAB_PLATFORM_CONFIG_FILE_LIST
  ci_lab_internal_cfg_values.h
  ci_lab_platform_cfg.h
  ci_lab_perfids.h
  ci_lab_msgids.h
  ci_lab_msgid_values.h
)

generate_configfile_set(${CI_LAB_PLATFORM_CONFIG_FILE_LIST})

