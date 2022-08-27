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
-- Lua implementation of "parse defines" processing step
--
-- This should always be the very first script to run, which replaces the
-- ${} placeholder syntax in EDS files with real values.
--
-- An "attributes" property is added to every DOM node, with the
-- with the same keys as "xml_attrs" but placeholders resolved
--
-- NOTE: the semantics of this replacement is not specified by CCSDS book 876,
-- which specfies it as an implementation-defined substitution.
-- -------------------------------------------------------------------------

SEDS.info "SEDS parse defines START"

local eval_global_syms = { }
local final_global_syms = { }

-- Helper function to evaluate a single symbolic value
local basic_define_evaluate = function(context_node, define_value)
  local ref = context_node:find_reference(define_value, "DEFINE");
  local result
  if (ref) then
    if (ref.eval_attrs) then
      result = ref.eval_attrs["value"]
    end
    if (result == nil and ref.attributes) then
      result = ref.attributes["value"]
    end
    if (result == nil and ref.xml_attrs) then
      result = ref.xml_attrs["value"]
    end
  end
  -- as a fallback, check the local symbol tables
  if (not result) then
    result = eval_global_syms[define_value] or final_global_syms[define_value]
  end
  while (type(result) == "function") do
    result = result()
  end
  return result
end

local default_define_evaluate = function(define_value)
  return basic_define_evaluate(SEDS.root, define_value)
end


-- ---------------------------------------
-- MAIN ROUTINE BEGINS
-- ---------------------------------------

-- Step 1a: Generate an "eval" function for all command line parameters
-- Store it in a local table "eval_global_syms"
for name,val in pairs(SEDS.commandline_defines) do
  if (type(val) == "string") then
    local func,msg = SEDS.load_refs(val)
    if (not func) then
      error(msg)
    else
      eval_global_syms[name] = func
    end
  elseif (type(val) == "number" or type(val) == "boolean") then
    final_global_syms[name] = val
  end
end

-- Step 1b: Generate an "eval" function for all attributes
-- Store it in a new property, "eval_attrs", under each node
for n in SEDS.root:iterate_subtree() do
  local eval_attrs = {}
  for a,v in pairs(n.xml_attrs or {}) do
    local func,msg = SEDS.load_refs(v)
    if (not func) then
      n:error(msg)
    else
      eval_attrs[a] = func
    end
  end
  n.eval_attrs = eval_attrs
end

-- ------------------
--  Call the eval function
-- ------------------

-- Step 2a: Call the "eval" function for all command line parameters
-- Store it in a local table "final_global_syms"
SEDS.set_eval_func(default_define_evaluate)
for name,val in pairs(SEDS.commandline_defines) do
  local status = true
  local result = eval_global_syms[name]
  while (status and type(result) == "function") do
    status,result = pcall(result)
  end
  if (not status) then
    error(tostring(val))
  else
    SEDS.info(string.format("GLOBAL: %s=%s", name, result or "(nil)"))
    final_global_syms[name] = result
  end
end

-- Step 2b: Call the "eval" function for all attributes
-- Store result in a new property "attributes" on each node
for n in SEDS.root:iterate_subtree() do
  local attributes = {}

  -- The evaluate function is defined locally, such that
  -- it can use the value of "n" as a context to evaulate references
  SEDS.set_eval_func(function(v)
    return basic_define_evaluate(n, v)
  end)

  if (n.xml_attrs) then
    for a,v in pairs(n.xml_attrs) do
      if (n.eval_attrs) then
        v = n.eval_attrs[a]
        local status = true
        while (status and type(v) == "function") do
          status,v = pcall(v)
        end
        if (not status) then
          n:error(tostring(v))
          v = nil
        end
      end
      attributes[a] = v
    end
  end
  n.attributes = attributes
end

-- ------------------
--  cleanup
-- ------------------

-- Step 3: remove the "eval_attrs" from every node (not needed anymore)
-- discard any evaluate function
SEDS.set_eval_func(nil)
for key in pairs(eval_global_syms) do
  eval_global_syms[key] = nil
end
for n in SEDS.root:iterate_subtree() do
  n.eval_attrs = nil
end

-- Provide a library function for any subsequent scripts to lookup a define
SEDS.get_define = default_define_evaluate

SEDS.info "SEDS parse defines END"
