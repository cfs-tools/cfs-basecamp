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
-- Lua implementation of "write data type db objects" EdsLib processing step
--
-- This creates the C source files containing the objects that map the EDS
-- binary blobs into native C structures and vice versa.
-- -------------------------------------------------------------------------
SEDS.info ("SEDS write interface db objects START")

-- -----------------------------------------------------------------------
-- Merge fields helper (combine src table fields into dest)
-- -----------------------------------------------------------------------
local function merge_fields(dest, src)
  for k,v in pairs(src) do
    dest[k] = v
  end
end

-- -----------------------------------------------------------------------
-- Execute list of output handler functions helper
-- -----------------------------------------------------------------------
local function execute_output_handlers(output, node, handler)
  local fields = {}
  local handler_set

  if (type(handler) == "function") then
    handler_set = { handler }
  elseif (type(handler) == "table") then
    handler_set = handler
  end

  assert(handler_set,"Invalid handler parameter, must be list or function")

  if (node) then
    for _,func in ipairs(handler_set) do
      merge_fields(fields, func(output,node))
    end
  end

  return fields
end

-- -----------------------------------------------------------------------
-- Write fields to C struct output helper
-- -----------------------------------------------------------------------
local function write_fields(output, field_vals)
  output:append_previous(",")
  output:start_group("{")
  for key in SEDS.sorted_keys(field_vals) do
    if (field_vals[key] ~= nil) then
      output:append_previous(",")
      output:write(string.format(".%s = %s", key, field_vals[key]))
    end
  end
  output:end_group("}")
end

-- -----------------------------------------------------------------------
-- Generate fields for a "intf name" object
-- -----------------------------------------------------------------------
-- This is just the user-friendly name of a data type
-- To conserve some space the namespace is stored separately from the type names
local function map_edslib_intfname(output,node)
  local namespace = node:find_parent("PACKAGE")
  local name = node:get_qualified_name()
  namespace = namespace and namespace:get_qualified_name() or ""
  if (string.sub(name,1,1 + string.len(namespace)) == (namespace .. "/")) then
    name = string.sub(name,2 + string.len(namespace))
  else
    namespace = nil
  end
  return {
    Name = string.format("\"%s\"", name),
  }
end

