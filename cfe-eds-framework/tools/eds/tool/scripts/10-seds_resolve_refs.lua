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
-- Lua implementation of "resolve references" processing step
--
-- This runs early in the processing stage and translates the string values
-- supplied in XML files into correct references to other EDS objects.  The
-- text-based attributes from the original files are mapped to real Lua
-- values of the correct type.
--
-- Most importantly, references to other DOM objects (such as type references)
-- will become an actual link to the DOM object, rather than a string.
--
-- Other attributes of the various EDS DOM nodes are resolved to an actual value
-- that can be used by subsequent scripts.
--
-- NOTE: the specific attribute names and meanings in this file are defined in
-- CCSDS book 876.0 and those definitions should be adhered to as closely as possible.
--
-- -------------------------------------------------------------------------

SEDS.info ("SEDS resolve refs START")

-- -------------------------------------------------------------------------
-- HELPER FUNCTIONS
-- These are used locally but not exposed to the outside world
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
---
-- mark_datasheet_dependency: Establish a relationship between the parent datasheets of two DOM objects.
--
-- This is a specialized version of mark_reference that operates on the encapsulating DATASHEET or PACKAGEFILE
-- nodes for each of the supplied arguments.  Essentially, if any element within a datasheet
-- refers to an element that is defined in another datasheet, that dependency relationship
-- may need to be known for future processing.  In the case of C output files, this generally
-- means that the header file from one must include the header file from the other.
--
local function mark_datasheet_dependency(node,dependency)
  local linktype
  if (SEDS.any_datatype_filter(dependency)) then
    linktype = "datatype"
  end

  node = node:find_parent(SEDS.basenode_filter)
  dependency = dependency:find_parent(SEDS.basenode_filter)
  if (node and dependency and node ~= dependency) then
    node:mark_reference(dependency,linktype)
  end
end


-- A special modifier for the enum label "value" attribute
-- If unspecified, then use the previous label value + 1
local function resolve_enum_value(node,attr)
  local parent_enum = node:find_parent("ENUMERATION_DATATYPE")
  local value = node.attributes[attr]
  if (parent_enum) then
    if (not value) then
      if (parent_enum.last_value) then
        value = 1 + parent_enum.last_value
      else
        value = 0
      end
    end
    parent_enum.last_value = value
  end
  return value
end

-- Helper function to determine the minimum and maximum value of an Enum
local function resolve_enum_range(node,attr)
  local rmin
  local rmax
  for ent in node:iterate_subtree("ENUMERATION_ENTRY") do
    if (not ent.value) then
      ent:error("Enum missing value")
    else
      if (not rmin or not rmin.value) then
        rmin = { value = ent.value, inclusive = true }
      elseif(ent.value < rmin.value) then
        rmin.value = ent.value
      end
      if (not rmax or not rmax.value) then
        rmax = { value = ent.value, inclusive = true }
      elseif(ent.value > rmax.value) then
        rmax.value = ent.value
      end
    end
  end

  return { min = rmin, max = rmax }
end

-- Helper function to determine the possible range of an
-- element where minimum and maximum are specified
local function resolve_minmax_range(rangedef)

  local rmin = rangedef.attributes.min
  local rmax = rangedef.attributes.max
  local subrange = { }

  if (rangedef.attributes.rangetype) then
    local resolved_text = string.lower(rangedef.attributes.rangetype)
    if (resolved_text == "exclusiveminexclusivemax") then
      subrange.min = { value = rmin, inclusive = false }
      subrange.max = { value = rmax, inclusive = false }
    elseif(resolved_text == "inclusivemininclusivemax") then
      subrange.min = { value = rmin, inclusive = true }
      subrange.max = { value = rmax, inclusive = true }
    elseif(resolved_text == "inclusiveminexclusivemax") then
      subrange.min = { value = rmin, inclusive = true }
      subrange.max = { value = rmax, inclusive = false }
    elseif (resolved_text == "exclusivemininclusivemax") then
      subrange.min = { value = rmin, inclusive = false }
      subrange.max = { value = rmax, inclusive = true }
    elseif (resolved_text == "greaterthan") then
      subrange.min = { value = rmin, inclusive = false }
    elseif (resolved_text == "atleast") then
      subrange.min = { value = rmin, inclusive = true }
    elseif (resolved_text == "lessthan") then
      subrange.max = { value = rmax, inclusive = false }
    elseif (resolved_text == "atmost") then
      subrange.max = { value = rmax, inclusive = true }
    end
  end

  return subrange
end

-- Helper function to calculate the absolute/final range
-- of a range EDS element.  This is basically the union
-- of all limiting elements beneath it.
local function resolve_range(rangenode)

  local subrange = {}

  -- merge all the range definitions into a single one
  for n in rangenode:iterate_subtree() do
    if (n.resolved_range) then
      if (not subrange.min or not subrange.min.value) then
        subrange.min = n.resolved_range.min
      elseif(n.resolved_range.min and n.resolved_range.min.value) then
        if (n.resolved_range.min.value > subrange.min.value) then
          subrange.min = n.resolved_range.min
        elseif (n.resolved_range.min.value == subrange.min.value) then
          subrange.min.inclusive = n.resolved_range.min.inclusive and subrange.min.inclusive
        end
      end
      if (not subrange.max or not subrange.max.value) then
        subrange.max = n.resolved_range.max
      elseif(n.resolved_range.max and n.resolved_range.max.value) then
        if (n.resolved_range.max.value < subrange.max.value) then
          subrange.max = n.resolved_range.max
        elseif (n.resolved_range.max.value == subrange.max.value) then
          subrange.max.inclusive = n.resolved_range.max.inclusive and subrange.max.inclusive
        end
      end
    end
  end

  return subrange
