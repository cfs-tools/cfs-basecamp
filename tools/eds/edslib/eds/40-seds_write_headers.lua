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
-- Lua implementation of "write header" EdsLib processing step
--
-- This script generates the C language header files for all EDS-described
-- data types.  Note that this does _not_ attempt to make structures which
-- can directly "overlay" an EDS object for interpretation.  Such an approach
-- is generally not viable, as EDS allows complete flexibility in the binary
-- format (no packing or length rules) but CPUs and compilers have quite a few
-- rules that must be adhered to.
--
-- So while it _is possible_ to write an EDS to match a specific CPU+compiler's
-- representation of a C structure, it is _not possible_ to do the inverse, which
-- is to write a C structure to match an EDS object.
--
-- If the EDS object and the C object do happen to exactly match (because the user
-- authored the EDS with that intent) then the process of conversion can be
-- optimized.
-- -------------------------------------------------------------------------
SEDS.info ("SEDS write headers START")


-- -------------------------------------------------
-- Helper function to write an integer typedef
-- -------------------------------------------------
local function write_c_integer_typedef(output,node)
  local ctype
  local bits
  if (node.resolved_size) then
    bits = 8 * node.resolved_size.bytes
  else
    bits = "max"
  end
  return { ctype = string.format("%s%s_t", node.is_signed and "int" or "uint", bits) }
end

