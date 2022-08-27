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

local global_sym_prefix = SEDS.get_define("MISSION_NAME")
local global_file_prefix = global_sym_prefix and string.lower(global_sym_prefix) or "eds"
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"
local mts_comp = SEDS.root:find_reference("CFE_SB/MTS", "COMPONENT")

local components = {}
local msgid_table = {}
local topicid_table = {}
local intf_commands = {}
local total_intfs = {}
local interface_list = {}
local indication_id = 0

-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------
local function write_c_command_defs(hdrout,desc,cmds)
  if (cmds) then
    hdrout:write("struct " .. desc)
    hdrout:start_group("{")
    for i,cmd in ipairs(cmds) do
      hdrout:start_group(string.format("int32 (*%s_%s)(", cmd.intf.name, cmd.refnode.name))
      for j,arg in ipairs(cmd.args) do
        hdrout:append_previous(",")
        hdrout:write(string.format("const %-50s *%s",arg.type.header_data.typedef_name,arg.name))
      end
      hdrout:end_group(");")
    end
    hdrout:end_group("}")
    hdrout:add_whitespace(1)
  end
end


-- -----------------------------------------------------
-- helper function to determine a complete "interface chain"
-- -----------------------------------------------------
--[[
Each Software Bus routing path should involve a chain of interfaces:
MTS -> PubSub -> (Telecommand|Telemetry) -> Application
--]]

