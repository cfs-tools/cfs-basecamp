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
-- Lua implementation of "implicit nodes" processing step
--
-- This resolves those cases where an entity must exist that is not explicitly
-- written in the EDS source.  This includes:
--
--   Implicit Types:
--     When a container entry uses an "encoding" element to essentially override
--     the specified type, for all intents and purposes this is making a new type
--     on the fly.  To keep future processing simpler, it is best to create a
--     new type node for this.
--
--   Multiple constraints:
--     When a container has a "ConstrantSet" with multiple entities in it, the
--     intended effect is to treat them as an "AND" function.  To facilitate
--     conversion to a decision tree, this rearranges the constraints across
--     multiple implicit base type nodes, with one constraint per base type.
--
-- All of these implicit nodes must be created _before_ the linkages are determined,
-- so adjustments can be made more easily and relationships will be correct after
-- linkages/refs are resolved.
--
-- -------------------------------------------------------------------------

SEDS.info ("SEDS implicit scalars START")

-- -------------------------------------------------------------------------
-- HELPER FUNCTIONS
-- These are used locally but not exposed to the outside world
-- -------------------------------------------------------------------------

local ENCODING_FILTER = SEDS.create_nodetype_filter
{
        "INTEGER_DATA_ENCODING",
        "FLOAT_DATA_ENCODING",
        "STRING_DATA_ENCODING"
}

-- The ALL_IMPLICIT_TYPES is a local registry of all types created in this
-- script.  It is indexed by the container that necessitated them.
local ALL_IMPLICIT_TYPES = {}

-- Creates an implicit node (might be a type or something else)
-- This sets up the metadata of the new node consistently.
local function initialize_implicit_node(entity_type, refentry)

  local implicit_node = SEDS.new_tree_object(entity_type)

  -- Copy the xml_filename/xml_linenum attributes for traceability
  -- This helps with diagnosis if an error occurs later
  implicit_node.implicit = true -- mark to indicate this was not in original XML
  implicit_node.xml_filename = refentry.xml_filename
  implicit_node.xml_linenum = refentry.xml_linenum
  implicit_node.attributes = { }

  return implicit_node
end

-- Creates an implicit type node.
-- This helper function enforces a consistent naming convention
-- for the implicit type and also adds it to the local registry table
local function add_implicit_type(container, entry, entity_type, subnodes)

  local typenode = initialize_implicit_node(entity_type, entry)

  typenode.name = container.name .. "_" .. entry.name
  typenode.subnodes = subnodes

  -- Register the implicit type in the dictionary
  -- This is used to update the datatypeset later.
  local typelist = ALL_IMPLICIT_TYPES[container]
  if (not typelist) then
    typelist = {}
    ALL_IMPLICIT_TYPES[container] = typelist
  end
  typelist[1 + #typelist] = typenode

  return typenode

end

-- -------------------------------------------------------------------------
-- MAIN SCRIPT ROUTINE BEGINS HERE
-- -------------------------------------------------------------------------

-- Fix up all "DataTypeSet" entities as needed
local datatypesets = {}
for datatypeset in SEDS.root:iterate_subtree("DATA_TYPE_SET") do
  datatypesets[1 + #datatypesets] = datatypeset
end

for _,datatypeset in ipairs(datatypesets) do
  for cont in datatypeset:iterate_children("CONTAINER_DATATYPE") do
    for entry in cont:iterate_subtree(SEDS.container_entry_filter) do
      local typeref = entry.type
      local encoding = entry:find_first(ENCODING_FILTER)
      local arraydim = entry:find_first("ARRAY_DIMENSIONS")
      if (encoding) then
        local subnodes = {}
        subnodes[1 + #subnodes] = encoding
        subnodes[1 + #subnodes] = entry.type:find_first("ENUMERATION_LIST")

        local implicit_encnode = add_implicit_type(cont, entry, typeref.entity_type, subnodes)

        typeref = implicit_encnode
        implicit_encnode.implicit_basetype = entry.type
      end

      if (arraydim or entry.entity_type == "CONTAINER_LIST_ENTRY") then

        local dimension_list = initialize_implicit_node("DIMENSION_LIST", entry)

        if (arraydim) then
          -- Reference the subnodes directly
          -- Should contain "Dimension" subnode(s)
          dimension_list.subnodes = arraydim.subnodes

        else
          -- Create a dimension node
          -- Should have a "ValidRange" subnode or a use a
          -- range-restricted type as an indextyperef
          local dimension_node = initialize_implicit_node("DIMENSION", entry)
          local vrange = entry.listlengthfield:find_first("VALID_RANGE")

          if (not vrange) then
            -- Assume the type is an index type (will be verified later)
            dimension_node.indextyperef = entry.listlengthfield.type
          elseif (vrange.resolved_range.min and
             (not vrange.resolved_range.min.inclusive or
              vrange.resolved_range.min.value ~= 0)) then
            entry:error("Range is not zero-based", entry.listlengthfield)
          elseif (not vrange.resolved_range.max or not
            vrange.resolved_range.max.value) then
            entry:error("Range has no maximum defined", entry.listlengthfield)
          else
            local dim_size = vrange.resolved_range.max.value
            if (vrange.resolved_range.max.inclusive) then
              dim_size = 1 + dim_size
            end
            dimension_node.size = dim_size
          end

          dimension_node.parent = dimension_list
          dimension_list.subnodes = { dimension_node }
        end

        local implicit_arraynode = add_implicit_type(cont, entry, "ARRAY_DATATYPE", { dimension_list })

        dimension_list.parent = implicit_arraynode
        implicit_arraynode.datatyperef = typeref
        typeref = implicit_arraynode
      end

      entry.type = typeref

    end
  end

  -- Create a new "subnodes" list for the parent datatypeset,
  -- by inserting any implicit types directly prior
  -- to the container that requires them
  local merged_typelist = {}
  for typenode in datatypeset:iterate_children() do
    for _,implicit_type in ipairs(ALL_IMPLICIT_TYPES[typenode] or {}) do
      implicit_type.parent = datatypeset
      merged_typelist[1 + #merged_typelist] = implicit_type
    end
    merged_typelist[1 + #merged_typelist] = typenode
  end
  datatypeset.subnodes = merged_typelist
end

SEDS.info ("SEDS implicit scalars END")
