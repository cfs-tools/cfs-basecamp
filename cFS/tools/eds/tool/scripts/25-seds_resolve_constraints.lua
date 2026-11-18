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
-- Lua implementation of "resolve constraints" processing step
--
-- This short step just focuses on the entity references within constraint
-- elements.  This could not be done earlier since the references could point
-- to entries within the base type.
-- The "find_entity" implementation requires the "decode_sequence" to be
-- established first, which is not available until the sizes are resolved.
-- -------------------------------------------------------------------------
SEDS.info ("SEDS resolve constraints START")

local weights = {}
local base_entitylist = {}
local base_entitymap = {}

-- --------------------------------------------------------
-- Helper function: convert rangesegment values into comparable numbers
--
-- The "resolved_range" within the DOM is usually an instance of
-- the rangesegment userdata type, or it could be a string if
-- the subject entity is an enumeration.
-- The goal here is to map everything to a number so it can be
-- properly compared to other numbers in a decision tree
-- --------------------------------------------------------
local function remap_segment(refval,node)

  local orig = refval

  local reftype = node.resolved_entry and node.resolved_entry[#node.resolved_entry].type

  if (type(refval) == "userdata" and refval.is_single_value) then
    refval = refval.as_single_value
  end

  -- In the case that the entity datatype is an enumeration and the value
  -- is a string, then it must be an enumeration label.  In this case it
  -- must be mapped back to the numeric value before adding it to the value table.
  if (reftype and reftype.entity_type == "ENUMERATION_DATATYPE" and
      type(refval) == "string") then
    for label in reftype:iterate_subtree("ENUMERATION_ENTRY") do
      if (label.name == refval) then
        refval = label.value
        break
      end
    end

    -- If value is still a string, that means the mapping did not exist
    if (type(refval) == "string") then
      node:error("value is undefined",refval)
    end
  end

  return refval

end

-- --------------------------------------------------------
-- Helper function: resolve "entry" refs on constraint attributes
-- --------------------------------------------------------
local function resolve_container_entity(node, cont)
  if (not node.resolved_entry and node.attributes.entry) then
    node.resolved_entry = cont:find_entity(node.attributes.entry)
  end
  if (not node.resolved_entry) then
    node:error("undefined entry attribute",node.attributes.entry)
  end
  return node.resolved_entry
end

-- --------------------------------------------------------
-- Helper function: iterate over constraints on a container or element
-- --------------------------------------------------------
local function iterate_constraints(elem)

  local results = {}
  local iter = 0
  local parent_container
  local filter

  if (elem.entity_type == "PRESENT_WHEN") then
    filter = SEDS.presence_constraint_filter
  elseif (SEDS.container_filter(elem)) then
    parent_container = elem
    filter = SEDS.constraint_filter
  end

  if (not parent_container) then
    parent_container = elem:find_parent(SEDS.container_filter)
  end

  local my_iter = elem:iterate_subtree(filter)

  return function ()
    local node = my_iter()
    if (node) then
      resolve_container_entity(node, parent_container)
      return parent_container, node
    end
  end
end

-- --------------------------------------------------------
-- Helper function: iterate all values in a constraint
-- If the constraint is just a single value, the iterator returns just this value
-- --------------------------------------------------------
local function iterate_resolvedrange_values(value)

  local value_set = {}
  local iter_pos = 0
  local vforeach

  if (type(value) == "userdata") then -- seds_range typically
    vforeach = value.foreach
  end
  if (type(vforeach) ~= "function") then
    -- passthrough
    vforeach = function(v,f) f(v) end
  end
  vforeach(value, function (segment)
    value_set[1 + #value_set] = segment
  end)

  return function()
    iter_pos = 1 + iter_pos
    return value_set[iter_pos]
  end
end


-- --------------------------------------------------------
-- Helper function: build a map to find derivative types
--
-- This assembles the constraints on this type into a lookup
-- table that gets stored in the base type.

-- The lookup table is 2 deep with constrained entities at
-- the first level and the required value(s) they may have
-- at the second level, ending with a reference to this node
-- --------------------------------------------------------
local function build_derivative_decisiontree_map(cont)

  local basetype = cont.basetype
  local entitymap = {}
  local cont_entitylist = {}

  for _,node in iterate_constraints(cont) do
    local refent = node.attributes.entry
    if (entitymap[refent]) then
      node:error("Duplicate constraint", entitymap[refent])
    else
      entitymap[refent] = node
      cont_entitylist[1 + #cont_entitylist] = refent
    end
  end

  -- Add to basetype decision map
  local decisiontree_entry = basetype.derivative_decisiontree_map

  -- If basetype decision map did not exist, create now
  if (not decisiontree_entry) then
    decisiontree_entry = {}
    basetype.derivative_decisiontree_map = decisiontree_entry
  end

  -- process constraints in consistent order
  table.sort(cont_entitylist, function(a,b)
    local cweights = weights[basetype]
    if (cweights[a] == cweights[b]) then
      return (a < b) -- go by entity name if weights are equal
    else
      return (cweights[a] > cweights[b]) -- go by weight (highest first)
    end
  end)

  for _,entity in ipairs(cont_entitylist) do
    local node = entitymap[entity]
    local refval = node.resolved_range

    if (not decisiontree_entry.entities) then
      decisiontree_entry.entities = {}
    end
    decisiontree_entry = decisiontree_entry.entities

    if (not decisiontree_entry[entity]) then
      decisiontree_entry[entity] = {}
    end
    decisiontree_entry = decisiontree_entry[entity]

    if (not decisiontree_entry.refnode) then
      decisiontree_entry.refnode = node
    end
    if (not decisiontree_entry.values) then
      decisiontree_entry.values = SEDS.new_rangemap()
    end
    decisiontree_entry = decisiontree_entry.values

    if (not decisiontree_entry[refval]) then
      decisiontree_entry[refval] = {}
    end

    decisiontree_entry = decisiontree_entry[refval]
  end

  if (not decisiontree_entry.derivatives) then
    decisiontree_entry.derivatives = {}
  end
  decisiontree_entry = decisiontree_entry.derivatives
  decisiontree_entry[1 + #decisiontree_entry] = cont

  return entitymap
end

-- --------------------------------------------------------
-- Helper function: resolve and expand references to values in the decision tree
--
-- The values in the tree are directly as they were in the XML, meaning that
--   (a) refs to enum labels are still expressed as strings, not numbers
--   (b) multi-part ranges are in the table as a single constraint
--
-- It is important to boil everything down to individual numbers as much as
-- possible because a later stage is going to generate a lookup sequence that
-- implements a binary search.  This requires that all entries are "sortable"
-- and also that the sorting is done based on the real values, not strings.
-- --------------------------------------------------------
local fixup_descisiontree_entities
fixup_descisiontree_entities = function (etree)
  for entity in pairs(etree) do
    local vtree = etree[entity].values
    local refnode = etree[entity].refnode

    if (vtree) then
      local remapped_values = {}
      for value,subtree in pairs(vtree) do

        -- Recursively process all entities at this level
        if (subtree.entities) then
          fixup_descisiontree_entities(subtree.entities)
        end

        -- Now go through each of the values at this level and expand the set
        for segment in iterate_resolvedrange_values(value) do
          local v = remap_segment(segment,refnode)
          remapped_values[v] = subtree
        end
      end
      etree[entity].values = remapped_values
    end
  end
end

-- --------------------------------------------------
-- MAIN CODE BEGINS HERE
-- --------------------------------------------------

-- Initial Pass: determine the various offsets and comparison values
-- for every constraint in the database.
-- This is used to determine the order of nodes in the decision
-- tree that is produced.  By putting the common values first
-- the number of nodes (and number of different comparisons) can
-- be minimized.
for cont in SEDS.root:iterate_subtree(SEDS.container_filter) do
  local basetype = cont.basetype
  if (basetype) then
    if (not base_entitylist[basetype]) then
      base_entitylist[basetype] = {}
    end
    for _,node in iterate_constraints(cont) do
      local refent = node.attributes.entry
      local refval = tostring(node.resolved_range)
      local el = base_entitylist[basetype]
      if (not el[refent]) then
        el[refent] = {}
      end
      el = el[refent]
      el[refval] = (el[refval] or 0) + 1
    end
  end
end

-- Determine a reasonable order for evaluation:
-- Get a "weight" for each offset which is the ratio
-- of the number of total derivatives that have a constraint
-- on that offset to the number of different possible values
-- The higher this number is, the earlier it should be checked.
for cont,el in pairs(base_entitylist) do
  weights[cont] = {}
  for ent,vtbl in pairs(el) do
    local total = 0
    local nvals = 0
    for refval,count in pairs(vtbl) do
      total = total + count
      nvals = 1 + nvals
    end
    weights[cont][ent] = total / nvals
  end
end

-- Main Pass: Find all containers and process the constraints within them
for cont in SEDS.root:iterate_subtree(SEDS.container_filter) do

  -- Check for "PresentWhen" elements and resolve those constraints
  for entity in cont:iterate_subtree(SEDS.container_entry_filter) do
    local presence = entity:find_first("PRESENT_WHEN")
    if (presence) then
      local condition_list = {}
      if (entity.presence_condition_list) then
        entity:error("entity already has condition list")
      end
      for _,pcond in iterate_constraints(presence) do
        local value_set = {}
        for segment in iterate_resolvedrange_values(pcond.resolved_range) do
          value_set[1 + #value_set] = remap_segment(segment,pcond)
        end
        condition_list[1 + #condition_list] = {
          entity_name = pcond.attributes.entry or "what",
          entity = pcond.resolved_entry,
          value_set = value_set
        }
      end
      entity.presence_condition_list = condition_list
    end
  end

  -- If it is a derived container (i.e. has a base type) then add this
  -- to the decision tree on the base type
  if (cont.basetype) then
    base_entitymap[cont] = build_derivative_decisiontree_map(cont)
  end
end

-- Final Pass: cleanup
-- Expand all the multi-segment entries in the decisiontree into their constituent segments
-- This also handles things like mapping enumerated label references to their real value
for cont in SEDS.root:iterate_subtree(SEDS.container_filter) do

  if (cont.derivative_decisiontree_map) then
    local constraint_entity_set = cont.derivative_decisiontree_map.entities
    if (constraint_entity_set) then
      fixup_descisiontree_entities(constraint_entity_set)
    end
  end

end


SEDS.info ("SEDS resolve constraints END")
