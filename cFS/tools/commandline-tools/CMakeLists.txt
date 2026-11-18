#
# LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
#
# Copyright (c) 2020 United States Government as represented by
# the Administrator of the National Aeronautics and Space Administration.
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


# This directory provides command-line utilities which are
# useful when debugging CFS applications

include_directories(${MISSION_BINARY_DIR}/inc)

set(UTIL_LINK_LIBS)
set(CMDSEND_SOURCE_LIST
    src/cmd_send.c
    src/send_udp.c
    src/console_utils.c
    src/passthru_encode.c
)

set(TLMRECV_SOURCE_LIST
    src/tlm_recv.c
    src/console_utils.c
    src/passthru_decode.c
)

if (CFE_EDS_ENABLED)

    list(APPEND UTIL_LINK_LIBS
        edslib_runtime_static
        cfe_missionlib
        cfe_missionlib_runtime_static
        cfe_edsdb_static
        cfe_missionlib_sb_dispatchdb_static
    )

    list(APPEND CMDSEND_SOURCE_LIST
        src/eds_encode.c
    )
    list(APPEND TLMRECV_SOURCE_LIST
        src/eds_decode.c
    )

endif()


# CMake snippet for building cmd send/encoder tool
add_executable(cmd_send ${CMDSEND_SOURCE_LIST})
target_compile_definitions(cmd_send PUBLIC _DEFAULT_SOURCE _BSD_SOURCE)
target_link_libraries(cmd_send ${UTIL_LINK_LIBS})
install(TARGETS cmd_send DESTINATION host)


# CMake snippet for building tlm display/decoder tool
add_executable(tlm_recv ${TLMRECV_SOURCE_LIST})
target_link_libraries(tlm_recv ${UTIL_LINK_LIBS})
install(TARGETS tlm_recv DESTINATION host)
