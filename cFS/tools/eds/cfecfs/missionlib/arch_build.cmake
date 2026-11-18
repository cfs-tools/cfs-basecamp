###########################################################
#
# EDS CFE platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# on target architecture builds, the "edstool-execute" target
# will not actually run the tool again.  Instead, it will just build all
# the generated make targets from the parent build for this processor by
# supplying the correct CC/AR tool

file(RELATIVE_PATH EDSTOOL_ARCH_SUBDIR "${MISSION_BINARY_DIR}" "${CMAKE_BINARY_DIR}")

add_custom_target(edstool-execute
    COMMAND "\$(MAKE)"
        BUILD_CONFIG="${BUILD_CONFIG_${TARGETSYSTEM}}"
        EDSTOOL_PROJECT_NAME=${MISSION_NAME}
        O="${EDSTOOL_ARCH_SUBDIR}/obj"
        S="src"
        -f ${missionlib_MISSION_DIR}/cmake/edstool-execute-arch.mk
        all
    WORKING_DIRECTORY
        ${MISSION_BINARY_DIR}
    DEPENDS
        missionlib-runtime-install
)

if (ENABLE_UNIT_TESTS)
  add_custom_target(edstool-execute-ut
    COMMAND "\$(MAKE)"
        BUILD_CONFIG="${BUILD_CONFIG_${TARGETSYSTEM}}"
        EDSTOOL_PROJECT_NAME="ut"
        O="${EDSTOOL_ARCH_SUBDIR}/obj"
        S="src"
        -f ${missionlib_MISSION_DIR}/cmake/edstool-execute-arch.mk
        db_objects
    WORKING_DIRECTORY
        ${MISSION_BINARY_DIR}
    DEPENDS
        missionlib-runtime-install
  )
endif()
