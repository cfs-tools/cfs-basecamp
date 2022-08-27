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
-- Lua implementation for SEDS tool DOM tree node methods
--
-- This returns a table object that contains a static set of methods
-- for all tree nodes in the generated DOM.
-- -------------------------------------------------------------------------

SEDS.info "loading SEDS tree methods"


-- -------------------------------------------------------------------------
-- LOCAL CONSTANTS
-- these are used locally but are not exposed outside of this source file
-- -------------------------------------------------------------------------

-- Suffix table for use with the "get_flattened_name" function below.
local FLAT_SUFFIX_TABLE = {
  FLOAT_DATATYPE ="Atom",
  INTEGER_DATATYPE = "Atom",
  GENERIC_TYPE = "Generic",
  ENUMERATION_DATATYPE = "Enum",
  STRING_DATATYPE = "String",
  ARRAY_DATATYPE = "Array",
  PARAMETER = "Parameter",
  COMMAND = "Command",
  ARGUMENT = "Argument",
  COMPONENT = "Component",
  VARIABLE = "Variable",
  DECLARED_INTERFACE = "Interface",
  PROVIDED_INTERFACE = "Interface",
  REQUIRED_INTERFACE = "Interface"
}

-- Values for the decoder table used by iterate_members below
local PROVINTF_FILTER = SEDS.create_nodetype_filter("PROVIDED_INTERFACE")
local PARAMETER_FILTER = SEDS.create_nodetype_filter("PARAMETER")
local ARGUMENT_FILTER = SEDS.create_nodetype_filter("ARGUMENT")
local CONTAINER_ENTRY_FILTER = SEDS.create_nodetype_filter({
  "CONTAINER_ENTRY",
  "CONTAINER_FIXED_VALUE_ENTRY",
  "CONTAINER_PADDING_ENTRY",
  "CONTAINER_LIST_ENTRY",
  "CONTAINER_LENGTH_ENTRY",
  "CONTAINER_ERROR_CONTROL_ENTRY",
})

local ITERATE_MEMBER_SET_DECODER_TABLE =
{
  ["default"] = {
    CONTAINER_DATATYPE =  { base = "basetype", entity_set = "CONTAINER_ENTRY_LIST", entfilter = CONTAINER_ENTRY_FILTER },
    DECLARED_INTERFACE =  { base = "basetype", entity_set = "PARAMETER_SET", entfilter = PARAMETER_FILTER, recurse = true },
    REQUIRED_INTERFACE =  { base = "type", entity_set = "PARAMETER_SET", entfilter = PARAMETER_FILTER, recurse = true },
    PROVIDED_INTERFACE =  { base = "type", entity_set = "PARAMETER_SET", entfilter = PARAMETER_FILTER, recurse = true },
    COMPONENT =           { entity_set = "PROVIDED_INTERFACE_SET", entfilter = PROVINTF_FILTER, submember_direct = true },
    COMMAND =             { entfilter = ARGUMENT_FILTER },
  },
  ["trailer"] = {
    CONTAINER_DATATYPE =  { base = "basetype", entity_set = "CONTAINER_TRAILER_ENTRY_LIST", entfilter = CONTAINER_ENTRY_FILTER, recurse = true }
  }
}

-- -------------------------------------------------------------------------
-- HELPER FUNCTIONS
-- these are used locally but are not exposed outside of this source file
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
---
-- find_actual_type: Use the breadcrumbs in "reflist" to obtain a concrete type for typeref
--
-- If typeref is already concrete, the same value is returned.
-- If typeref is abstract, a suitable concrete type is found by following the chain until
-- a matching generic type map node is located.
--
-- @param typeref provides the starting point for relative references
-- @param initial_reflist "breadcrumbs" for generic type resolution
-- @return type node
--
local function find_actual_type(typeref, reflist)

  local mapnode

  -- This is only meaningful for generic types.  For regular
  -- concrete types, just return the same type
  while (typeref and typeref.entity_type == "GENERIC_TYPE") do
    mapnode = nil
    for i,v in ipairs(reflist or {}) do
      for node in v:iterate_subtree("GENERIC_TYPE_MAP") do
        if (node.name == typeref.name) then
          mapnode = node.type
        end
      end
      if (mapnode) then
        break
      end
    end
    if (not mapnode) then
      break
    end
    typeref = mapnode
  end

  return typeref
end

