###########################################################
#
# CI_LAB mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the CI_LAB configuration
set(CI_LAB_MISSION_CONFIG_FILE_LIST
  ci_lab_fcncode_values.h
  ci_lab_interface_cfg_values.h
  ci_lab_mission_cfg.h
  ci_lab_perfids.h
  ci_lab_msg.h
  ci_lab_msgdefs.h
  ci_lab_msgstruct.h
  ci_lab_topicid_values.h
)

generate_configfile_set(${CI_LAB_MISSION_CONFIG_FILE_LIST})

# The same test script can be utilized on all targets that run this app
add_cfe_app_test(${ci_lab_MISSION_DIR}/tests/ci_lab_test_methods.py)
