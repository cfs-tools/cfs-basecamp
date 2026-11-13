###########################################################
#
# @TEMPLATE@ mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the @TEMPLATE@ configuration
set(@TEMPLATE@_MISSION_CONFIG_FILE_LIST
  @template@_fcncode_values.h
  @template@_interface_cfg_values.h
  @template@_mission_cfg.h
  @template@_perfids.h
  @template@_msg.h
  @template@_msgdefs.h
  @template@_msgstruct.h
  @template@_tbl.h
  @template@_tbldefs.h
  @template@_tblstruct.h
  @template@_topicid_values.h
)

generate_configfile_set(${@TEMPLATE@_MISSION_CONFIG_FILE_LIST})