-- -------------------------------------------------------------------------
-- QUERY FUNCTIONS
-- These functions extract specific information from a DOM node
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
---
-- get_qualified_name: Determines the fully-qualified SEDS type name for the given node
--
-- This is of the form PACKAGE[/SUBNAMESPACE...]/datatype[.entity]
-- This uses a forward slash as a namespace separator and a period as
-- an entity separator, as specified per the SEDS schema.  Additionally,
-- enumeration entries and command entries are separated by a colon.
--
-- @param node node to query
-- @return a string containing the fully qualified EDS name
--
local function get_qualified_name(node)
  local result
  local sep
  local separator_table =
  {
    ENUMERATION_ENTRY = ":",
    COMMAND = ":",
    ARGUMENT = ".",
    PARAMETER = ".",
    CONTAINER_ENTRY = ".",
    ENUMERATION_DATATYPE = "/",
    CONTAINER_TRAILER_ENTRY_LIST = ".",
    CONTAINER_ENTRY_LIST = ".",
    CONTAINER_DATATYPE = "/",
    ARRAY_DATATYPE = "/",
    INTEGER_DATATYPE = "/",
    FLOAT_DATATYPE = "/",
    BINARY_DATATYPE = "/",
    BOOLEAN_DATATYPE = "/",
    STRING_DATATYPE = "/",
    COMPONENT = "/",
    DECLARED_INTERFACE = "/",
    REQUIRED_INTERFACE = "/",
    PROVIDED_INTERFACE = "/",
    PACKAGE = "/",
    DEFINE = "/"
  }

  local fixedname_table =
  {
    CONTAINER_TRAILER_ENTRY_LIST = "Trailer",
    CONTAINER_ENTRY_LIST = "Content"
  }


  while (node) do
    local nodename = node.name or fixedname_table[node.entity_type]
    if (not result) then
      result = nodename
    elseif (sep and nodename and node.entity_type ~= "PACKAGEFILE" and node.entity_type ~= "DATASHEET") then
      result = nodename .. sep .. result
      sep = nil
    end
    if (not sep) then
      sep = separator_table[node.entity_type]
    end
    node = node.parent
  end

  return result
end

