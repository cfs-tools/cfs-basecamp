###########################################################
#
# MSG mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

#
# This just makes it so the EDS-generated headers (plus any extra required content)
# is includable from applications via a known name "cfe_msg_hdr.h"
#
# doing this at mission scope (rather than as a target_include_directory) should also
# make it available for inclusion in tools/non-CFE code
#

# Generate the header definition files, use local default for this module)
generate_config_includefile(
    FILE_NAME           "cfe_msg_hdr.h"
    FALLBACK_FILE       "${CMAKE_CURRENT_LIST_DIR}/fsw/inc/cfe_msg_hdr_eds.h"
)

