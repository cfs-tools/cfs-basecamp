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
-- Lua Method implementation for "instance" objects
--
-- This returns a table object which contains a static set of methods that
-- operate on instance objects (i.e. "seds_instance" userdata.
-- -------------------------------------------------------------------------

SEDS.info "loading SEDS component instance methods"

-- -----------------------------------------------------------------------
---
-- get_qualified_name: obtains the fully-qualified name of an instance object
--
-- The fully qualified name is the name of the component (the instance type)
-- combined with the node that demanded it, for on demand entities.
--
local function get_qualified_name(node)
  local name
  if (node.trigger) then
    name = node.trigger:get_qualified_name()
  end
  if (name) then
    name = name .. "/" .. node.component.name
  else
    name = node.component:get_qualified_name()
  end
  return name
end

-- -----------------------------------------------------------------------
---
-- debug_print: obtains the fully-qualified name of an instance object
--
-- Print the node properties on the console for debugging purposes
--
local function debug_print(node)
  print ("-- BEGIN INSTANCE PROPERTIES --")
  for i,v in ipairs(node:get_properties()) do
    local propval = node[v]
    print (" [" .. tostring(v) .. "] =>" .. type(propval) .. ":" .. tostring(propval))
    if (type(propval) == "table") then
      for tk,tv in pairs(propval) do
        print ("    [" .. tostring(tk) .. "] => " .. type(tv) .. ":" .. tostring(tv))
      end
    end
  end
  print ("-- END INSTANCE PROPERTIES --")
end

-- Return a table of methods for instance objects
return {
  get_qualified_name = get_qualified_name,
  debug_print = debug_print
}

