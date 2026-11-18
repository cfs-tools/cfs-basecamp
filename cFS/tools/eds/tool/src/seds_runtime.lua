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
-- Lua implementation for SEDS tool basic functions
--
-- This returns a table object that contains a static set of methods that
-- implement the "SEDS" global object in the tool
-- -------------------------------------------------------------------------

local SEDS = {}


-- -----------------------------------------------------------------------
-- NAME CONVERSION AND TRANSLATION FUNCTIONS
-- these functions assist with creating sane identifier names for output files
-- -----------------------------------------------------------------------

-- -----------------------------------------------------------------------
---
-- to_safe_identifier: convert any string into a safe identifier for generated code
--
-- XML and EDS allow a greater variety of characters in names than other
-- languages such as C/C++.  This helper method ensures that the string only
-- contains simple alphanumeric characters.
--
-- This returns a string with any non-alphanumeric character in the input
-- replaced with an underscore.  Note that it does _not_ avoid the possibility
-- of the identifier beginning with a digit.  It is recommended to add an appropriate
-- prefix prior to final use to ensure that the identifier does not have a leading digit.
--
SEDS.to_safe_identifier = function (ident)
  -- Note that gsub actually returns two values.
  -- The extra parentheses here only returns the first
  return (string.gsub(tostring(ident),"[^%w%d]+","_"))
end

-- -----------------------------------------------------------------------
---
-- to_filename: convert any value into a filename for output.
--
-- The convention is to use all lowercase for filenames, and use underscore in place
-- of any non alphanumeric character (with the exception of a . for the extension)
-- The intent is to play nice on filesystems that are not fully cases sensitive, and to
-- avoid issues with shell script quoting that would be required if filenames had spaces.
--
-- This creates a filename of the form:
--    qualifier_eds_filename
--
-- where everything is converted to lowercase.  If qualifier is nil, then the EDSTOOL_PROJECT_NAME
-- global definition is used, and if that is also nil then it is left out entirely.
--
SEDS.to_filename = function(filename, qualifier)

  if (not qualifier) then
    qualifier = SEDS.get_define("EDSTOOL_PROJECT_NAME")
  end

  if (qualifier) then
    qualifier = qualifier .. "_"
  else
    qualifier = ""
  end

  local result = string.lower(qualifier .. "eds_" .. filename)

  -- In order to ensure that it is a safe filename on most typical filesystems,
  -- remove any non alphanumeric character and replace it with an underbar.
  -- This also ensures that it has no embedded spaces, although likely supported by filesystems,
  -- tend to cause quoting nightmares for tools and scripts
  result = string.gsub(result, "[^%w.]+", "_")

  -- Remove leading and trailing underscores as well.
  result = string.gsub(result, "^_+", "")
  result = string.gsub(result, "_+$", "")

  return  result
end

-- -----------------------------------------------------------------------
---
-- to_macro_name: convert any value into a macro name for C source files.
--
-- This function will convert names of the CamelCaseNoUnderscore style into
-- ALL_UPPERCASE_WITH_UNDERSCORES as this is a typical convention for macros
--
-- The logic searches for capitalized letters in the source and prefixes them
-- with an underscore, and converts the entire string to uppercase.  Sequences
-- that have just a single letter are then recombined.
--
SEDS.to_macro_name = function(ident)

  local result = string.gsub(ident, "%W+", "_") -- first scrub non alphanumeric characters
  result = string.gsub(result, "([%u%d])", ".%1") .. "." -- prefix capitalized letters
  
  -- Recombine plural acronyms that get broken up.
  -- For example: ActiveAPs is active action points.
  -- The "A" and "Ps" should not be separated
  result = string.gsub(result, "(%u)%.(%us)", "%1%2")

  result = string.upper(result) -- make ALL_UPPERCASE

  -- find multiple one-letter words, which most likely are some sort
  -- of abbreviation that was already ALLCAPS in the original.
  -- i.e. WriteABCDString would be .WRITE.A.B.C.D.STRING. after above translation
  -- The "ABCD" should be recombined
  -- Tentatively mark these spots by replacing the . with another char (=)

  while(true) do
    local temp = string.gsub(result, "[%.=](%w)%.", "=%1=", 1)

    if (temp == result) then
      -- no replacements made, stop now
      break
    end
    result = temp
  end

  -- Any place that there was a pair of letters, set it back to an underscore
  result = string.gsub(result, "(%w%w)=", "%1_")
  result = string.gsub(result, "=(%w%w)", "_%1")
  -- The remaining = characters can be removed outright
  result = string.gsub(result, "=", "")


  -- scrub non alphanumeric characters again, along with leading or trailing underscores.
  result = string.gsub(result, "%W+", "_")
  result = string.gsub(result, "^_+", "")
  result = string.gsub(result, "_+$", "")

  return result
end

local CTYPEDEF_QUALIFIER_TABLE = {
  GENERIC_TYPE = "Generic_",
  PARAMETER = "Parameter_",
  COMMAND = "Command_",
  ARGUMENT = "Argument_",
  COMPONENT = "Component_",
  VARIABLE = "Variable_",
  DECLARED_INTERFACE = "Interface_",
  PROVIDED_INTERFACE = "Interface_",
  REQUIRED_INTERFACE = "Interface_"
}

