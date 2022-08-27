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
-- Lua implementation of "resolve components" processing step
--
-- This stage creates the "instance nodes" that signify an actual realization of
-- and EDS-defined component.  It relies on a set of "instance rules" (defined separately)
-- to determine what components have instances and how many.
--
-- Three basic patterns are implemented (specified by the rule):
--   singleton: One and only one instance is always created, which "provides" interfaces
--       to many other dependent components.  This is usually for the low level components.
--   matchname: An instance of every component matching the given name will be created
--       This pattern is usually appropriate for high level application components.
--   ondemand: An instance is created as necessary to satisfy dependencies of other
--       components.  This pattern is used for the intermediate layers, and a 1:1
--       relationship exists between provided and required interfaces.

-- -------------------------------------------------------------------------
SEDS.info ("SEDS resolve components START")

-- -----------------------------------------------------------------------------------------
--                              Main component resolution routine
-- -----------------------------------------------------------------------------------------

local pending_instance_list = {}
local lowlevel_interfaces = {}
local highlevel_interfaces = {}
local num_instances = 0

-- Do a single scan through the instance rules and perform initial instantiations
-- This includes singleton components and anything matched by a "matchname" pattern
-- Note that "ondemand" patterns are ignored for now; those are instantiated later to satisfy dependencies
for rule in SEDS.root:iterate_subtree("INSTANCE_RULE") do
  local pattern = rule.attributes.pattern
  if (pattern == "singleton") then
    -- Singletons should have a "component" attribute that identifies the component to instantiate
    -- As the pattern suggests, only a single one will be created, and it will be created now.
    if (not rule.component) then
      rule:error("invalid singleton component")
    else
      pending_instance_list[1 + #pending_instance_list] =
        SEDS.new_component_instance(rule.component, rule)
    end
  elseif (pattern == "matchname") then
    -- For the "matchname" pattern, search all defined components
    -- any component with a matching name will be instantiated here
    for comp in SEDS.root:iterate_subtree("COMPONENT") do
      if (comp.name == rule.name) then
        pending_instance_list[1 + #pending_instance_list] =
          SEDS.new_component_instance(comp, rule)
      end
    end
  end
end

-- Loop through the pending instances and determine what will fulfill the required dependencies
while (#pending_instance_list > 0 and SEDS.get_error_count() == 0) do
  local resolve_list = pending_instance_list
  pending_instance_list = {}
  for i,instance in ipairs(resolve_list) do
    local req_count = 0
    local prov_count = 0
    num_instances = num_instances + 1
    instance.id = num_instances
    for reqintf in instance.component:iterate_subtree("PROVIDED_INTERFACE") do
      prov_count = prov_count + 1
    end
    for reqintf in instance.component:iterate_subtree("REQUIRED_INTERFACE") do
      local provider_inst
      req_count = req_count + 1

      -- Find a rule to select a provider for the required interface
      local mapping
      for map in instance.rule:iterate_subtree("INTERFACE_MAP") do
        if (map.type == reqintf.type) then
          mapping = map
          break
        end
      end

      -- Sanity checks on the mapping rule selected
      if (not mapping or not mapping.rule) then
        reqintf:error("no rule to map required interface")
        return
      end

      -- The rule should refer to a specific component
      local provcomp = mapping.rule.component
      if (not provcomp) then
        mapping.rule:error("rule does not refer to a valid component")
        return
      end

      -- Verify that the selected component actually provides the interface type
      local provintf
      for intf in provcomp:iterate_subtree("PROVIDED_INTERFACE") do
        if (intf.type == reqintf.type) then
          provintf = intf
          break
        end
      end
      if (not provintf) then
        reqintf:error(string.format("%s does not provide required interface", provcomp), reqintf.type)
        return
      end

      if (mapping.rule.attributes.pattern == "ondemand") then
        -- For an ondemand pattern, create a new instance
        provider_inst = SEDS.new_component_instance(provcomp, mapping.rule, instance.trigger or instance.component)
        pending_instance_list[1 + #pending_instance_list] = provider_inst
      else
        -- For a other pattern (e.g. singleton), reuse the existing instance
        provider_inst = mapping.rule.instance_list[1];
      end
      if (not provider_inst) then
        -- Should have been created already
        mapping.rule:error("no available instance of component",provcomp)
      end

      -- the provided/required interface types should point back to the same declared interface
      -- this declared interface has a set of parameters associated with it
      local params = {}
      for param in provintf.type:iterate_subtree("PARAMETER") do
        params[param.name] = { type = param.type }
      end

      for parmval in mapping:iterate_children("PARAMETER_VALUE") do
        if (params[parmval.name]) then
          params[parmval.name].method = parmval.attributes.method
          params[parmval.name].value = parmval.attributes.value
        end
      end

      -- Establish the connection between the provider and the requirer
      local binding = {
        reqinst = instance,
        reqintf = reqintf,
        provinst = provider_inst,
        provintf = provintf,
        params = params
      }
      provider_inst.provided_links[1 + #provider_inst.provided_links] = binding
      instance.required_links[1 + #instance.required_links] = binding
    end

    -- If there were no required interfaces, treat this as a "base" interface
    -- from which all communication is derived.  In the cFS world there should be
    -- only one of these and it should be the logical software bus.  This entertains
    -- the possibilty that there is more than one of these such interfaces however
    -- that is not really an expected mode of operation
    if (req_count == 0) then
      lowlevel_interfaces[1 + #lowlevel_interfaces] = instance
    end

    -- Likewise if the component does not provide interfaces to another component,
    -- treat it as a top level (leaf) interface that belongs to application
    if (prov_count == 0) then
      highlevel_interfaces[1 + #highlevel_interfaces] = instance
    end
  end
end

SEDS.lowlevel_interfaces = lowlevel_interfaces
SEDS.highlevel_interfaces = highlevel_interfaces

SEDS.info ("SEDS resolve components END")
