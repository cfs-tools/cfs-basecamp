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
SEDS.info ("SEDS write display objects START")

local datasheet_namespace_objs = {}

-- -----------------------------------------------------------------------
-- Generate fields for a "display hint" object
-- -----------------------------------------------------------------------
-- This is intended to provide guidance to a user application in how to display a given object.
--   1) Binary objects may be interpreted as strings
--   2) Integers may actually be enumerations or booleans
--   3) Containers have named members
--   4) Arrays might use an "indextyperef" to name their indices
local function write_c_displayhint_object(output,node)
  local displayhint, displayarg, displayargsz
  if (node.entity_type == "ENUMERATION_DATATYPE") then
    displayhint = "ENUM_SYMTABLE"
    local labels = {}
    local values = {}
    for ent in  node:iterate_subtree("ENUMERATION_ENTRY") do
      labels[1 + #labels]  = ent.name
      values[ent.name] = ent.value
    end
    table.sort(labels)
    displayargsz = #labels
    local table_name = string.format("%s_SYMTABLE", node:get_flattened_name())
    output:write(string.format("static const EdsLib_SymbolTableEntry_t %s[] =", table_name))
    output:start_group("{")
    for _,label in ipairs(labels) do
      output:append_previous(",")
      output:write(string.format("{ .SymValue = %d, .SymName = \"%s\" }", values[label], label))
    end
    output:end_group("};")
    output:add_whitespace(1)
    displayarg = string.format("{ .SymTable = %s }", table_name)
  elseif (node.entity_type == "CONTAINER_DATATYPE" or
          node.entity_type == "DECLARED_INTERFACE" or
          node.entity_type == "PROVIDED_INTERFACE" or
          node.entity_type == "REQUIRED_INTERFACE" or
          node.entity_type == "COMPONENT") then
    if (#node.decode_sequence > 0) then
      local checksum = node.resolved_size.checksum
      local table_name = string.format("%s_%s_NAMETABLE", output.datasheet_name, checksum)
      displayargsz = #node.decode_sequence
      displayarg = string.format("{ .NameTable = %s }", table_name)
      displayhint = "MEMBER_NAMETABLE"
      if (not output.checksum_table[checksum]) then
        output.checksum_table[checksum] = table_name
        output:write(string.format("static const char * const %s[] =", table_name))
        output:start_group("{")
        for idx,info in ipairs(node.decode_sequence) do
          output:append_previous(",")
          if (info.name) then
            output:write(string.format("\"%s\"", info.name))
          else
            output:write("NULL")
          end
        end
        output:end_group("};")
        output:add_whitespace(1)
      end
    end
  elseif(node.entity_type == "BOOLEAN_DATATYPE") then
    displayhint = "BOOLEAN"
  elseif(node.entity_type == "STRING_DATATYPE") then
    displayhint = "STRING"
  elseif(node.entity_type == "BINARY_DATATYPE") then
    displayhint = "BASE64"
  elseif(node.entity_type == "ARRAY_DATATYPE") then
    local indextyperef
    -- Check if the array dimension uses an "indextyperef"
    for dim in node:iterate_subtree("DIMENSION") do
      indextyperef = dim.indextyperef
    end
    if (indextyperef) then
      displayhint = "REFERENCE_TYPE"
      displayarg = string.format("{ .RefObj = %s }", indextyperef.edslib_refobj_initializer)
    end
  end
  return {
    DisplayHint = "EDSLIB_DISPLAYHINT_" .. (displayhint or "NONE"),
    DisplayArg = displayarg,
    DisplayArgTableSize = displayargsz
  }
end

-- -----------------------------------------------------------------------
-- Generate fields for a "display name" object
-- -----------------------------------------------------------------------
-- This is just the user-friendly name of a data type
-- To conserve some space the namespace is stored separately from the type names
local function map_edslib_displayname(output,node)
  local namespace = node:find_parent("PACKAGE")
  local name = node:get_qualified_name()
  namespace = namespace and namespace:get_qualified_name() or ""
  if (string.sub(name,1,1 + string.len(namespace)) == (namespace .. "/")) then
    name = string.sub(name,2 + string.len(namespace))
    if (not datasheet_namespace_objs[namespace]) then
      datasheet_namespace_objs[namespace] = SEDS.to_safe_identifier(namespace) .. "_NAMESPACE"
      output:write(string.format("static const char %s[] = \"%s\";",
        datasheet_namespace_objs[namespace], namespace))
      output:add_whitespace(1)
    end
  else
    namespace = nil
  end
  return {
    Namespace = datasheet_namespace_objs[namespace],
    Name = string.format("\"%s\"", name),
  }
end

-- -----------------------------------------------------------------------
--    MAIN ROUTINE BEGINS
-- -----------------------------------------------------------------------

-- Each handler function will be called in order,
-- and each may return a set of fields to add to the output object
local datatype_output_handlers =
{
  map_edslib_displayname,
  write_c_displayhint_object
}

local global_sym_prefix = SEDS.get_define("MISSION_NAME")
local global_file_prefix = global_sym_prefix and string.lower(global_sym_prefix) or "eds"
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"

-- --------------------------------------
-- Generate the "displaydb_impl.c" file per datasheet
-- --------------------------------------
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do

  local datasheet_objs = { { BasicType = "EDSLIB_BASICTYPE_NONE", DisplayHint = "EDSLIB_DISPLAYHINT_NONE" } }
  local output = SEDS.output_open(SEDS.to_filename("displaydb_impl.c", ds.name), ds.xml_filename)

  output.checksum_table = {}
  output.datasheet_name = ds:get_flattened_name()
  output:write(string.format("#include \"edslib_database_types.h\""))
  output:write(string.format("#include \"%s\"", SEDS.to_filename("master_index.h")))

  output:section_marker("Display Hint Objects")
  for node in ds:iterate_subtree() do
    if (node.edslib_refobj_local_index and node.header_data) then
      local objs = {}
      for i,handler in ipairs(datatype_output_handlers) do
        local fields = handler(output,node)
        for j,value in pairs(fields) do
          objs[j] = value
        end
      end
      datasheet_objs[1 + #datasheet_objs] = objs
    end
  end

  local ds_name = SEDS.to_macro_name(ds.name)

  output:section_marker("Lookup Table")
  output:write(string.format("static const EdsLib_DisplayDB_Entry_t %s_DISPLAYINFO_TABLE[] =", ds_name))
  output:start_group("{")
  for idx,objs in ipairs(datasheet_objs) do
    output:append_previous(",")
    output:start_group("{")
    for i,key in ipairs({ "Namespace", "Name", "DisplayHint", "DisplayArgTableSize", "DisplayArg" }) do
      if (objs[key] ~= nil) then
        output:append_previous(",")
        output:write(string.format(".%s = %s", key, objs[key]))
      end
    end
    output:end_group("}")
  end
  output:end_group("};")

  output:section_marker("Database Object")
  output:write(string.format("const struct EdsLib_App_DisplayDB %s_DISPLAY_DB =", ds_name))
  output:start_group("{")
  output:write(string.format(".EdsName = \"%s\",", ds.name))
  output:write(string.format(".DisplayInfoTable = %s_DISPLAYINFO_TABLE", ds_name));
  output:end_group("};")

  -- Close the output files
  SEDS.output_close(output)

end

-- -----------------------------------------------
-- GENERATE GLOBAL / MASTER SOURCE FILE
-- -----------------------------------------------
-- This array object has a pointer reference to _all_ generated datasheets
-- It can be linked into tools to easily pull in all database objects
local output = SEDS.output_open(SEDS.to_filename("displaydb_impl.c"))

output:write(string.format("#include \"edslib_database_types.h\""))
output:write(string.format("#include \"%s\"",SEDS.to_filename("master_index.h")))
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  output:write(string.format("#include \"%s\"",SEDS.to_filename("dictionary.h", ds.name)))
end

output:add_whitespace(1)

output:write(string.format("const EdsLib_DisplayDB_t %s_DISPLAYDB_APPTBL[%s_MAX_INDEX] =", global_sym_prefix, global_sym_prefix))
output:start_group("{")
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  local ds_name = SEDS.to_macro_name(ds.name)
  output:append_previous(",");
  output:write(string.format("[%s_INDEX_%s] = &%s_DISPLAY_DB",global_sym_prefix,ds_name,ds_name))
end
output:end_group("};")

SEDS.output_close(output)

SEDS.info ("SEDS write display objects END")
