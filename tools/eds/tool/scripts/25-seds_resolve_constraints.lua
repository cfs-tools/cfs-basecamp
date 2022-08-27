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

local entitylist = {}

-- Pass 1: determine the various offsets and comparison values
-- for every constraint in the database.
-- This is used to determine the order of nodes in the decision
-- tree that is produced.  By putting the common values first
-- the number of nodes (and number of different comparisons) can
-- be minimized.
for cont in SEDS.root:iterate_subtree(SEDS.container_filter) do
  local basetype = cont.basetype
  if (basetype) then
    if (not entitylist[basetype]) then
      entitylist[basetype] = {}
    end
    for node in cont:iterate_subtree(SEDS.constraint_filter) do
      if (node.attributes.entry) then
        node.resolved_entry = cont:find_entity(node.attributes.entry)
      end
      if (not node.resolved_entry) then
        node:error("undefined entry attribute",node.attributes.entry)
      else
        local refval = node.attributes.value
        local refent = node.attributes.entry
        local el = entitylist[basetype]
        if (not el[refent]) then
          el[refent] = {}
        end
        el = el[refent]
        el[refval] = (el[refval] or 0) + 1
      end
    end
  end
end

-- Determine a reasonable order for evaluation:
-- Get a "weight" for each offset which is the ratio
-- of the number of total derivatives that have a constraint
-- on that offset to the number of different possible values
-- The higher this number is, the earlier it should be checked.
local weights = {}
for cont,el in pairs(entitylist) do
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


for cont in SEDS.root:iterate_subtree(SEDS.container_filter) do

  local basetype = cont.basetype
  local entitymap = {}
  local entitylist = {}

  if (basetype) then

    for node in cont:iterate_subtree(SEDS.constraint_filter) do
      if (node.attributes.entry) then
        node.resolved_entry = cont:find_entity(node.attributes.entry)
      end
      if (not node.resolved_entry) then
        node:error("undefined entry attribute",node.attributes.entry)
      else
        local refent = node.attributes.entry
        if (entitymap[refent]) then
          node:error("Duplicate constraint", entitymap[refent])
        else
          entitymap[refent] = node
          entitylist[1 + #entitylist] = refent
        end
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
    table.sort(entitylist, function(a,b)
      local cweights = weights[basetype]
      if (cweights[a] == cweights[b]) then
        return (a < b) -- go by entity name if weights are equal
      else
        return (cweights[a] > cweights[b]) -- go by weight (highest first)
      end
    end)

    for _,entity in ipairs(entitylist) do
      local node = entitymap[entity]
      local refval = node.attributes.value
      local reftype = node.resolved_entry and node.resolved_entry[#node.resolved_entry].type

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


      if (not decisiontree_entry.entities) then
        decisiontree_entry.entities = {}
      end
      decisiontree_entry = decisiontree_entry.entities

      if (not decisiontree_entry[entity]) then
        decisiontree_entry[entity] = {}
      end
      decisiontree_entry = decisiontree_entry[entity]

      if (not decisiontree_entry.values) then
        decisiontree_entry.values = {}
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

  end
end


SEDS.info ("SEDS resolve constraints END")
