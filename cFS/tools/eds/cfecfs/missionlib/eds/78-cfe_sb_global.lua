--
-- LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
--
-- Copyright (c) 2020 United States Government as represented by
-- the Administrator of the National Aeronautics and Space Administration.
-- All Rights Reserved.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--    http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--


-- -------------------------------------------------------------------------
-- Lua implementation of the "dispatch tables" as part of the CFS/MissionLib
--
-- In this area the EDS processing becomes very specific to CFE/CFS
-- and how this application will actually use it.  The objective is
-- to generate the appropriate data structures for "dispatching"
-- messages from the software bus to the local application.
-- -------------------------------------------------------------------------

local global_sym_prefix = SEDS.get_define("EDSTOOL_PROJECT_NAME")
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"

local dbout = SEDS.output_open(SEDS.to_filename("sb_global_impl.c"))

dbout:write("#include \"edslib_database_types.h\"")
dbout:write("#include \"cfe_missionlib_database_types.h\"")
dbout:write(string.format("#include \"%s\"",SEDS.to_filename("master_index.h")))
dbout:add_whitespace(1)


dbout:write(string.format("extern CFE_MissionLib_TopicId_Entry_t %s_TOPICID_LOOKUP[];", global_sym_prefix))
dbout:add_whitespace(1)

dbout:section_marker("Instance Name Table")
dbout:write("#define DEFINE_TGTNAME(x)       #x,")
dbout:write(string.format("static const char * const %s_INSTANCE_LIST[] =", global_sym_prefix))
dbout:write("{")
dbout:write("#include \"cfe_mission_tgtnames.inc\"")
dbout:write("    NULL")
dbout:write("};")

dbout:section_marker("Summary Object")
dbout:write(string.format("const struct CFE_MissionLib_SoftwareBus_Interface %s_SOFTWAREBUS_INTERFACE =", global_sym_prefix))
dbout:start_group("{")
    dbout:write(string.format(".ParentGD = &%s_DATABASE,", global_sym_prefix))
    dbout:write(string.format(".InstanceList = %s_INSTANCE_LIST,", global_sym_prefix))
    dbout:write(string.format(".NumInstances = (sizeof(%s_INSTANCE_LIST) / sizeof(const char *)) - 1,", global_sym_prefix))
    dbout:write(string.format(".NumTopics = %d,", SEDS.get_define("CFE_MISSION/MAX_TOPICID")))
    dbout:write(string.format(".TopicList = %s_TOPICID_LOOKUP", global_sym_prefix))
dbout:end_group("};")
dbout:add_whitespace(1)

SEDS.output_close(dbout)
