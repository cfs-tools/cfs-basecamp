###########################################################
#
# Resource ID mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# In this build the EDS definition of resource ID is used
set(RESOURCEID_HDR_FILE "cfe_resourceid_eds.h")

# Generate the header definition files, use local default for this module)
generate_config_includefile(
    FILE_NAME           "cfe_resourceid_typedef.h"
    FALLBACK_FILE       "${CMAKE_CURRENT_LIST_DIR}/option_inc/${RESOURCEID_HDR_FILE}"
)
