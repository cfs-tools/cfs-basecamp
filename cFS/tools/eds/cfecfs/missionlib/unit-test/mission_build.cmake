###########################################################
#
# EDS CFE mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################


# Use only the EDS files under the "validation" subdir at the top level
# This ensures a consistent set of data types so that the EdsLib API can be tested
edstool_find_eds_sources("ut" ${MISSION_BINARY_DIR}
    ${MISSION_SOURCE_DIR}/tools/eds/edslib
    ${MISSION_SOURCE_DIR}/tools/eds/validation
)

# This function will add a target called "edstool-execute-ut"
# However it will not set up a dependency on that target (this is done below)
edstool_setup_toplevel(edstool-execute-ut "ut" ${MISSION_BINARY_DIR})

add_dependencies(mission-prebuild edstool-execute-ut)

