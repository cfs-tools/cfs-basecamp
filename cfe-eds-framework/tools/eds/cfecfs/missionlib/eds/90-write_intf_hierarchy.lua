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

-- ---------------------------------------------
-- generate interface hierarchy graphic
-- This generates dot files based on the interface heirarchy which can be
-- easily turned into graphics using the "dot" tool.
-- This script should run late in the processing stages
-- ---------------------------------------------

local function add_edges(output,inst)
  local desc = inst.component:get_qualified_name()
  for _,binding in ipairs(inst.required_links) do
    local params = inst.params
    if (params) then
      for iname,iinf in SEDS.edslib.IterateFields(params) do
        for vname,vinf in SEDS.edslib.IterateFields(iinf) do
          desc = desc .. string.format("\\n%s.%s=%s", iname, tostring(vname), tostring(vinf))
        end
      end
    end
    output:write(string.format("id%03d -> id%03d [label=\"%s\"]", inst.id, binding.provinst.id, binding.reqintf.name))
    add_edges(output,binding.provinst)
  end
  output:write(string.format("id%03d [label=\"%s\"]", inst.id, desc))
end


for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  local comps = {}
  for _,instance in ipairs(SEDS.highlevel_interfaces) do
    local app_ds = instance.component:find_parent(SEDS.basenode_filter)
    if (app_ds == ds) then
      comps[1 + #comps] = instance
    end
  end

  if (#comps > 0) then
    local output = SEDS.output_open(SEDS.to_filename("interfaces.dot", ds.name));
    output:start_group("digraph mission {")
    output:write("rankdir=\"LR\"")
    for _,instance in ipairs(comps) do
      add_edges(output,instance)
    end
    output:end_group("}")
    SEDS.output_close(output)
  end
end