-- -----------------------------------------------------------------------
-- Write command node info helper
-- -----------------------------------------------------------------------
local function write_command_info(output,node)

  local retfields = {}
  local list_content = {}

  retfields["FirstIdx"] = string.format("%s_INTFDB_MAX_DIRECT + %d", output.datasheet_name, output.total_subobj_count)

  -- if no command set, nothing to do here
  if (node) then
    local parent_intf = node:find_parent(SEDS.declintf_filter)
    local entry_obj_name = string.format("EDS_COMMANDLIST_%s",SEDS.to_macro_name(node:get_flattened_name()))

    for cmd in node:iterate_children(SEDS.command_filter) do
      local cmd_fields = { }

      cmd_fields["Name"] = string.format("\"%s\"", cmd.name)
      if (parent_intf) then
        cmd_fields["ParentIdx"] = string.format("%s,", parent_intf:get_flattened_name("DECLARATION"))
      end

      local args = {}
      for arg in cmd:iterate_children("ARGUMENT") do
        local arg_fields = { }
        arg_fields["Name"] = string.format("\"%s\"", arg.name)
        arg_fields["Mode"] = string.format("EdsLib_ArgMode_%s", SEDS.to_macro_name(arg.attributes["mode"] or "UNDEFINED"))
        if (arg.type) then
          arg_fields["RefObj"] = arg.type.edslib_refobj_typedb_initializer
        end
        args[1 + #args] = arg_fields
      end
      if (#args > 0) then
        local arglist_obj_name = string.format("EDS_ARGUMENTLIST_%s",SEDS.to_macro_name(cmd:get_flattened_name()))
        output:write(string.format("static const EdsLib_IntfDB_ArgumentEntry_t %s[] =",arglist_obj_name))
        output:start_group("{")
        for _,arg in ipairs(args) do
          write_fields(output,arg)
        end
        output:end_group("};")
        output:add_whitespace(1)
        cmd_fields["ArgumentList"] = arglist_obj_name
        cmd_fields["ArgumentCount"] = #args
      end
      list_content[1 + #list_content] = cmd_fields
    end

    if (#list_content > 0) then
      output:write(string.format("static const EdsLib_IntfDB_CommandEntry_t %s[] =",entry_obj_name))
      output:start_group("{")
      for _,cmd_fields in ipairs(list_content) do
        write_fields(output,cmd_fields)
      end
      output:end_group("};")
      output:add_whitespace(1)

      retfields["CommandList"] = entry_obj_name
      retfields["CommandCount"] = #list_content

      output.total_subobj_count = output.total_subobj_count + #list_content
    end
  end

  return retfields
end

-- -----------------------------------------------------------------------
-- Write typemap list info helper
-- -----------------------------------------------------------------------
local function write_generic_typemap(output,node)
  local retfields = {}
  local list_content = {}

  if (node) then
    local entry_obj_name = string.format("EDS_GENTYPEMAPLIST_%s",SEDS.to_macro_name(node:get_flattened_name()))

    for typemap in node:iterate_children("GENERIC_TYPE_MAP") do
      local map_fields = { }
      if (typemap.ref) then
        map_fields["GenTypeRef"] = typemap.ref.edslib_refobj_typedb_initializer
      end
      if (typemap.type) then
        map_fields["ActualTypeRef"] = typemap.type.edslib_refobj_typedb_initializer
      end
      list_content[1 + #list_content] = map_fields
    end

    if (#list_content > 0) then
      output:write(string.format("static const EdsLib_IntfDB_GenericTypeMapInfo_t %s[] =",entry_obj_name))
      output:start_group("{")
      for _,objs in ipairs(list_content) do
        write_fields(output,objs)
      end
      output:end_group("};")
      output:add_whitespace(1)

      retfields["GenericTypeMapList"] = entry_obj_name
      retfields["GenericTypeMapCount"] = #list_content
    end

  end

  return retfields
end

-- -----------------------------------------------------------------------
-- Write all required/provided intf detail helper
-- -----------------------------------------------------------------------
local function write_referredintf_details(output,node)
  local list_content = {}

  if (node) then
    local parent_comp = node:find_parent(SEDS.component_filter)
    for intf in node:iterate_children(SEDS.referredintf_filter) do
      local intf_fields = { }
      intf_fields["Name"] = string.format("\"%s\"", intf.name)
      if (parent_comp) then
        intf_fields["ParentIdx"] = string.format("%s,", parent_comp:get_flattened_name("INSTANCE"))
      end
      if (intf.type) then
        intf_fields["IntfTypeRef"] = intf.type.edslib_refobj_intfdb_initializer
      end
      merge_fields(intf_fields, write_generic_typemap(output, intf:find_first("GENERIC_TYPE_MAP_SET")))
      list_content[1 + #list_content] = intf_fields
    end
  end

  return list_content
end

-- -----------------------------------------------------------------------
-- Write component info helper
-- -----------------------------------------------------------------------
local function write_comp_intf_info(output,node)
  local retfields = {}
  local reqintf_list_content
  local provintf_list_content

  retfields["FirstIdx"] = string.format("%s_INTFDB_MAX_DIRECT + %d", output.datasheet_name, output.total_subobj_count)

  reqintf_list_content = write_referredintf_details(output,node:find_first("REQUIRED_INTERFACE_SET"))
  if (#reqintf_list_content > 0) then
    local entry_obj_name = string.format("EDS_REQINTFLIST_%s",SEDS.to_macro_name(node:get_flattened_name()))
    output:write(string.format("static const EdsLib_IntfDB_InterfaceEntry_t %s[] =",entry_obj_name))
    output:start_group("{")
    for _,objs in ipairs(reqintf_list_content) do
      write_fields(output,objs)
    end
    output:end_group("};")
    output:add_whitespace(1)

    retfields["RequiredIntfList"] = entry_obj_name
    retfields["RequiredIntfCount"] = #reqintf_list_content

    output.total_subobj_count = output.total_subobj_count + #reqintf_list_content
  end

  provintf_list_content = write_referredintf_details(output,node:find_first("PROVIDED_INTERFACE_SET"))
  if (#provintf_list_content > 0) then
    local entry_obj_name = string.format("EDS_PROVINTFLIST_%s",SEDS.to_macro_name(node:get_flattened_name()))
    output:write(string.format("static const EdsLib_IntfDB_InterfaceEntry_t %s[] =",entry_obj_name))
    output:start_group("{")
    for _,objs in ipairs(provintf_list_content) do
      write_fields(output,objs)
    end
    output:end_group("};")
    output:add_whitespace(1)

    retfields["ProvidedIntfList"] = entry_obj_name
    retfields["ProvidedIntfCount"] = #provintf_list_content

    output.total_subobj_count = output.total_subobj_count + #provintf_list_content
  end

  return retfields
end


-- -----------------------------------------------------------------------
-- Write detail information about declared intf node
-- -----------------------------------------------------------------------
local function collect_declintf_fields(output,node)

  local fields = {}

  merge_fields(fields, execute_output_handlers(output,node:find_first("COMMAND_SET"),write_command_info))

  return fields
end

-- -----------------------------------------------------------------------
-- Write detail information about component intf node
-- -----------------------------------------------------------------------
local function collect_component_fields(output,node)

  local fields = {}

  merge_fields(fields, execute_output_handlers(output,node,write_comp_intf_info))

  return fields
end

-- -----------------------------------------------------------------------
-- Write all declared intfs
-- -----------------------------------------------------------------------
local function write_declared_intfs(output,setnode)

  local fields = {}

  local declintf_output_handlers =
  {
    map_edslib_intfname,
    collect_declintf_fields
  }

  if (setnode) then
    local declintf_objs = {}

    output:section_marker("Declared Interfaces")
    for node in setnode:iterate_children("DECLARED_INTERFACE") do
      declintf_objs[1 + #declintf_objs] = execute_output_handlers(output, node, declintf_output_handlers);
    end

    if (#declintf_objs > 0) then
      local list_obj_name = string.format("EDS_DECLINTFLIST_%s", setnode:get_flattened_name())
      output:write(string.format("static const EdsLib_IntfDB_DeclIntfEntry_t %s[] =", list_obj_name))
      output:start_group("{")
      for _,objs in ipairs(declintf_objs) do
        write_fields(output, objs)
      end
      output:end_group("};")

      fields["DeclIntfListEntries"] = list_obj_name
      fields["DeclIntfListSize"] = #declintf_objs
    end
  end

  return fields

end

-- -----------------------------------------------------------------------
-- Write all components
-- -----------------------------------------------------------------------
local function write_components(output,setnode)

  local fields = {}

  local component_output_handlers =
  {
    map_edslib_intfname,
    collect_component_fields
  }

  if (setnode) then
    local component_objs = {}

    output:section_marker("Components")
    for comp in setnode:iterate_children("COMPONENT") do
      component_objs[1 + #component_objs] = execute_output_handlers(output, comp, component_output_handlers);
    end

    if (#component_objs > 0) then
      local list_obj_name = string.format("EDSLIST_COMPONENTLIST_%s", setnode:get_flattened_name())
      output:write(string.format("static const EdsLib_IntfDB_ComponentEntry_t %s[] =", list_obj_name))
      output:start_group("{")
      for _,objs in ipairs(component_objs) do
        write_fields(output, objs)
      end
      output:end_group("};")

      fields["ComponentListEntries"] = list_obj_name
      fields["ComponentListSize"] = #component_objs
    end
  end

  return fields

end

-- -----------------------------------------------------------------------
--  Get names for top level datasheets
--  The flattened name is generally used for C macros, it has an EDS_ prefix
-- -----------------------------------------------------------------------
local function get_toplevel_names(ds)

  local ds_name = SEDS.to_macro_name(ds:get_qualified_name())
  local ds_flat_name = SEDS.to_macro_name(ds:get_flattened_name())

  return ds_name, ds_flat_name
end

-- -----------------------------------------------------------------------
--    MAIN ROUTINE BEGINS
-- -----------------------------------------------------------------------

-- Each handler function will be called in order,
-- and each may return a set of fields to add to the output object
local component_output_handlers =
{
  map_edslib_intfname,
  collect_component_fields,
}

local global_sym_prefix = SEDS.get_define("EDSTOOL_PROJECT_NAME")
local global_file_prefix = global_sym_prefix and string.lower(global_sym_prefix) or "eds"
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"

-- --------------------------------------
-- Generate the "intfdb_impl.c" file per datasheet
-- --------------------------------------
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do

  local component_objs = { }
  local ds_fields = { }
  local output = SEDS.output_open(SEDS.to_filename("intfdb_impl.c", ds.name), ds.xml_filename)

  output.total_subobj_count = 0

  local ds_name, ds_flat_name = get_toplevel_names(ds)
  output.datasheet_name = ds_flat_name

  output:write(string.format("#include \"edslib_database_types.h\""))
  output:write(string.format("#include \"%s\"", SEDS.to_filename("master_index.h")))

  merge_fields(ds_fields, execute_output_handlers(output,ds:find_first("DECLARED_INTERFACE_SET"),write_declared_intfs))
  merge_fields(ds_fields, execute_output_handlers(output,ds:find_first("COMPONENT_SET"),write_components))

  output:section_marker("Database Object")
  output:write(string.format("const struct EdsLib_App_IntfDB %s_INTF_DB =", ds_flat_name))
  output:start_group("{")
  for key in SEDS.sorted_keys(ds_fields) do
    if (ds_fields[key] ~= nil) then
      output:append_previous(",")
      output:write(string.format(".%s = %s", key, ds_fields[key]))
    end
  end
  output:end_group("};")

  -- Close the output files
  SEDS.output_close(output)

end

-- -----------------------------------------------
-- GENERATE GLOBAL / MASTER SOURCE FILE
-- -----------------------------------------------
-- This array object has a pointer reference to _all_ generated datasheets
-- It can be linked into tools to easily pull in all database objects
local output = SEDS.output_open(SEDS.to_filename("intfdb_impl.c"))

output:write(string.format("#include \"edslib_database_types.h\""))
output:write(string.format("#include \"%s\"",SEDS.to_filename("master_index.h")))
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  output:write(string.format("#include \"%s\"",SEDS.to_filename("dictionary.h", ds.name)))
end

output:add_whitespace(1)

output:write(string.format("const EdsLib_IntfDB_t EDSLIB_%s_INTFDB_APPTBL[%s_MAX_INDEX] =", global_sym_prefix, global_sym_prefix))
output:start_group("{")
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  local ds_name, ds_flat_name = get_toplevel_names(ds)
  output:append_previous(",");
  output:write(string.format("[%s_INDEX_%s] = &%s_INTF_DB",global_sym_prefix,ds_name,ds_flat_name))
end
output:end_group("};")

SEDS.output_close(output)

SEDS.info ("SEDS write intf objects END")