-- -------------------------------------------------
-- Helper function to write an integer typedef
-- -------------------------------------------------
SEDS.to_ctype_typedef = function(ref,containment_style)

  local prefix
  local refstr

  -- this assumes if it is a userdata, then it must be a seds_tree_node object.
  -- in theory this could accept anything as long as it implements a "get_qualified_name" method.
  if (type(ref) == "userdata" and ref.entity_type) then
    if (not containment_style) then
      prefix = CTYPEDEF_QUALIFIER_TABLE[ref.entity_type]
    end
    refstr = ref:get_ctype_basename(containment_style)
  else
    refstr = SEDS.to_safe_identifier(ref)
  end

  if (not prefix) then
    -- if containment_style was set, then the qualifier should already be embedded within refstr
    if (containment_style) then
      prefix = ""
    elseif (ref.implicit_basetype) then
      -- There can actually be more than one type on a single entry, so this needs to include
      -- the data type as well as the name to differentiate it and avoid collision.
      prefix = "Implicit_" .. SEDS.to_safe_identifier(ref.entity_type) .. "_"
      prefix = prefix:gsub("_DATATYPE_", "_") -- DATATYPE is redundant
    else
      prefix = "DataType_"
    end
  end

  return "Eds" .. prefix .. refstr .. "_t"
end

SEDS.count_results = function(iterator)

  local count = 0
  for _ in iterator do
    count = 1 + count
  end

  return count
end

SEDS.debug_print_table = function(indent,propval)
    for tk,tv in pairs(propval) do
    print (indent .. "  [" .. tostring(tk) .. "] => " .. type(tv) .. ":" .. tostring(tv))
    if (type(tv) == "table") then
      SEDS.debug_print_table(indent .. "  ", tv)
    end
  end
end


-- -----------------------------------------------------------------------
-- GLOBAL NODE FILTER FUNCTIONS
-- In many cases it is necessary to grep through the SEDS node tree
-- and filter those results to only certain node types.  This defines
-- several commonly-required filter sets as well as a factory to
-- generate a filter function for any arbitrary set of node types
-- -----------------------------------------------------------------------

-- -----------------------------------------------------------------------
---
-- create_nodetype_filter: a filter function factory.
--
-- This will return a callable function that returns true if the nodetype matches
-- the parameter, which can either be a string for a specific node type or
-- a table of acceptable nodetypes.  The returned function is typically
-- passed to an iterator to select matching nodes.
--
SEDS.create_nodetype_filter = function (want_nodetype)
  local node_filtertable = {}
  if (type(want_nodetype) == "table") then
    for k,v in pairs(want_nodetype) do
      node_filtertable[v] = true
    end
  else
    node_filtertable[want_nodetype] = true
  end

  -- If a datatype alias is encountered, then check the type it refers to
  if (node_filtertable["ALIAS_DATATYPE"]) then
    node_filtertable["ALIAS_DATATYPE"] = function(node)
      local tgtype = node.type and node.type.entity_type or "UNKNOWN"
      return node_filtertable[tgtype]
    end
  end

  return function (node)
    local result = node_filtertable[node.entity_type]
    while (type(result) == "function") do
      result = result(node)
    end
    return result
  end
end

-- -----------------------------------------------------------------------
---
-- execute_tool: execute an external tool
--
-- This launches a sub-process (via the Lua "os" module) to run the specified tool
-- with the specified arguments.  The actual tool executable file is expected to be
-- part of the defines supplied on the command line when this tool was executed.
--
SEDS.execute_tool = function(toolname, args)
  local command = SEDS.commandline_defines[toolname]
  local result
  if (not command) then
    error(string.format("Tool \'%s\' is not defined",toolname));
  else
    command = string.format("%s %s", command, args)
    SEDS.info("execute_tool: " .. command)
    result = os.execute(command);
  end
  if (not result) then
    error(string.format("External tool failed: %s", toolname));
  end
end

