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
-- Lua implementation of "resolved_seds" EdsLib processing step
--
-- The objective of this script is to generate SEDS XML files
-- that have the preprocessor attributes filled in with real values
-- used in the mission.  These files may be passed to other tools which
-- may not understand the ${} replacement syntax
-- -------------------------------------------------------------------------

-- -------------------------------------------------------------------------
-- write_node Helper function
-- This "recreates" the EDS XML from the DOM tree.  However, instead of using
-- the original attributes values in "xml_attrs", this uses the resolved attributes
-- instead.  Additionally, it will recurse and generate XML for all nodes within
-- before generating the XML closing tag
-- -------------------------------------------------------------------------
local function write_node(output,node,name_prefix)

  -- Implicitly generated nodes should not get put into exported XML
  -- nothing to do for these.
  if (node.implicit) then
    return
  end

  -- Regenerate the NAME/SPACE/SEPARATOR structure that was broken up when imported
  -- Just go down to the child elements, but no output for these nodes.
  local my_attrs = node.xml_element
  if (not my_attrs) then
    if (node.name) then
      name_prefix = (name_prefix or "") .. node.name .. "/"
      for child in node:iterate_children() do
        write_node(output,child,name_prefix)
      end
    end
    return
  end

  -- Normal node processing starts here .
  if (node.name and node.entity_type ~= "PACKAGEFILE" and node.entity_type ~= "DATASHEET") then
    my_attrs = string.format("%s %s=\"%s\"",my_attrs,
        node.entity_type == "ENUMERATION_ENTRY" and "label" or "name", (name_prefix or "") .. node.name)
  end
  for attr in SEDS.sorted_keys(node.xml_attrs) do
    local attrval = node.attributes[attr] or node.xml_attrs[attr]
    if (type(attrval) == "number") then
      local whole,frac = math.modf(attrval)
      if (frac == 0) then
        -- be sure to export as a "fixed point" and not add any
        -- unintended decimal point or scientific notation
        attrval = string.format("%.0f",attrval)
      else
        attrval = string.format("%g",attrval)
      end
    end
    my_attrs = string.format("%s %s=\"%s\"",my_attrs, node.xml_attrname[attr], tostring(attrval))
  end
  local cdata = ""
  if (node.xml_cdata) then
    cdata = node.xml_cdata:gsub("^%s*(.-)%s*$", "%1")
    cdata = cdata:gsub("&", "&amp;")
    cdata = cdata:gsub("<", "&lt;")
    cdata = cdata:gsub(">", "&gt;")
  end
  if (node.subnodes or node.longdescription) then
    output:start_group(string.format("<%s>",my_attrs))
    if (node.longdescription) then
      local s = node.longdescription:gsub("^%s*(.-)%s*$", "%1") .. "\n"
      s = s:gsub("&", "&amp;")
      s = s:gsub("<", "&lt;")
      s = s:gsub(">", "&gt;")
      output:start_group("<LongDescription>")
      for l in string.gmatch(s, "(.-)\n") do
        l = l:gsub("^%s*","")
        if (#l > 0) then
          output:write(l)
        else
          output:add_whitespace(1)
        end
      end
      output:end_group("</LongDescription>")
    end
    output:write(cdata)
    -- Recursively write any child node contents
    for child in node:iterate_children() do
      write_node(output,child)
    end
    output:end_group(string.format("</%s>",node.xml_element))
  elseif(string.len(cdata) > 0) then
    output:write(string.format("<%s>%s</%s>",my_attrs,cdata,node.xml_element))
  else
    output:write(string.format("<%s/>",my_attrs))
  end
end

-- -------------------------
-- MAIN ROUTINE
-- -------------------------
-- Just loop through each datasheet and call the "write_node" function,
-- which recursively handles everything beneath it.
for ds in SEDS.root:iterate_children({"DATASHEET","PACKAGEFILE"}) do
  local output = SEDS.output_open(ds.name .. ".xml", ds.xml_filename, "xml")
  write_node(output,ds)
  SEDS.output_close(output)
end