-- -------------------------------------------------
-- Helper function to write a structure typedef
-- -------------------------------------------------
local function write_c_struct_typedef(output,node)
  local checksum = node.resolved_size.checksum
  local struct_name = string.format("struct %s", SEDS.to_safe_identifier(node:get_qualified_name()))

  -- this potentially renders create more than one struct with same checksum.
  -- This just records the first/initial occurrence of this checksum
  if (not output.checksum_table[checksum]) then
    output.checksum_table[checksum] = struct_name
  end

  -- Note that the XML allows one to specify a container/interface with no members.
  -- but C language generally frowns upon structs with no members.  So this check is
  -- in place to avoid generating such code.
  if (#node.decode_sequence > 0) then
    output:add_documentation(string.format("Structure definition for %s \'%s\'", node.entity_type, node:get_qualified_name()),
      "Data definition signature " .. checksum)
    output:write(string.format("%s /* checksum=%s */", struct_name, checksum))
    output:start_group("{")
    for idx,ref in ipairs(node.decode_sequence) do
      local c_name = ref.type:get_flattened_name()
      -- If the entry refers to a base type, then use a buffer instead of a direct instance here,
      -- but only if the derivatives actually do extend the type (i.e. add fields).
      if (ref.name and ref.type.max_size and ref.type.max_size.bits > ref.type.resolved_size.bits) then
        c_name = c_name .. "_Buffer"
      end
      if (ref.entry) then
        output:add_documentation(ref.entry.attributes.shortdescription, ref.entry.longdescription)
      end
      output:write(string.format("%-50s %-30s /* %-3d bits/%-3d bytes */",
        c_name .. "_t",
        (ref.name or ref.type.name) .. ";",
        ref.type.resolved_size.bits,
        ref.type.resolved_size.bytes))
    end
    output:end_group("};")
  end

  return { ctype = struct_name }
end

-- -------------------------------------------------
-- Helper function to write an array typedef
-- -------------------------------------------------
local function write_c_array_typedef(output,node)
  local basetype_name = node.datatyperef:get_flattened_name()
  local basetype_modifier = ""

  if (node.datatyperef.max_size) then
    basetype_name = basetype_name .. "_Buffer"
  end

  for dim in node:iterate_subtree("DIMENSION") do
    if (dim.total_elements) then
      basetype_modifier = string.format("[%d]",dim.total_elements) .. basetype_modifier
    end
  end

  if (basetype_modifier == "" and node.total_elements) then
    basetype_modifier = string.format("[%d]",node.total_elements)
  end

  return { ctype = basetype_name .. "_t" , typedef_modifier = basetype_modifier }
end

-- -------------------------------------------------
-- Helper function to write a floating point typedef
-- -------------------------------------------------
local function write_c_float_typedef(output,node)
  local ctype
  if (node.resolved_size.bits <= 32) then
    ctype = "float"
  elseif (node.resolved_size.bits <= 64) then
    ctype = "double"
  else
    -- "long double" is the largest standard C99 floating point type,
    -- generally the max FPU precision that the machine supports, but
    -- probably not a full IEEE-754 quad.  So there will likely
    -- be some loss of precision when using this feature.
    ctype = "long double"
  end
  return { ctype = ctype }
end

-- -------------------------------------------------
-- Helper function to write a string typedef
-- -------------------------------------------------
local function write_c_string_typedef(output,node)
  -- string objects are just character arrays
  return { ctype = "char", typedef_modifier = string.format("[%d]",node.length) }
end

-- -------------------------------------------------
-- Helper function to write a binary blob typedef
-- -------------------------------------------------
local function write_c_binary_typedef(output,node)
  -- binary objects are just uint8 arrays
  return { ctype = "uint8_t", typedef_modifier = string.format("[%d]",node.resolved_size.bytes) }
end

-- -------------------------------------------------
-- Helper function to write a boolean typedef
-- -------------------------------------------------
local function write_c_bool_typedef(output,node)
  return { ctype = "bool" }
end

-- -------------------------------------------------
-- Helper function to write a subrange typedef
-- -------------------------------------------------
local function write_c_subrange_typedef(output,node)
  -- C does not do subranges, so just use the basetype
  return { ctype = node.basetype:get_flattened_name() .. "_t" }
end

-- -------------------------------------------------
-- Helper function to write an enumeration typedef
-- -------------------------------------------------
local function write_c_enum_typedef(output,node)
  local list = node:find_first("ENUMERATION_LIST")
  local enum_name = string.format("enum %s", SEDS.to_safe_identifier(node:get_qualified_name()))
  local enum_typedef = node:get_flattened_name() .. "_t"
  local enum_min, enum_max
  if (list) then
    output:add_documentation(string.format("Label definitions associated with %s", enum_typedef))
    output:write(enum_name)
    output:start_group("{")
    for ent in list:iterate_children("ENUMERATION_ENTRY") do
      output:append_previous(",")
      output:add_documentation(ent.attributes.shortdescription, ent.longdescription)
      if (not enum_min or enum_min > ent.value) then
        enum_min = ent.value
      end
      if (not enum_max or enum_max < ent.value) then
        enum_max = ent.value
      end
      output:write(string.format("%-50s = %d", ent:get_flattened_name(), ent.value))
    end
    output:end_group("};")
    if (enum_min) then
      output:write(string.format("#define %-50s %d", enum_typedef .. "_MIN", enum_min))
    end
    if (enum_max) then
      output:write(string.format("#define %-50s %d", enum_typedef .. "_MAX", enum_max))
    end
  end
  local typedef = write_c_integer_typedef(output,node)
  typedef.extra_desc = string.format("@sa %s", enum_name)
  return typedef
end

-- -------------------------------------------------
-- Helper function to write an enumeration typedef
-- -------------------------------------------------
local function write_c_define(output,def)
  local value = def.attributes.value
  if (type(value) == "number" or type(value) == "boolean") then
    value = string.format("(%s)", tostring(value))
  elseif (type(value) == "string") then
    value = string.format("%s", value)
  end
  if (value) then
    output:write(string.format("#define %-50s %s", def:get_flattened_name(), value))
  end
end

-- -----------------------------------------------------------------------------------------
--                              Main output routine begins
-- -----------------------------------------------------------------------------------------

-- Handler table for various data type nodes
local datatype_output_handler =
{
  INTEGER_DATATYPE = write_c_integer_typedef,
  CONTAINER_DATATYPE = write_c_struct_typedef,
  ARRAY_DATATYPE = write_c_array_typedef,
  FLOAT_DATATYPE = write_c_float_typedef,
  STRING_DATATYPE = write_c_string_typedef,
  BINARY_DATATYPE = write_c_binary_typedef,
  BOOLEAN_DATATYPE = write_c_bool_typedef,
  SUBRANGE_DATATYPE = write_c_subrange_typedef,
  ENUMERATION_DATATYPE = write_c_enum_typedef,
  PROVIDED_INTERFACE = write_c_struct_typedef
}

local global_sym_prefix = SEDS.get_define("MISSION_NAME")
local global_file_prefix = global_sym_prefix and string.lower(global_sym_prefix) or "eds"
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"

for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do

  local output
  local datasheet_basename = ds:get_flattened_name()
  local enum_basename = datasheet_basename .. "_DATADICTIONARY"

  ds.edslib_refobj_global_index = SEDS.to_safe_identifier(string.format("%s_INDEX_%s", global_sym_prefix, SEDS.to_macro_name(ds.name)))

  -- -----------------------------------------------------
  -- DATASHEET OUTPUT FILE 1: The "typedefs.h" file
  -- -----------------------------------------------------
  -- This contains the all basic typedefs for the EDS data types
  output = SEDS.output_open(SEDS.to_filename("typedefs.h", ds.name),ds.xml_filename)

  -- defined types will be based on the C99 standard fixed-width types
  output:write("#include <stdint.h>")
  output:write("#include <stdbool.h>")

  -- For any local datatypes which depend on a type defined in another datasheet,
  -- generate an appropriate #include statement.  Note this needs to be done in a
  -- consistent order so it is sorted before output
  output.checksum_table = {}
  output.datasheet_name = ds:get_flattened_name()
  for _,dep in ipairs(ds:get_references("datatype")) do
    output:write(string.format("#include \"%s\"", SEDS.to_filename("typedefs.h", dep.name)))
  end

  output:section_marker("Typedefs")
  for node in ds:iterate_subtree() do
    if (node.implicit_basetype) then
      node.header_data = {}
    else
      local datatype_handler = datatype_output_handler[node.entity_type]
      if (type(datatype_handler) == "function") then
        node.header_data = datatype_handler(output,node)
      end
    end
    if (node.header_data) then
      local c_name = node:get_flattened_name() .. (node.header_data.typedef_extrasuffix or "")
      if (node.resolved_size) then
        node.edslib_refobj_local_index = c_name .. "_DATADICTIONARY"
        node.edslib_refobj_initializer = string.format("{ %s, %s }", ds.edslib_refobj_global_index, node.edslib_refobj_local_index)
      end
      node.header_data.typedef_name = c_name .. "_t"

      if (node.implicit_basetype) then
        output:add_documentation("Implicitly created wrapper for " .. tostring(node.implict_basetype))
        output:write(string.format("typedef %-50s %s;", node.implicit_basetype.header_data.typedef_name, node.header_data.typedef_name))
      else
        output:add_documentation(node.attributes.shortdescription,
          (node.longdescription or "") .. "\n" .. (node.header_data.extra_desc or ""))
        output:write(string.format("typedef %-50s %s%s;", node.header_data.ctype, node.header_data.typedef_name, node.header_data.typedef_modifier or ""))
        if (node.resolved_size) then
          output:write(string.format("  /* %s */", tostring(node.resolved_size)))
        end
        output:add_whitespace(1)
      end

      if (node.resolved_size) then
        local packedsize = 0
        local buffname = node:get_flattened_name()
        if (node.max_size) then
          output:write(string.format("union %s_Buffer", buffname))
          output:start_group("{")
          output:write(string.format("%-50s BaseObject;", node.header_data.typedef_name))
          output:write(string.format("%-50s Byte[%d];", "uint8_t", node.max_size.bytes))
          local aligntype = (node.max_size.alignment <= 64) and tostring(node.max_size.alignment) or "max"
          output:write(string.format("%-50s Align%d;", "uint" .. aligntype .. "_t", node.max_size.alignment))
          output:end_group("};")
          output:write(string.format("typedef %-50s %s_Buffer_t;", "union " .. buffname .. "_Buffer", buffname))
          packedsize = node.max_size.bits
        else
          packedsize = node.resolved_size.bits
        end
        if (packedsize == 0) then
          packedsize = 1
        else
          packedsize = math.floor((packedsize + 7) / 8)
        end
        output:write(string.format("typedef %-50s %s_PackedBuffer_t[%d];", "uint8_t", buffname, packedsize))
        output:add_whitespace(1)
      end
    end
  end


  output:section_marker("Components")
  for comp in ds:iterate_subtree("COMPONENT") do
    comp.header_data = write_c_struct_typedef(output,comp)
    if (comp.resolved_size) then
      local c_name = comp:get_flattened_name()
      comp.edslib_refobj_local_index = c_name .. "_DATADICTIONARY"
      comp.edslib_refobj_initializer = string.format("{ %s, %s }", ds.edslib_refobj_global_index,
        comp.edslib_refobj_local_index)
      comp.header_data.typedef_name = c_name .. "_t"
      output:add_documentation(comp.attributes.shortdescription, comp.longdescription)
      output:write(string.format("typedef %-50s %s;", comp.header_data.ctype, comp.header_data.typedef_name))
      output:write(string.format("  /* %s */", tostring(comp.resolved_size)))
      output:add_whitespace(1)
    end
  end

  SEDS.output_close(output)


  -- -----------------------------------------------------
  -- DATASHEET OUTPUT FILE 2: The "defines.h" file
  -- -----------------------------------------------------
  -- This contains the all #define statements from EDS,
  -- and also the "datadictionary" type enumeration which
  -- has one entry per typedef in the previous file.
  output = SEDS.output_open(SEDS.to_filename("defines.h", ds.name), ds.xml_filename)
  output:section_marker("Defines")
  for def in ds:iterate_subtree("DEFINE") do
    write_c_define(output,def)
  end

  output:section_marker("Dictionary Enumeration")
  output:write("enum")
  output:start_group("{")
  output:write(enum_basename .. "_RESERVED,")
  for node in ds:iterate_subtree() do
    if (node.edslib_refobj_local_index) then
      output:write(node.edslib_refobj_local_index .. ",")
    end
  end
  output:write(enum_basename .. "_MAX")
  output:end_group("};")

  SEDS.output_close(output)

  -- -----------------------------------------------------
  -- DATASHEET OUTPUT FILE 3: The "dictionary.h" file
  -- -----------------------------------------------------
  -- This only contains an "extern" definition for the DB objects that
  -- will be instantiated in the source files (generated in a future script)
  local symbol_name = SEDS.to_macro_name(datasheet_basename)

  output = SEDS.output_open(SEDS.to_filename("dictionary.h", ds.name),ds.xml_filename)
  output:write(string.format("extern const struct EdsLib_App_DataTypeDB %s_DATATYPE_DB;", symbol_name))
  output:write(string.format("extern const struct EdsLib_App_DisplayDB %s_DISPLAY_DB;", symbol_name))
  SEDS.output_close(output)

end

-- -----------------------------------------------------
-- GLOBAL HEADER FILE 1: The "master_index.h" file
-- -----------------------------------------------------
-- This generates a master index enumeration with one entry per datasheet
local output = SEDS.output_open(SEDS.to_filename("master_index.h"))

for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  output:write(string.format("#include \"%s\"", SEDS.to_filename("defines.h", ds.name)))
end

output:section_marker("Top Level Index Enum")
output:write("enum")
output:start_group("{")
output:write(string.format("%s_RESERVED_INDEX,", global_sym_prefix))
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  output:write(string.format("%s_INDEX_%s,",global_sym_prefix,SEDS.to_macro_name(ds.name)))
end
output:write(string.format("%s_MAX_INDEX",global_sym_prefix))
output:end_group("};")
output:add_whitespace(1)

output:write(string.format("extern const struct EdsLib_DatabaseObject %s_DATABASE;", global_sym_prefix))

SEDS.output_close(output)

-- -----------------------------------------------------
-- GLOBAL HEADER FILE 2: The "designparameters.h" files
-- -----------------------------------------------------
-- This contains all #define statements from the design parameters XML file(s)
-- Note this is different than a datasheet XML file, and generally intended for
-- parameters that apply across the entire system

local all_designparam_files = {}
local all_designparam_defines = {}

-- Output one "designparamters.h" file per EDS package/namespace
-- Note that this could be split among many XML files, so it requires
-- two passes to first assemble everything and then generate the files.
for dp in SEDS.root:iterate_subtree("DESIGN_PARAMETERS") do
  for pkg in dp:iterate_children("PACKAGE") do
    local ns = all_designparam_defines[pkg.name]
    if (not ns) then
      ns = {}
      all_designparam_defines[pkg.name] = ns
      all_designparam_files[1 + #all_designparam_files] = pkg.name
    end

    for def in pkg:iterate_subtree("DEFINE") do
      ns[1+#ns] = def
    end
  end
end

for _,pkgname in ipairs(all_designparam_files) do
  if (#all_designparam_defines[pkgname] > 0) then
    local dpfile = SEDS.to_filename("designparameters.h", pkgname)
    output = SEDS.output_open(dpfile)
    output:section_marker("Design Parameters")
    for _,def in ipairs(all_designparam_defines[pkgname]) do
      write_c_define(output, def)
    end
    SEDS.output_close(output)
  end
end

-- The final "designparameters.h" file simply summarizes all existing files
output = SEDS.output_open(SEDS.to_filename("designparameters.h"))
output:section_marker("Includes")
for _,pkgname in ipairs(all_designparam_files) do
  if (#all_designparam_defines[pkgname] > 0) then
    output:write(string.format("#include \"%s\"", SEDS.to_filename("designparameters.h", pkgname)))
  end
end
SEDS.output_close(output)

SEDS.info ("SEDS write headers END")
