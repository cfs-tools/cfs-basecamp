###########################################################
#
# @TEMPLATE@ platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the @TEMPLATE@ configuration
set(@TEMPLATE@_PLATFORM_CONFIG_FILE_LIST
  @template@_internal_cfg_values.h
  @template@_platform_cfg.h
  @template@_perfids.h
  @template@_msgids.h
  @template@_msgid_values.h
)

generate_configfile_set(${@TEMPLATE@_PLATFORM_CONFIG_FILE_LIST})