local function get_chain(inst)

  local chain = { }
  local found_chain = false

  local function do_get_chain(inst)
    if (inst) then
      found_chain = (inst.component == mts_comp)
      if (not found_chain) then
        for i,binding in ipairs(inst.required_links) do
          chain[1 + #chain] = binding
          do_get_chain(binding.provinst)
          if (found_chain) then
            break
          end
          chain[#chain] = nil
        end
      end
    end
  end

  do_get_chain(inst)
  return chain[1], chain[2], chain[3]
end

for _,instance in ipairs(SEDS.highlevel_interfaces) do
  for _,binding in ipairs(instance.required_links) do
    local tctm, pubsub = get_chain(binding.provinst)
    if (pubsub) then
      components[instance.component] = true
      local chain = {
        binding = binding,
        tctm = tctm.reqinst.params[binding.provintf.name](),
        pubsub = pubsub.reqinst.params[tctm.provintf.name]()
      }
      local MsgId = chain.pubsub.MsgId
      local TopicId = chain.tctm.TopicId
      -- Build the reverse lookup tables for Message Id and Topic Id
      -- Ensure that there are no conflicts between these indices
      if (msgid_table[MsgId]) then
        local exist_comp = msgid_table[MsgId].binding
        exist_comp.reqintf:error(string.format("MsgId Conflict on MsgId=%04x/TopicId=%d with %s",MsgId,TopicId,binding.reqintf:get_qualified_name()))
      else
        msgid_table[MsgId] = chain
      end
      if (topicid_table[TopicId]) then
        local exist_comp = topicid_table[TopicId].binding
        exist_comp.reqintf:error(string.format("TopicId Conflict on MsgId=%04x/TopicId=%d with %s",MsgId,TopicId,binding.reqintf:get_qualified_name()))
      else
        topicid_table[TopicId] = chain
      end
    end
  end
end

for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  local first_intf = 1 + #total_intfs
  local hdrout = SEDS.output_open(SEDS.to_filename("interface.h", ds.name),ds.xml_filename)
  local cmdcode_list = {}
  hdrout:write(string.format("#include \"%s\"", SEDS.to_filename("typedefs.h", ds.name)))
  hdrout:add_whitespace(1)

  for comp in ds:iterate_subtree("COMPONENT") do
    if (components[comp]) then
      hdrout:section_marker(comp.name .. " Command Handlers")
      local localintfs = {}
      local desc = comp:get_flattened_name()
      for reqintf in comp:iterate_subtree("REQUIRED_INTERFACE") do
        local intfcmds = {}
        for cmd in reqintf.type:iterate_subtree("COMMAND") do
          local args = {}
          local subcommand_arg
          for i,stype,refnode in cmd:iterate_members({ reqintf }) do
            if (not stype or not refnode or stype.entity_type == "GENERIC_TYPE") then
              reqintf:error(string.format("Undefined %s", cmd), refnode)
            else
              args[i] = { type = stype, name = refnode.name }
              if (stype.derivative_decisiontree_map and
                stype.derivative_decisiontree_map.entities and
                stype.derivative_decisiontree_map.entities["Sec.FunctionCode"]) then
                subcommand_arg = i
              end
            end
          end

          intfcmds[1 + #intfcmds] = { intf = reqintf, refnode = cmd, args = args,
              subcommand_arg = subcommand_arg }

          local compcmds = localintfs[reqintf.type]
          if (not compcmds) then
            compcmds = {}
            localintfs[reqintf.type] = compcmds
          end
          compcmds[1 + #compcmds] = intfcmds[#intfcmds]
        end
        reqintf.mapping_info = {
          dispatchid = string.format("%s_%s", desc, reqintf.type.name)
        }
        if (#intfcmds > 0) then
          local cmdstructname = string.format("%s_Commands", reqintf:get_flattened_name())
          hdrout:write(string.format("struct %s", cmdstructname))
          hdrout:start_group("{")
          for _,cmd in ipairs(intfcmds) do
            if (cmd.subcommand_arg) then
              for _,deriv in ipairs(cmd.args[cmd.subcommand_arg].type:get_references("derivatives")) do
                hdrout:add_documentation(string.format("Handler function for %s messages", deriv.name))
                hdrout:start_group(string.format("int32_t (*%s)(",deriv.name .. "_" .. cmd.refnode.name))
                for i,arg in ipairs(cmd.args) do
                  hdrout:append_previous(",")
                  local argtype = (i == cmd.subcommand_arg) and deriv or arg.type
                  hdrout:write(string.format("const %s *%s", argtype.header_data.typedef_name, arg.name))
                end
                hdrout:end_group(");")
              end
            else
              hdrout:start_group(string.format("int32_t (*%s)(",cmd.refnode.name))
              for i,arg in ipairs(cmd.args) do
                hdrout:append_previous(",")
                hdrout:write(string.format("const %s *%s", arg.type.header_data.typedef_name, arg.name))
              end
              hdrout:end_group(");")
            end
          end
          hdrout:end_group("};")
          reqintf.mapping_info.cmdstruct_typedef = cmdstructname .. "_t"
          hdrout:write(string.format("typedef struct %s %s;", cmdstructname, reqintf.mapping_info.cmdstruct_typedef))
          hdrout:add_whitespace(1)
        end
        intf_commands[reqintf] = intfcmds
        for _,cmd in ipairs(intfcmds) do
          if (cmd.subcommand_arg) then
            local argtype = cmd.args[cmd.subcommand_arg].type
            for _,subcommand in ipairs(argtype.edslib_derivtable_list) do
              -- Note that this list is not in value-order, the index relates to the lookup table, not the actual cmdcode value
              -- To get the command code value, need to drill down into the constraint set.  This assumes a single value constraint.
              local cmdname = subcommand:get_flattened_name()
              local constraint = subcommand:find_first("CONSTRAINT_SET")
              if (constraint) then
                constraint = constraint:find_first("VALUE_CONSTRAINT")
              end
              if (constraint) then
                cmdcode_list[constraint.attributes["value"]] = cmdname
              end
            end
          end
        end
        hdrout:add_whitespace(1)
      end
      local sorted_localintfs = {}
      for intf in pairs(localintfs) do
        sorted_localintfs[1 + #sorted_localintfs] = intf
      end
      table.sort(sorted_localintfs, function(a,b)
        return a.name < b.name
      end)

      hdrout:section_marker(comp.name .. " Dispatch Tables")
      for _,intf in ipairs(sorted_localintfs) do
        local output_name = string.format("%s_%s", desc, intf.name)
        total_intfs[1 + #total_intfs] = { component = comp, intf = intf, output_name = output_name }
        hdrout:add_documentation(string.format("Dispatch table for %s %s interface", ds.name, intf.name))
        hdrout:write(string.format("struct %s_DispatchTable", output_name))
        hdrout:start_group("{")
        for reqintf in comp:iterate_subtree("REQUIRED_INTERFACE") do
          if (reqintf.type == intf) then
            hdrout:add_documentation(string.format("Dispatch table associated with %s interface", reqintf.name))
            hdrout:write(string.format("%-60s %s;", reqintf.mapping_info.cmdstruct_typedef, reqintf.name))
          end
        end
        hdrout:end_group("};")
        hdrout:write(string.format("typedef struct %s_DispatchTable %s_DispatchTable_t;", output_name,output_name))
        hdrout:add_whitespace(1)
      end
    end
  end

  SEDS.output_close(hdrout)

  -- Create a "cc.h" header file containing the command/function codes specificed as EDS value constraints
  -- This info was collected above, but is easier to use in C files if not mixed with the interface declarations
  -- It also needs to be put in a reasonable order and the macro names need to be scrubbed/fixed to match
  local all_cmdcodes = {}
  for cc in pairs(cmdcode_list) do
    all_cmdcodes[1 + #all_cmdcodes] = cc
  end
  if (#all_cmdcodes > 0) then
    table.sort(all_cmdcodes)
    hdrout = SEDS.output_open(SEDS.to_filename("cc.h", ds.name),ds.xml_filename)
    for _,cc in ipairs(all_cmdcodes) do
      local cmdname = cmdcode_list[cc]
      local macroname = SEDS.to_macro_name(cmdname)
      -- If the command name ends in "_CMD" then lop it off
      if (string.sub(macroname, -4, -1) == "_CMD") then
        macroname = string.sub(macroname, 1, -5)
      end
      hdrout:add_documentation(string.format("Command code associated with %s_t", cmdname))
      hdrout:write(string.format("#define %-60s %s",  macroname .. "_CC", cc))
    end
    SEDS.output_close(hdrout)
  end

  hdrout = SEDS.output_open(SEDS.to_filename("dispatcher.h", ds.name), ds.xml_filename)
  hdrout:write(string.format("#include \"cfe_msg_dispatcher.h\""))
  hdrout:write(string.format("#include \"%s\"", SEDS.to_filename("interfacedb.h", global_sym_prefix)))
  hdrout:write(string.format("#include \"%s\"", SEDS.to_filename("interface.h", ds.name)))
  hdrout:add_whitespace(1)

  hdrout:section_marker("Dispatch Function Prototypes")
  for i = first_intf,#total_intfs do
    local entry = total_intfs[i]
    hdrout:start_group(string.format("static inline int32_t %s_Dispatch(",entry.output_name))
    hdrout:write(string.format("%-65s IndicationId,", entry.intf:get_flattened_name() .. "_IndicationId_t"))
    hdrout:write(string.format("%-65s *Message,", "const CFE_SB_Buffer_t"))
    hdrout:write(string.format("%-65s *DispatchTable","const " .. entry.output_name .. "_DispatchTable_t"))
    hdrout:end_group(")")
    hdrout:start_group("{")
    hdrout:start_group("return CFE_MSG_EdsDispatch(")
    hdrout:write(string.format("%s,", entry.intf:get_flattened_name() .. "_ID"))
    hdrout:write(string.format("IndicationId - %s,", entry.intf:get_flattened_name() .. "_INDICATION_BASE"))
    hdrout:write(string.format("%s_DISPATCHTABLE_ID,",entry.output_name))
    hdrout:write("Message,")
    hdrout:write("DispatchTable")
    hdrout:end_group(");")
    hdrout:end_group("}")
  end

  SEDS.output_close(hdrout)
end

local hdrout = SEDS.output_open(SEDS.to_filename("interfacedb.h"))

for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  for intftype in ds:iterate_subtree("DECLARED_INTERFACE") do
    interface_list[1 + #interface_list] = intftype
    local command_list = {}
    for cmd in intftype:iterate_subtree("COMMAND") do
      command_list[1 + #command_list] = cmd
    end
    if (#command_list > 0) then
      hdrout:write("typedef enum")
      hdrout:start_group("{")
      indication_id = 1 + indication_id
      hdrout:write(intftype:get_flattened_name() .. "_INDICATION_BASE = " .. indication_id)
      for _,cmd in ipairs(command_list) do
        hdrout:append_previous(",")
        hdrout:write(cmd:get_flattened_name() .. "_ID")
        indication_id = 1 + indication_id
      end
      hdrout:end_group(string.format("} %s;", intftype:get_flattened_name() .. "_IndicationId_t"))
      hdrout:add_whitespace(1)
    end
  end
end

hdrout:write("enum")
hdrout:start_group("{")
hdrout:write(string.format("%s_DISPATCHTABLE_ID_RESERVED = 0,", global_sym_prefix))
for _,entry in ipairs(total_intfs) do
  hdrout:write(entry.output_name .. "_DISPATCHTABLE_ID,")
end
hdrout:write(string.format("%s_DISPATCHTABLE_ID_MAX", global_sym_prefix))
hdrout:end_group("};")
hdrout:add_whitespace(1)

hdrout:write("enum")
hdrout:start_group("{")
hdrout:write(string.format("%s_INTERFACE_ID_RESERVED = 0,", global_sym_prefix))
for _,intftype in ipairs(interface_list) do
  hdrout:write(intftype:get_flattened_name() .. "_ID,")
end
hdrout:write(string.format("%s_INTERFACE_ID_MAX", global_sym_prefix))
hdrout:end_group("};")
hdrout:add_whitespace(1)

hdrout:write(string.format("extern const struct CFE_MissionLib_SoftwareBus_Interface %s_SOFTWAREBUS_INTERFACE;", global_sym_prefix))


SEDS.output_close(hdrout)


local dbout = SEDS.output_open(SEDS.to_filename("interfacedb_impl.c"))

dbout:write("#include \"edslib_database_types.h\"")
dbout:write("#include \"cfe_missionlib_database_types.h\"")
dbout:write(string.format("#include \"%s\"",SEDS.to_filename("interfacedb.h")))
dbout:write(string.format("#include \"%s\"",SEDS.to_filename("master_index.h")))
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  dbout:write(string.format("#include \"%s\"",SEDS.to_filename("interface.h", ds.name)))
end
dbout:add_whitespace(1)

for tid = 1,SEDS.get_define("CFE_MISSION/MAX_TOPICID") do
  local chain = topicid_table[tid]
  if (chain) then
    local intf = chain.binding.reqintf
    local arg_list = {}
    for j,cmd in ipairs(intf_commands[intf]) do
      if (#cmd.args > 0) then
        arg_list[j] = string.format("%s_%s_ARGUMENT_LIST",intf:get_flattened_name(), cmd.refnode.name)
        dbout:write(string.format("static const CFE_MissionLib_Argument_Entry_t %s[] =", arg_list[j]))
        dbout:start_group("{")
        for _,arg in ipairs(cmd.args) do
          dbout:append_previous(",")
          dbout:start_group("{")
          local ds = arg.type:find_parent(SEDS.basenode_filter)
          dbout:write(string.format(".AppIndex = %s,", ds.edslib_refobj_global_index))
          dbout:write(string.format(".TypeIndex = %s", arg.type.edslib_refobj_local_index))
          dbout:end_group("}")
        end
        dbout:end_group("};")
        dbout:add_whitespace(1)
      end
    end

    for j,cmd in ipairs(intf_commands[intf]) do
      if (cmd.subcommand_arg) then
        local derivlist = cmd.args[cmd.subcommand_arg].type.edslib_derivtable_list
        dbout:write(string.format("static const CFE_MissionLib_Subcommand_Entry_t %s_%s_SUBCOMMAND_LIST[] =",
          intf:get_flattened_name(), cmd.refnode.name))
        dbout:start_group("{")
        for _,deriv in ipairs(derivlist) do
          dbout:append_previous(",")
          dbout:start_group("{")
          dbout:write(string.format(".DispatchOffset = offsetof(struct %s_Commands,%s)",
            intf:get_flattened_name(), deriv.name .. "_" .. cmd.refnode.name))
          dbout:end_group("}")
        end
        dbout:end_group("};")
        dbout:add_whitespace(1)
      end
    end

    local cmd_def = string.format("%s_COMMANDS",intf:get_flattened_name())
    dbout:write(string.format("static const CFE_MissionLib_Command_Definition_Entry_t %s[] =",cmd_def))
    dbout:start_group("{")
    for j,cmd in ipairs(intf_commands[intf]) do
      dbout:append_previous(",")
      dbout:start_group("{")
      if (cmd.subcommand_arg) then
        dbout:write(string.format(".SubcommandArg = %d,", cmd.subcommand_arg))
        dbout:write(string.format(".SubcommandCount = %d,",
          #cmd.args[cmd.subcommand_arg].type.edslib_derivtable_list))
        dbout:write(string.format(".SubcommandList = %s_%s_SUBCOMMAND_LIST",
          intf:get_flattened_name(), cmd.refnode.name))
      end
      if (arg_list[j]) then
        dbout:append_previous(",")
        dbout:write(string.format(".ArgumentList = %s", arg_list[j]))
      end
      dbout:end_group("}")
    end
    dbout:end_group("};")
    dbout:add_whitespace(1)
  end
end

local objname = string.format("%s_TOPICID_LOOKUP", global_sym_prefix)
local base_topicid = 1
dbout:write(string.format("static const CFE_MissionLib_TopicId_Entry_t %s[%d] =", objname, SEDS.get_define("CFE_MISSION/MAX_TOPICID") - base_topicid))
dbout:start_group("{")
for tid = 1,SEDS.get_define("CFE_MISSION/MAX_TOPICID") do
  local chain = topicid_table[tid]
  if (chain) then
    dbout:append_previous(",")
    dbout:write(string.format("[%d] =", chain.tctm.TopicId - base_topicid))
    dbout:start_group("{")
    dbout:write(string.format(".DispatchTableId = %s_DISPATCHTABLE_ID,", chain.binding.reqintf.mapping_info.dispatchid))
    dbout:write(string.format(".DispatchStartOffset = offsetof(%s_DispatchTable_t,%s),",
      chain.binding.reqintf.mapping_info.dispatchid,
      chain.binding.reqintf.name))
    dbout:write(string.format(".InterfaceId = %s_ID,", chain.binding.reqintf.type:get_flattened_name()))
    dbout:write(string.format(".TopicName = \"%s\",", chain.binding.reqintf:get_qualified_name()))
    dbout:write(string.format(".CommandList = %s_COMMANDS",chain.binding.reqintf:get_flattened_name()))
    dbout:end_group("}")
  end
end
dbout:end_group("};")
dbout:add_whitespace(1)

dbout:section_marker("Interface Definitions")
for _,intftype in ipairs(interface_list) do
  if (intftype:find_first("COMMAND")) then
    dbout:write(string.format("static const CFE_MissionLib_Command_Prototype_Entry_t %s_BASE_COMMAND_LIST[] =", intftype:get_flattened_name()))
    dbout:start_group("{")
    for cmd in intftype:iterate_subtree("COMMAND") do
      dbout:append_previous(",")
      dbout:start_group("{")
      dbout:write(string.format(".CommandName = \"%s\",", cmd.name))
      local arg_count = 0
      for _ in cmd:iterate_subtree("ARGUMENT") do
        arg_count = 1 + arg_count
      end
      dbout:write(string.format(".NumArguments = %d", arg_count))
      dbout:end_group("}")
    end
    dbout:end_group("};")
    dbout:add_whitespace(1)
  end
end

dbout:write(string.format("static const CFE_MissionLib_InterfaceId_Entry_t %s_INTERFACEID_LOOKUP[] =", global_sym_prefix))
dbout:start_group("{")
for _,intftype in ipairs(interface_list) do
  local command_count = 0
  local intfname = intftype:get_qualified_name()
  for cmd in intftype:iterate_subtree("COMMAND") do
    command_count = 1 + command_count
  end
  dbout:append_previous(",")
  dbout:start_group("{")
  dbout:write(string.format(".NumCommands = %d,", command_count))
  dbout:write(string.format(".InterfaceName = \"%s\",", intfname))
  if (command_count > 0) then
    dbout:write(string.format(".CommandList = %s,", intftype:get_flattened_name() .. "_BASE_COMMAND_LIST"))
  end
  -- Only include topicid lookup info on the TC/TM interfaces
  -- this is a bit of a hack as this doesn't really belong here.
  if (intfname == "CFE_SB/Telecommand" or intfname == "CFE_SB/Telemetry") then
    dbout:write(string.format(".NumTopics = %d,", SEDS.get_define("CFE_MISSION/MAX_TOPICID") - base_topicid))
    dbout:write(string.format(".TopicList = %s_TOPICID_LOOKUP,", global_sym_prefix))
  end
  dbout:end_group("}")
end
dbout:end_group("};")
dbout:add_whitespace(1)

dbout:section_marker("Instance Name Table")
dbout:write("#define DEFINE_TGTNAME(x)       #x,")
dbout:write(string.format("static const char * const %s_INSTANCE_LIST[] =", global_sym_prefix))
dbout:write("{")
dbout:write(string.format("#include \"%s\"",SEDS.to_filename("tgtnames.inc")))
dbout:write("    NULL")
dbout:write("};")

dbout:section_marker("Summary Object")
dbout:write(string.format("const struct CFE_MissionLib_SoftwareBus_Interface %s_SOFTWAREBUS_INTERFACE =", global_sym_prefix))
dbout:start_group("{")
    dbout:write(string.format(".NumInterfaces = %d,", #interface_list))
    dbout:write(string.format(".InterfaceList = %s_INTERFACEID_LOOKUP,", global_sym_prefix))
    dbout:write(string.format(".InstanceList = %s_INSTANCE_LIST", global_sym_prefix))
dbout:end_group("};")
dbout:add_whitespace(1)

SEDS.output_close(dbout)
