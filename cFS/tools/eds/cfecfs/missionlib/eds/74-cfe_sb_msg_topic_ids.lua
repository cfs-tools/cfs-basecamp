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

local mts_comp = SEDS.root:find_reference("CFE_SB/MTS", "COMPONENT")

if (type(mts_comp) ~= "userdata") then
    SEDS.error("CFE_SB/MTS component is not defined")
    return
end

local components = {}

-- -----------------------------------------------------
-- helper function to determine a complete "interface chain"
-- -----------------------------------------------------
--[[
Each Software Bus routing path should involve a chain of interfaces:
MTS -> PubSub -> (Telecommand|Telemetry) -> Application
--]]

local function get_chain(inst)

  local chain = { }
  local found_chain = false
  local do_get_chain

  do_get_chain = function(inst)
    if (inst) then
      found_chain = (inst.component == mts_comp)
      if (not found_chain) then
        for i,binding in ipairs(inst.required_links) do
          chain[1 + #chain] = binding
          do_get_chain(binding.provinst)
          if (found_chain) then
            break
          end
          chain[#chain] = nil
        end
      end
    end
  end

  do_get_chain(inst)
  return chain[1], chain[2], chain[3]
end

-- -----------------------------------------------------
-- Main routine starts here
-- -----------------------------------------------------
--[[
  First, find the chain for each entry in the SEDS.highlevel_interfaces list
  If it is a CFE_SB interface (pubsub) then identify the MsgId and TopicId
  Build a reverse-map table for MsgId and TopicId and ensure there are no duplicates

  Also note - this puts the topicid table at the MTS level, ensuring uniqueness of topic IDs
  across the entire MTS.  Technically only MsgIds need to be unique at the MTS (subnetwork) level,
  as TopicIDs could be qualified at the access level (CMD or TLM).  However it is just simpler to
  manage if all the topic IDs are unique too.  This way, all that is needed to pass around is just
  strictly a topic ID alone, it does not need to be a combination of (topic ID + access intf).
--]]

mts_comp.msgid_table = {}
mts_comp.topicid_table = {}

for _,instance in ipairs(SEDS.highlevel_interfaces) do
  for _,binding in ipairs(instance.required_links) do
    local tctm, pubsub = get_chain(binding.provinst)
    if (pubsub) then
      components[instance.component] = true
      local chain = {
        binding = binding,
        tctm = tctm.reqinst.params[binding.provintf.name](),
        pubsub = pubsub.reqinst.params[tctm.provintf.name]()
      }
      local MsgId = chain.pubsub.MsgId.Value
      local TopicId = chain.tctm.TopicId
      -- Build the reverse lookup tables for Message Id and Topic Id
      -- Ensure that there are no conflicts between these indices
      if (mts_comp.msgid_table[MsgId]) then
        local exist_comp = mts_comp.msgid_table[MsgId].binding
        exist_comp.reqintf:error(string.format("MsgId Conflict on MsgId=%04x/TopicId=%d with %s",MsgId,TopicId,binding.reqintf:get_qualified_name()))
      else
        mts_comp.msgid_table[MsgId] = chain
      end
      if (mts_comp.topicid_table[TopicId]) then
        local exist_comp = mts_comp.topicid_table[TopicId].binding
        exist_comp.reqintf:error(string.format("TopicId Conflict on MsgId=%04x/TopicId=%d with %s",MsgId,TopicId,binding.reqintf:get_qualified_name()))
      else
        mts_comp.topicid_table[TopicId] = chain
      end
    end
  end
end

-- Export the findings via the SEDS global
SEDS.mts_info = {
    mts_component = mts_comp,  -- The base (low level) MTS subnetwork component
    component_set = components -- The keys of this table are the components that use this MTS
}

--SEDS.error("Stop")