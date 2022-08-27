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

# EDS CFE/CFS top level makefile
# There are no targets here directly, they are all in subdirectories

add_subdirectory(missionlib)
add_subdirectory(eds2cfetbl)
add_subdirectory(util)

# the test executive is an optional component to provide a black-box
# style test framework.  It is built using OSAL and UT-Assert.
if (ENABLE_EDS_TESTEXEC)
    add_subdirectory(testexecutive)
endif (ENABLE_EDS_TESTEXEC)
