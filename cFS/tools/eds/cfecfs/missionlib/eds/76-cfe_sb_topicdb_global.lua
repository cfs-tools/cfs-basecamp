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
-- Lua implementation of the TopicID table generator as part of the CFS/MissionLib
--
-- In this area the EDS processing becomes very specific to CFE/CFS
-- and how this application will actually use it.  The objective is
-- to generate the appropriate data structures for "dispatching"
-- messages from the software bus to the local application.
-- -------------------------------------------------------------------------

local global_sym_prefix = SEDS.get_define("EDSTOOL_PROJECT_NAME")
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"

local topicid_table = SEDS.mts_info.mts_component.topicid_table


-- -----------------------------------------------------
-- Main routine starts here
-- -----------------------------------------------------
local dbout = SEDS.output_open(SEDS.to_filename("sb_topicdb_impl.c"))

dbout:write("#include \"edslib_database_types.h\"")
dbout:write("#include \"cfe_missionlib_database_types.h\"")
dbout:write(string.format("#include \"%s\"",SEDS.to_filename("master_index.h")))
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  dbout:write(string.format("#include \"%s\"",SEDS.to_filename("interface.h", ds.name)))
end
dbout:add_whitespace(1)

for tid = 1,SEDS.get_define("CFE_MISSION/MAX_TOPICID") do
  local chain = topicid_table[tid]
  if (chain) then
    local intf = chain.binding.reqintf
    local cmdlist = intf.intf_commands or {}
    local arg_list = {}

    for j,cmd in ipairs(cmdlist) do
      if (cmd.subcommand_arg) then
        local derivlist = cmd.args[cmd.subcommand_arg].type.edslib_derivtable_list
        chain.dispatch_symbol_name = string.format("EDS_DISPATCHTABLE_%s_%s", intf:get_flattened_name(), cmd.refnode.name)
        chain.dispatch_table_size = #derivlist
        dbout:write(string.format("static const CFE_MissionLib_DispatchTable_Entry_t %s[] =",chain.dispatch_symbol_name))
        dbout:start_group("{")
        for _,deriv in ipairs(derivlist) do
          dbout:append_previous(",")
          dbout:start_group("{")
          dbout:write(string.format(".DispatchOffset = offsetof(struct %s, %s)",
            intf:get_ctype_basename("Commands"), deriv.name .. "_" .. cmd.refnode.name))
          dbout:end_group("}")
        end
        dbout:end_group("};")
        dbout:add_whitespace(1)
      end
    end
  end
end

local objname = string.format("%s_TOPICID_LOOKUP", global_sym_prefix)
dbout:write(string.format("const CFE_MissionLib_TopicId_Entry_t %s[%d] =", objname, SEDS.get_define("CFE_MISSION/MAX_TOPICID")))
dbout:start_group("{")
for tid = 1,SEDS.get_define("CFE_MISSION/MAX_TOPICID") do
  local chain = topicid_table[tid]
  if (chain) then
    dbout:append_previous(",")
    dbout:write(string.format("[%d] =", chain.tctm.TopicId))
    dbout:start_group("{")
    dbout:write(string.format(".DispatchStartOffset = offsetof(struct %s,%s),",
      chain.binding.reqintf.mapping_info.dispatchtype,
      chain.binding.reqintf.name))

    if (chain.dispatch_table_size) then
      dbout:write(string.format(".LocalDispatchTable = %s,",chain.dispatch_symbol_name))
      dbout:write(string.format(".LocalDispatchSize = %d,",chain.dispatch_table_size))
    end

    dbout:write(string.format(".InterfaceRefObj = %s", chain.binding.reqintf.edslib_refobj_intfdb_initializer))

    dbout:end_group("}")
  end
end
dbout:end_group("};")
dbout:add_whitespace(1)

SEDS.output_close(dbout)
