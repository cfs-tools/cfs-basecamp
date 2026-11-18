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

local components = SEDS.mts_info.component_set
local total_intfs = {}


-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------

local function get_mapping_info(desc, intftype)
  return {
    dispatchid = string.format("%s_%s", desc, intftype.name),
    dispatchtype = intftype:get_ctype_basename("DispatchTable_" .. desc)
  }
end

local function append_list(list_combined, list_in)
  for _,val in ipairs(list_in) do
    list_combined[1 + #list_combined] = val
  end
end

-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------
local function write_intfcmd_data(hdrout,reqintf)
  local ctypedef_name

  if (#reqintf.intf_commands > 0) then
    local cmdstructname = reqintf:get_ctype_basename("Commands")
    hdrout:write(string.format("struct %s", cmdstructname))
    hdrout:start_group("{")
    for _,cmd in ipairs(reqintf.intf_commands) do
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
    ctypedef_name = SEDS.to_ctype_typedef(reqintf, "Commands")
    hdrout:write(string.format("typedef struct %s %s;", cmdstructname, ctypedef_name))
    hdrout:add_whitespace(1)
  end

  return ctypedef_name
end

-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------

local function write_reqintf_data(hdrout,reqintf)

  local intfcmds = {}

  for cmd in reqintf.type:iterate_subtree("COMMAND") do
    local args = {}
    local subcommand_arg

    -- Determine if any of the EDS-defined commands have subcommands
    -- A "subcommand" in this context maps to the traditional CFE commands, where
    -- each one maps to a different FunctionCode value in the secondary header.
    -- NOTE: This is specifically checking for items that follow a pattern of
    -- being derived from the same basetype, and have a ValueConstraint on a field
    -- named "Sec.FunctionCode".  If found, subcommand_arg will be set to the position
    -- in the argument list.  In practice, this value can only be undefined or 1.
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

    intfcmds[1 + #intfcmds] = { refnode = cmd, args = args, subcommand_arg = subcommand_arg }
  end

  -- Export information by saving in the DOM tree
  reqintf.intf_commands = intfcmds
  reqintf.mapping_info.cmdstruct_typedef = write_intfcmd_data(hdrout,reqintf)

  hdrout:add_whitespace(1)

end

-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------
local function write_dispatch_table_cdecl(hdrout, intftype_mapping_info)

  hdrout:add_documentation(string.format("Dispatch table for %s interface", intftype_mapping_info.intftype.name))
  hdrout:write(string.format("struct %s", intftype_mapping_info.dispatchtype))
  hdrout:start_group("{")
  for reqintf in intftype_mapping_info.component:iterate_subtree("REQUIRED_INTERFACE") do
    if (reqintf.type == intftype_mapping_info.intftype) then
      hdrout:add_documentation(string.format("Dispatch table associated with %s interface", reqintf.name))
      hdrout:write(string.format("%-60s %s;", reqintf.mapping_info.cmdstruct_typedef, reqintf.name))
    end
  end
  hdrout:end_group("};")
  hdrout:write(string.format("typedef struct %-50s %s;", intftype_mapping_info.dispatchtype,
    SEDS.to_ctype_typedef(intftype_mapping_info.dispatchtype, true)))

  hdrout:add_whitespace(1)
end

-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------

local function write_interface_header(hdrout,comp)

  local component_intftype_list = {}

  hdrout:section_marker(comp.name .. " Command Handlers")
  local component_intftypes = {}
  local desc = comp:get_flattened_name()
  for reqintf in comp:iterate_subtree("REQUIRED_INTERFACE") do

    -- Only do this on SB functional interfaces
    local intf_type_str = reqintf.type:get_qualified_name()
    if (intf_type_str == "CFE_SB/Telemetry" or intf_type_str == "CFE_SB/Telecommand") then
      -- Export information by saving in the DOM tree
      reqintf.mapping_info = get_mapping_info(desc, reqintf.type)

      -- This adds an "intf_commands" member that is a list of descriptors for the commands on this intf
      -- The descriptor has the following info in it:
      --  refnode : points to the cmd node in the DOM tree
      --  args : a list of arguments to that command (generally always length of 1 in CFE)
      --  subcommand_arg : the index of the argument that has subcommands (derivatives).  Should be 1 for CFE ground commands, nil on everything else.
      write_reqintf_data(hdrout,reqintf)

      -- If this yielded a non-empty list, then merge it with the component_intftypes
      -- This builds the complete set of interface types within this component
      if (#reqintf.intf_commands > 0) then
        local compcmds = component_intftypes[reqintf.type]

        if (not compcmds) then
          compcmds = {}
          component_intftypes[reqintf.type] = compcmds
        end

        -- Append this set of commands to the
        append_list(compcmds, reqintf.intf_commands)
      end
    end
  end

  -- This gets the complete set of interface types in this component in a sorted order
  local sorted_local_intftypes = SEDS.sorted_keys(component_intftypes, function(a,b)
    return a.name < b.name
  end)

  hdrout:section_marker(comp.name .. " Dispatch Tables")
  for intftype in sorted_local_intftypes do

    local intftype_mapping_info = get_mapping_info(desc, intftype)

    intftype_mapping_info.component = comp
    intftype_mapping_info.intftype = intftype

    component_intftype_list[1 + #component_intftype_list] = intftype_mapping_info

    write_dispatch_table_cdecl(hdrout, intftype_mapping_info)

  end

  return component_intftype_list
end

-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------

local function write_dispatch_header(hdrout,entry)

  -- In CFE usage, generally all of the interfaces for the SB that FSW accesses only have one
  -- command, which is a generic indication/activation of that interface.  In this case, the
  -- dispatch invocation can be simplified, it doesn't need an extra indication parameter if
  -- there is just a single one.

  local command_count = SEDS.count_results(entry.intftype:iterate_subtree("COMMAND"))
  local basenode_comp = entry.component:find_parent(SEDS.namespace_filter)
  local basenode_declintf = entry.intftype:find_parent(SEDS.namespace_filter)

  hdrout:start_group(string.format("static inline int32_t EdsDispatch_%s(",entry.dispatchid))
  hdrout:write(string.format("%-65s *Message,", "const CFE_SB_Buffer_t"))
  hdrout:write(string.format("%-65s *DispatchTable","const struct " .. entry.dispatchtype))

  -- Only need the caller to pass in an ID here if there is more than one command defined.
  if (command_count > 1) then
    hdrout:write(string.format("%-65s IndicationId,", "uint16"))
  end
  hdrout:end_group(")")
  hdrout:start_group("{")
  hdrout:start_group("return CFE_EDSMSG_Dispatch(")
  hdrout:write(string.format("EDSLIB_INTF_ID(%s_INDEX_%s, %s),", 
    global_sym_prefix, SEDS.to_macro_name(basenode_declintf.name),
    entry.intftype:get_flattened_name("DECLARATION")))
  hdrout:write(string.format("EDSLIB_INTF_ID(%s_INDEX_%s, %s),", 
    global_sym_prefix, SEDS.to_macro_name(basenode_comp.name),
    entry.component:get_flattened_name("INSTANCE")))
  hdrout:write("Message,")
  hdrout:write("DispatchTable")
  hdrout:end_group(");")
  hdrout:end_group("}")

end

-- -----------------------------------------------------
-- Main routine starts here
-- -----------------------------------------------------
--[[
--]]


for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  local ds_intftype_list = {}
  local hdrout

  hdrout = SEDS.output_open(SEDS.to_filename("interface.h", ds.name),ds.xml_filename)

  hdrout:write(string.format("#include \"%s\"", SEDS.to_filename("datatypes.h", ds.name)))
  hdrout:add_whitespace(1)

  for comp in ds:iterate_subtree("COMPONENT") do
    if (components[comp]) then
      local component_intftype_list = write_interface_header(hdrout,comp)
      append_list(ds_intftype_list, component_intftype_list)
    end
  end

  SEDS.output_close(hdrout)

  hdrout = SEDS.output_open(SEDS.to_filename("dispatcher.h", ds.name), ds.xml_filename)

  hdrout:write(string.format("#include \"edsmsg_dispatcher.h\""))
  hdrout:write(string.format("#include \"edslib_id.h\""))
  hdrout:write(string.format("#include \"%s\"", SEDS.to_filename("interface.h", ds.name)))
  hdrout:add_whitespace(1)

  hdrout:section_marker("Dispatch Function Prototypes")
  for _,entry in ipairs(ds_intftype_list) do
    write_dispatch_header(hdrout, entry)
  end

  SEDS.output_close(hdrout)

  append_list(total_intfs, ds_intftype_list)

end

-- Export the list of all intfs via the "mts_info" in the SEDS global
SEDS.mts_info.total_intfs = total_intfs
