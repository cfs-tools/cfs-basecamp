#
# LEW-20210-1, Python Ground Station for a Core Flight System with CCSDS Electronic Data Sheets Support
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


# The decision on whether to configure the cFS-EDS-GroundStation
# should be made by the user or parent build script.
option(CONFIGURE_CFS_EDS_GROUNDSTATION  
    "Configure a generic Python GUI module that can be imported into standalone Python code"
    OFF)

if (CONFIGURE_CFS_EDS_GROUNDSTATION)
    
    # The cFS-EDS-GroundStation requires both the EdsLib and CFE_MissionLib Python bindings to be built
    if(${EDSLIB_PYTHON_BUILD_STANDALONE_MODULE} STREQUAL ON AND
       ${CFE_MISSIONLIB_PYTHON_BUILD_STANDALONE_MODULE} STREQUAL ON)
       
            set(PYTHON_FILES cFS-EDS-GroundStation.py
                             GS_Model.py
                             GS_Controller.py
                             utils/cmd_util.py
                             utils/convert_tlm_file.py
                             utils/tlm_decode.py)
    
            set(CFS_EDS_GS_MISSION_NAME ${MISSION_NAME})
            foreach(file ${PYTHON_FILES})
		        configure_file(${file} ${CMAKE_BINARY_DIR}/exe/host/cFS-EDS-GroundStation/${file} @ONLY)
            endforeach()
            
            file(COPY nasa.png README.md DESTINATION ${CMAKE_BINARY_DIR}/exe/host/cFS-EDS-GroundStation/)
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/exe/host/cFS-EDS-GroundStation/output)
    else()
        message([SEND_ERROR] "Turn on EDSLIB_PYTHON_BUILD_STANDALONE_MODULE and CFE_MISSIONLIB_PYTHON_BUILD_STANDALONE_MODULE in CMakeCache.txt")
    endif()

endif (CONFIGURE_CFS_EDS_GROUNDSTATION)