-- -------------------------------------------------------------------------
---
-- get_flattened_name: Get a node name for use in a "flattened" namespace
--
-- Languages which do not support namespaces, such as C, require identifiers
-- to be unique in the global namespace.  To translate into these environments,
-- the EDS qualified name must be flattened before it can be used.
--
-- All the EDS namespaces will be compressed into a single string, which is
-- safe to use in typical language source files.
--
-- Note that two different type EDS elements may use matching names without
-- conflicting.  This is allowed in EDS but typically not in other languages
-- such as C, so an additional suffix ensures uniqueness between different
-- element types.
--
-- @param node node to query
-- @param suffix additional suffix to add (optional)
-- @return flattened name (string)
--
local function get_flattened_name(node,suffix)
  local output = {}
  -- Note that LUA treats "nil" table values as unassigned
  -- So if any of the inputs are nil it will simply be omitted
  output[1 + #output] = get_qualified_name(node)
  output[1 + #output] = FLAT_SUFFIX_TABLE[node.entity_type]
  output[1 + #output] = suffix

  local str = output[1]
  for i = 2, #output do
    str = str .. "_" .. output[i]
  end
  return SEDS.to_safe_identifier(str)
end


-- -------------------------------------------------------------------------
-- FIND FUNCTIONS
-- These functions find specific nodes in the DOM, starting from a context node
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
---
-- find_first: Find the first child node of the context node which matches the given filter
--
-- The filter can be a function which returns true/false or anything else is
-- treated as value to be passed to create_nodetype_filter().  The first child
-- that matches the filter is returned, or nil if nothing is found.
--
-- @param context provides the starting point for relative references
-- @param filter selection function to apply to subtree
-- @return matching node or or nil if no matching nodes
--
local function find_first(context,filter)
  return context:iterate_subtree(filter)()  -- grab the first item from the iterator
end

-- -------------------------------------------------------------------------
---
-- find_entity: Locates an entity within the given EDS container
--
-- This searches all base types and nested values for a matching entity
--
-- This function recognizes a "." character as a name separator and will recurse into
-- nested types as necessary to locate the given reference.
--
-- @param context provides the starting point for relative references
-- @param refstring specifies the entity to search for
-- @param initial_reflist optional "breadcrumbs" for generic type resolution
-- @return matching entity or nil
--
local function find_entity(context,refstring,initial_reflist)

  local refparts = {}
  local reflist = {}
  local refpos = 1

  local function do_search(node)
    local found = false
    if (node) then
      local bitoffset = node.resolved_size and 0
      for idx,type,refnode in node:iterate_members(reflist) do
        local matchparts
        if (refnode == nil) then
          matchparts = 0
        elseif (refnode.name == refparts[refpos]) then
          matchparts = 1
        end
        if (matchparts) then
          refpos = refpos + matchparts
          reflist[1 + #reflist] = { node = refnode, type = type, offset = bitoffset }
          if (not refparts[refpos] or do_search(type)) then
            -- Found match
            found = true
            break
          end
          refpos = refpos - matchparts
          reflist[#reflist] = nil
        end
        if (bitoffset and type and type.resolved_size) then
          bitoffset = bitoffset + type.resolved_size.bits
        end
      end
    end
    return found
  end

  for k,v in ipairs(initial_reflist or {}) do
    reflist[k] = v
  end

  if (context and refstring) then
    -- Split the refstring into its components
    for i,j in string.gmatch(refstring, "([%w%d_]+)([^%w%d_]*)") do
      refparts[1+#refparts] = i
      if(j ~= ".") then
        break
      end
    end
  end

  if (do_search(context)) then
    return reflist
  end

end

-- -------------------------------------------------------------------------
---
-- find_reference: Given a string that refers to a SEDS node, look up the corresponding tree object
--
-- Such reference strings are used throughout SEDS documents, e.g. for type and basetype references
-- among others.  These are notionally of the form "PACKAGE/typename" but the namespace or any
-- leading portion of the namespace may be omitted
--
-- @param context provides the starting point for relative references
-- @param refstring is a string which refers to a SEDS node
-- @param filter is an optional filter function which may provide additional match criteria (such as node type)
--
local function find_reference(context,refstring,filter)

  if (filter ~= nil and type(filter) ~= "function") then
    filter = SEDS.create_nodetype_filter(filter)
  end
  local result = nil
  local refparts = {}
  local refpos = 1
  local function do_search(n)
    if (not result) then
      -- Note that datasheet nodes are unique in that they have
      -- a name in the tree but this name does not count for searches
      if (n.entity_type == "DATASHEET" or n.entity_type == "PACKAGEFILE" or not n.name) then
        -- this node has no name -- move directly to child nodes
        for child in n:iterate_children() do
          do_search(child)
        end
      elseif (n.name == refparts[refpos]) then
        refpos = refpos + 1
        if (refparts[refpos]) then
          -- Search the next level
          for child in n:iterate_children() do
            do_search(child)
          end
        elseif (type(filter) ~= "function" or filter(n)) then
          -- Found match
          result = n
        end
        refpos = refpos - 1
      end
    end
    return result
  end

  if (refstring and context) then

    -- If the refstring has a leading "/", then this indicates
    -- an absolute path.  Move immediately to the root node.
    if (string.match(refstring,"^/")) then
      while(context.parent) do
        context = context.parent;
      end
      refstring = string.sub(refstring, 1)
    end

    -- Split the refstring into its components
    for i,j in string.gmatch(refstring, "([%w%d@_]+)([^%w%d@_]*)") do
      refparts[1+#refparts] = i
      if(j ~= "/") then
        break
      end
    end

    while(context and not result) do
      if (context.name or not context.parent) then
        for child in context:iterate_children() do
          do_search(child)
        end
      end
      context = context.parent
    end
  end
  return result
end

-- -------------------------------------------------------------------------
---
-- find_parent: Find a parent/ancestor node of the current node which matches the given filter
--
-- The filter can be a function which returns true/false or anything else is
-- treated as value to be passed to create_nodetype_filter().  This will progress
-- up the tree until a matching node is found, or the root node is reached.
--
-- @param base provides the starting point in the DOM
-- @param filter may provide additional match criteria (such as node type)
--
local function find_parent(base,filter)
  if (type(filter) ~= "function") then
    filter = SEDS.create_nodetype_filter(filter)
  end
  while (base and not filter(base)) do
    base = base["parent"];
  end
  return base
end


-- -------------------------------------------------------------------------
-- ITERATOR FUNCTIONS
-- These functions allow "for" loop constructs to easily iterate over nodes in the DOM
-- The supplied filter function, if specified, allows simple node selection criteria
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
---
-- iterate_children: Iterate direct children of the current node
--
-- Nodes will be selected from the immediate child nodes of the given node
-- using the filter function, or all children will be selected if the filter is nil.
--
-- The filter function should return a boolean value indicating whether or not
-- a given node should be selected.
--
-- Note that this only iterates direct children.
--
-- @param node indicates the starting node in the DOM tree
-- @param filter selection function to apply
-- @return iterator function compatible with Lua "for" loops
--
-- @see SEDS.create_filter_function
-- @see iterate_subtree for a version which iterates an entire subtree
--
local function iterate_children(node,filter)
  local subset = {}
  local subnode_pos = 0
  if (type(filter) == "nil") then
    filter = function(f) return true end
  elseif (type(filter) ~= "function") then
    filter = SEDS.create_nodetype_filter(filter)
  end
  for idx,subnode in ipairs(node and node.subnodes or {}) do
    if (filter(subnode)) then
      subset[1 + #subset] = subnode
    end
  end
  return function()
    subnode_pos = 1 + subnode_pos
    return subset[subnode_pos]
  end
end

-- -------------------------------------------------------------------------
---
-- iterate_subtree: Iterate all DOM nodes beneath the current node
--
-- Nodes will be selected from all nodes beneath the given node, including grandchildren,
-- great-grandchildren and so forth.
--
-- The filter function should return a boolean value indicating whether or not
-- a given node should be selected.  If nil, then all subtree nodes are selected.
--
-- Note that this also effectively "flattens" the subtree; the selected nodes
-- are returned in the loop sequentially, regardless of the original depth.
--
-- @param node indicates the starting node in the DOM tree
-- @param filter selection function to apply
-- @return iterator function compatible with Lua "for" loops
--
local function iterate_subtree(node,filter)
  local subset = {}
  local subnode_pos = 0
  if (type(filter) == "nil") then
    filter = function(f) return true end
  elseif (type(filter) ~= "function") then
    filter = SEDS.create_nodetype_filter(filter)
  end
  local function get_subtree(n)
    for idx,subnode in ipairs(n.subnodes or {}) do
      get_subtree(subnode)
      if (filter(subnode)) then
        subset[1 + #subset] = subnode
      end
    end
  end
  get_subtree(node)
  return function()
    subnode_pos = 1 + subnode_pos
    return subset[subnode_pos]
  end
end

-- -------------------------------------------------------------------------
---
-- iterate_members: Iterate all entities within a container or interface
--
-- Iterates through the entities contained within an EDS container object,
-- interface, component, or command.
--
-- The returned iterator function returns 3 values:
--   1: sequential position of member within parent
--   2: member type
--   3: referring node (e.g. container entry node)
--
-- @param basenode indicates the starting node in the DOM tree
-- @param initial_reflist optional "breadcrumbs" for generic type resolution
-- @return iterator function compatible with Lua "for" loops
--
local function iterate_members(basenode,initial_reflist,mode)
  local memb_types = {}
  local memb_refnode = {}
  local memb_pos = 0
  local memb_total = 0
  local reflist = initial_reflist or {}
  local decode_table

  local function add_members(node)
    local decoder = node and decode_table[node.entity_type]

    if (decoder) then
      reflist[1 + #reflist] = node
      local basetypes
      local subset
      if (decoder.base) then
        basetypes = { find_actual_type(node[decoder.base],reflist) }
        subset = find_first(node,"BASE_INTERFACE_SET")
        for idx,subbase in ipairs(subset and subset.subnodes or {}) do
          if (subbase.entity_type == "BASE_TYPE") then
            basetypes[1 + #basetypes] = find_actual_type(subbase.type,reflist)
          end
        end
        for i,bt in ipairs(basetypes) do
          if (decoder.recurse) then
            add_members(bt)
          else
            memb_total = 1 + memb_total
            memb_types[memb_total] = bt
          end
        end
      end

      subset = decoder.entity_set and find_first(node,decoder.entity_set) or node
      for idx,submember in ipairs(subset and subset.subnodes or {}) do
        if (decoder.entfilter(submember)) then
          memb_total = 1 + memb_total
          if (decoder.submember_direct) then
            memb_types[memb_total] = submember
          else
            memb_types[memb_total] = find_actual_type(submember.type,reflist)
          end
          memb_refnode[memb_total] = submember
        end
      end

      reflist[#reflist] = nil
    end
  end

  -- By default, iterate through the regular members that appear in
  -- both concrete and abstract/basetype usage
  if (not mode or mode == "default") then
    decode_table = ITERATE_MEMBER_SET_DECODER_TABLE.default
    add_members(basenode)
  end

  -- By default, if the container is not abstract, include the "trailer" items as well
  if (mode == "trailer" or (not mode and not basenode.attributes.abstract)) then
    decode_table = ITERATE_MEMBER_SET_DECODER_TABLE.trailer
    add_members(basenode)
  end

  return function()
    memb_pos = 1 + memb_pos
    if (memb_pos > memb_total) then return nil end
    return memb_pos,memb_types[memb_pos], memb_refnode[memb_pos]
  end
end

-- -------------------------------------------------------------------------
-- DEPENDENCY TRACKING FUNCTIONS
-- These assist in keeping track of dependencies between DOM objects
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
---
-- mark_reference:  Establish a dependency relationship between two DOM nodes.
--
-- It is often necessary to know which nodes are derived from or otherwise
-- depend on or refer to a given node.  This function establishes a relationship
-- between two nodes in the tree.  The referrer is used as a key in the internal table, so
-- doing multiple calls with the same node and referrer is benign; only a single entry
-- is made
--
-- @param dependent dependent node
-- @param referrer  node which refers to dependent node
-- @param relationship arbitrary value that if specified allows more details
-- of the relationship to be known later during processing
-- @see get_references
--
local function mark_reference(dependent,referrer,relationship)
  if (relationship == nil) then
    relationship = "default"
  end
  local reflist = dependent.references
  if (type(reflist) ~= "table") then
    reflist = {}
    dependent.references = reflist
  end
  if (type(reflist[referrer]) ~= "table") then
    reflist[referrer] = {}
  end
  reflist[referrer][relationship] = true
end

-- -------------------------------------------------------------------------
---
-- Get a list of DOM objects which reference the given DOM object
--
-- Returns an array of objects that refer to this object as indicated via previous calls to mark_reference()
-- The resulting array is sorted to ensure consistency in output order between calls
--
-- @param node subject node
-- @param relationship if specified, only returns nodes that were marked with the same relationship
-- @return a simple array with the referring nodes as values.
--
-- @see mark_reference
--
local function get_references(node,relationship)
  local reflist = {}
  if (node.references) then
    for robj,rtype in pairs(node.references) do
      if (relationship == nil or rtype[relationship]) then
        reflist[1 + #reflist] = robj
      end
    end
  end
  table.sort(reflist, function(a,b)
    if (not a.name or not b.name or a.name == b.name) then
      return a.id < b.id
    end
    return a.name < b.name
  end)
  return reflist
end

-- -------------------------------------------------------------------------
-- DEBUGGING FUNCTIONS
-- Additional tree methods which provide helpful information for debug
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
---
-- get_xml_location: Obtain the full XML file and line of the originating code
--
-- This extracts the original XML file and source line which corresponds to
-- the DOM node.
--
-- @param node the DOM node to query
-- @return a string in the form file:line
--
local function get_xml_location(node)
  local file = node.xml_filename
  if (file) then
    file = tostring(file)
    local line = node.xml_linenum
    if (line) then
      file = file .. ":" .. tostring(line)
    end
  end
  return file
end

-- -------------------------------------------------------------------------
---
-- debug_print: Print the node properties on the console for debugging purposes
--
local function debug_print(node)
  print ("-- BEGIN NODE PROPERTIES --")
  print ("entity_type => " .. node.entity_type)
  for i,v in ipairs(node:get_properties()) do
    local propval = node[v]
    print (" [" .. tostring(v) .. "] =>" .. type(propval) .. ":" .. tostring(propval))
    if (type(propval) == "table") then
      for tk,tv in pairs(propval) do
        print ("    [" .. tostring(tk) .. "] => " .. type(tv) .. ":" .. tostring(tv))
      end
    end
  end
  print ("-- END NODE PROPERTIES --")
end

-- Return a table of methods for tree objects
return {
  get_qualified_name = get_qualified_name,
  get_flattened_name = get_flattened_name,
  find_first = find_first,
  find_parent = find_parent,
  find_reference = find_reference,
  find_entity = find_entity,
  iterate_members = iterate_members,
  iterate_children = iterate_children,
  iterate_subtree = iterate_subtree,
  mark_reference = mark_reference,
  get_references = get_references,
  get_xml_location = get_xml_location,
  debug_print = debug_print
}