end

-- Helper function to create a "name" attribute for datasheet objects
-- This is an extension; the XML schema does not indicate any "name" attribute
-- on Datasheet elements, but it can be useful for future file generation.
-- The name of a datasheet DOM node will be the base name of the XML file it is from.
local function resolve_datasheet_basename(node)
  local basename = node.xml_filename
  basename = string.match(node.xml_filename, "/([^/]+)$") or basename
  basename = string.match(basename, "(%S+)%.[^%.]*") or basename
  return basename
end

-- -------------------------------------------------------------------------
-- MASTER TABLE OF ALL SEDS ATTRIBUTES NEEDING RESOLUTION HERE
-- This table is used locally by "resolve_attribs" but not exposed to the outside world
-- -------------------------------------------------------------------------

--
-- Given an element type, indicate the various attributes that
-- are valid for that element.  The recognized attribute styles are:
--  link)    resolve to another SEDS node, searching within parent
--           nodes/namespace(s) of the context element (outward search)
--  entity)  resolve to another SEDS node, searching entities within the
--           context container  (inward search)
--  integer) resolve to an integer value
--  string)  resolve to a string value, convert to lowercase for case-insensitive compares
--  boolean) resolve to either true/false
-- Alternatively, the entry can be a function, which will be called to handle the attribute

-- The following additional fields add additional control:
--  impl)     user-defined function to obtain the value for the attribute
--  filter)   applies to node links, specifies the type of node to link to
--  default)  indicates an optional default value if missing
--            if the default is a function, the function will be called
local SEDS_ATTRIBUTE_TABLE =
{
  PACKAGEFILE =
  {
    name = { impl=resolve_datasheet_basename }
  },
  DATASHEET =
  {
    name = { impl=resolve_datasheet_basename }
  },
  CONTAINER_ENTRY =
  {
    type = { style="link", filter=SEDS.concrete_datatype_filter, required = true }
  },
  CONTAINER_LENGTH_ENTRY =
  {
    type = { style="link", filter=SEDS.concrete_datatype_filter, required = true }
  },
  CONTAINER_FIXED_VALUE_ENTRY =
  {
    type = { style="link", filter=SEDS.concrete_datatype_filter, required = true }
  },
  CONTAINER_LIST_ENTRY =
  {
    type = { style="link", filter=SEDS.concrete_datatype_filter, required = true },
    listlengthfield = { style="entity", required=true }
  },
  CONTAINER_ERROR_CONTROL_ENTRY =
  {
    type = { style="link", filter=SEDS.concrete_datatype_filter, required = true }
  },
  CONTAINER_PADDING_ENTRY =
  {
    sizeinbits = { style="integer", required = true }
  },
  CONTAINER_DATATYPE =
  {
    basetype = { style="link", filter=SEDS.container_filter, mark_reference="derivatives" },
  },
  GENERIC_TYPE =
  {
    basetype = { style="link", filter=SEDS.any_datatype_filter }
  },
  ARRAY_DATATYPE =
  {
    datatyperef = { style="link", filter=SEDS.concrete_datatype_filter, required = true },
  },
  DIMENSION =
  {
    indextyperef = { style="link", filter=SEDS.index_datatype_filter },
    size = { style="integer" }
  },
  ENUMERATION_ENTRY =
  {
    value = { style="integer", impl=resolve_enum_value }
  },
  ENUMERATION_DATATYPE =
  {
    resolved_range = { impl=resolve_enum_range }
  },
  STRING_DATATYPE =
  {
    length = { style="integer", required = true }
  },
  BOOLEAN_DATA_ENCODING =
  {
    byteorder = { style="string", default="bigendian" },
    falsevalue = { style="string" },
    sizeinbits = { style="integer", default=1 }
  },
  STRING_DATA_ENCODING =
  {
    encoding = { style="string" }
  },
  INTEGER_DATA_ENCODING =
  {
    byteorder = { style="string", default="bigendian" },
    encoding = { style="string" },
    sizeinbits = { style="integer", required = true }
  },
  FLOAT_DATA_ENCODING =
  {
    byteorder = { style="string", default="bigendian" },
    encodingandprecision = { style="string" },
    sizeinbits = { style="integer" }
  },
  RANGE =
  {
    resolved_range = { impl=resolve_range }
  },
  VALID_RANGE =
  {
    resolved_range = { impl=resolve_range }
  },
  MINMAX_RANGE =
  {
    resolved_range = { impl=resolve_minmax_range },
  },
  RANGE_CONSTRAINT =
  {
    min = { style="integer" },
    max = { style="integer" }
  },
  BINARY_DATATYPE =
  {
    sizeinbits = { style="integer", required = true }
  },
  SUBRANGE_DATATYPE =
  {
    basetype = { style="link", filter=SEDS.numeric_datatype_filter, mark_reference="subranges", required = true }
  },
  BASE_INTERFACE =
  {
    type = { style="link", filter=SEDS.declintf_filter, required = true }
  },
  PARAMETER =
  {
    type = { style="link", filter=SEDS.any_datatype_filter, required = true }
  },
  GENERIC_TYPE_MAP =
  {
    type = { style="link", filter=SEDS.any_datatype_filter, required = true }
  },
  ARGUMENT =
  {
    type = { style="link", filter=SEDS.any_datatype_filter, required = true }
  },
  VARIABLE =
  {
    type = { style="link", filter=SEDS.any_datatype_filter, required = true },
    readonly = { style="boolean", default = false }
  },
  PARAMETER_MAP =
  {
    interface = { style="link", filter=SEDS.referredintf_filter, required = true },
    variableref = { style="link", filter="VARIABLE", required = true }
  },
  REQUIRED_INTERFACE =
  {
    type = { style="link", filter=SEDS.declintf_filter, mark_reference="requirers", required = true }
  },
  PROVIDED_INTERFACE =
  {
    type = { style="link", filter=SEDS.declintf_filter, mark_reference="providers", required = true }
  },
  INTERFACE_MAP =
  {
    type = { style="link", filter=SEDS.declintf_filter, required = true }
  },
  DEFINE =
  {
    export = { style="boolean", default=true }
  },
  INSTANCE_RULE =
  {
    component = { style="link", filter=SEDS.component_filter }
  },
  INTERFACE_MAP =
  {
    rule = { style="link", filter=SEDS.instancerule_filter, required = true },
    type = { style="link", filter=SEDS.declintf_filter, required = true }
  }
}