-- -----------------------------------------------------------------------
---
-- sorted_keys: A general purpose table key iterator that operates in order
--
-- The standard Lua pairs() function does not guarantee the order of keys
-- during an iteration, and may even vary between multiple loops in the same
-- program.  This provides an iterator function that returns the key values
-- from the table in sorted order.  It can be used with a "for" loop to
-- get consistent ordering of the keys in a table.
--
SEDS.sorted_keys = function(tbl,sortfunc)
  local keys = {}
  local k = (type(tbl) == "table") and next(tbl)
  while (k) do
    keys[1+#keys] = k
    k = next(tbl,k)
  end
  k = 0
  table.sort(keys,sortfunc)
  return function()
    k = 1 + k
    return keys[k]
  end
end

-- ----------------------------------------------
-- PREDEFINED FILTER TYPES
-- These are simply common filter functions that may be re-used as needed
-- User code can still create new ones as needed.
-- ----------------------------------------------

-- A filter function that will match seds file root nodes
-- this is the root node in an XML file - in memory DOM has its own root
SEDS.basenode_filter = SEDS.create_nodetype_filter
{
  "DATASHEET",
  "PACKAGEFILE"
}


-- A filter function that will match any concrete SEDS data type
SEDS.concrete_datatype_filter = SEDS.create_nodetype_filter
{
  "ALIAS_DATATYPE",
  "CONTAINER_DATATYPE",
  "ARRAY_DATATYPE",
  "INTEGER_DATATYPE",
  "STRING_DATATYPE",
  "FLOAT_DATATYPE",
  "ENUMERATION_DATATYPE",
  "SUBRANGE_DATATYPE",
  "BINARY_DATATYPE",
  "BOOLEAN_DATATYPE"
}

-- A filter function that will match any SEDS data type, including generics
SEDS.any_datatype_filter = SEDS.create_nodetype_filter
{
  "ALIAS_DATATYPE",
  "CONTAINER_DATATYPE",
  "ARRAY_DATATYPE",
  "INTEGER_DATATYPE",
  "STRING_DATATYPE",
  "FLOAT_DATATYPE",
  "ENUMERATION_DATATYPE",
  "SUBRANGE_DATATYPE",
  "BINARY_DATATYPE",
  "BOOLEAN_DATATYPE",
  "GENERIC_TYPE"
}

SEDS.generictype_filter = SEDS.create_nodetype_filter
{
  "GENERIC_TYPE"
}

-- A filter function that will match only containers (for base types)
SEDS.container_filter = SEDS.create_nodetype_filter
{
  "ALIAS_DATATYPE",
  "CONTAINER_DATATYPE"
}

-- A filter function that will match only datatypes which are suitable for indices
SEDS.index_datatype_filter = SEDS.create_nodetype_filter
{
  "ALIAS_DATATYPE",
  "INTEGER_DATATYPE",
  "ENUMERATION_DATATYPE",
  "SUBRANGE_DATATYPE"
}

-- A filter function that will match any scalar SEDS data type
SEDS.scalar_datatype_filter = SEDS.create_nodetype_filter
{
  "ALIAS_DATATYPE",
  "INTEGER_DATATYPE",
  "STRING_DATATYPE",
  "FLOAT_DATATYPE",
  "ENUMERATION_DATATYPE",
  "SUBRANGE_DATATYPE",
  "BOOLEAN_DATATYPE"
}

-- A filter function that will match only numeric data types
SEDS.numeric_datatype_filter = SEDS.create_nodetype_filter
{
  "ALIAS_DATATYPE",
  "INTEGER_DATATYPE",
  "FLOAT_DATATYPE",
  "SUBRANGE_DATATYPE"
}


-- A filter function that will match only interfaces within the DeclaredInterfaceSet
SEDS.declintf_filter = SEDS.create_nodetype_filter
{
  "DECLARED_INTERFACE"
}

-- A filter function that will match only provided/required interfaces of a component
SEDS.referredintf_filter = SEDS.create_nodetype_filter
{
  "REQUIRED_INTERFACE",
  "PROVIDED_INTERFACE"
}

-- A filter which matches anything that has named sub-entities
SEDS.subentity_filter = SEDS.create_nodetype_filter
{
  "ALIAS_DATATYPE",
  "CONTAINER_DATATYPE",
  "DECLARED_INTERFACE",
  "REQUIRED_INTERFACE",
  "PROVIDED_INTERFACE",
  "COMMAND",
  "COMPONENT"
}

-- A filter which matches anything that has named sub-entities
SEDS.container_entry_filter = SEDS.create_nodetype_filter
{
  "CONTAINER_ENTRY",
  "CONTAINER_FIXED_VALUE_ENTRY",
  "CONTAINER_LENGTH_ENTRY",
  "CONTAINER_PADDING_ENTRY",
  "CONTAINER_LIST_ENTRY",
  "CONTAINER_ERROR_CONTROL_ENTRY"
}

SEDS.command_filter = SEDS.create_nodetype_filter
{
  "COMMAND"
}

SEDS.command_argument_filter = SEDS.create_nodetype_filter
{
  "ARGUMENT"
}

SEDS.instancerule_filter = SEDS.create_nodetype_filter
{
  "INSTANCE_RULE"
}

SEDS.component_filter = SEDS.create_nodetype_filter
{
  "COMPONENT"
}

SEDS.constraint_filter = SEDS.create_nodetype_filter
{
  "VALUE_CONSTRAINT",
  "TYPE_CONSTRAINT",
  "RANGE_CONSTRAINT"
}

SEDS.presence_constraint_filter = SEDS.create_nodetype_filter
{
  "PRESENCE_VALUE_CONSTRAINT",
  "PRESENCE_TYPE_CONSTRAINT",
  "PRESENCE_RANGE_CONSTRAINT"
}

SEDS.namespace_filter = SEDS.create_nodetype_filter
{
  "PACKAGE",
  "NAMESPACE"
}

-- End of file.  Return the table object to the caller
return SEDS