-- Helper function to resolve attributes on a given node per the master
-- attribute resolution table above.
local function resolve_attribs(node)

  -- Handle all children first
  -- (In some cases this factors into the resolution of the local attributes)
  for child in node:iterate_children() do
    resolve_attribs(child)
  end

  local attrs = SEDS_ATTRIBUTE_TABLE[node.entity_type]
  if (attrs) then
    for attr,resolve in pairs(attrs) do
      local result = node[attr]
      if (result == nil) then

        -- If an implementation/modifier function was provided,
        -- then call that function now and use the result.  Otherwise,
        -- use the result from the define resolution step.
        if (type(resolve.impl) == "function") then
          result = resolve.impl(node,attr)
        elseif (resolve.style == "entity") then
          local pcont = node:find_parent("CONTAINER_DATATYPE")
          local entity = pcont and pcont:find_entity(node.attributes[attr])
          result = entity and entity[#entity].node
        elseif (resolve.style ~= "link") then
          result = node.attributes[attr]
        end

        if (resolve.style == "integer") then
          result = tonumber(result or resolve.default)
          if (result) then
            if (result < 0) then
              result = math.ceil(result)
            else
              result = math.floor(result)
            end
          end
        elseif (resolve.style == "boolean") then
          if (type(result) ~= "boolean") then
            if (type(result) == "number") then
              result = (result ~= 0)
            elseif (type(result) == "string") then
              -- convert to an actual bool
              result = string.lower(result)
              result = not (result == "no" or result == "false" or result == "0")
            else
              result = resolve.default
            end
          end
        elseif (resolve.style == "string") then
          result = string.lower(tostring(result or resolve.default))
        end

        if(result == nil and (resolve.required or node.xml_attrs[attr])) then
          node:error(string.format("attribute \'%s\' could not be resolved",attr),node.attributes[attr])
        end

        node[attr] = result
      end
    end
  end
end


-- -------------------------------------------------------------------------
-- MAIN SCRIPT ROUTINE BEGINS HERE
-- -------------------------------------------------------------------------

-- PASS 1: Resolve the "link" attributes (outside refs)
-- This includes base types and this must be done prior to other attributes
for node in SEDS.root:iterate_subtree() do
  local attrs = SEDS_ATTRIBUTE_TABLE[node.entity_type]
  if (attrs) then
    for attr,resolve in pairs(attrs) do
      -- Restrict to "link" style attributes in this first pass
      if(type(resolve) == "table" and resolve.style == "link") then
        local link = node:find_reference(node.attributes[attr], resolve.filter)
        if (link) then
          mark_datasheet_dependency(node, link)
          if (resolve.mark_reference) then
            link:mark_reference(node,resolve.mark_reference)
          end
          node[attr] = link
        elseif(resolve.required or node.xml_attrs[attr]) then
          node:error(string.format("attribute \'%s\' could not be resolved",attr),node.attributes[attr])
        end
      end
    end
  end
end

-- PASS 2: Call "resolve_attribs" helper function on the root node
-- the function recursively handles all nodes beneath the initial node.
-- This will resolve the remainder of attributes that was not done during pass 1.
resolve_attribs(SEDS.root)

SEDS.info ("SEDS resolve refs START")
