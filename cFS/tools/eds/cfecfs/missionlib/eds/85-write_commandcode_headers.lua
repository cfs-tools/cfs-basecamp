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


-- -------------------------
-- MAIN ROUTINE
-- -------------------------

for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do

  -- Note that each interface under the component could have its own separate set of command codes,
  -- but most have just one.  The goal is to combine all the cc's into a single header file.

  local ds_all_subcmds = {}
  local ds_cmdcode_lookup = {}

  for comp in ds:iterate_subtree("COMPONENT") do
    -- Locate all required intfs under this component that utilize subcommand/function codes
    for reqintf in comp:iterate_subtree("REQUIRED_INTERFACE") do
      for _,cmd in ipairs(reqintf.intf_commands or {}) do

        if (cmd.subcommand_arg) then
          local argtype = cmd.args[cmd.subcommand_arg].type
          local constraint_values = {}
          local constraint_argtype = {}

          for _,subcommand in ipairs(argtype.edslib_derivtable_list) do
            -- Note that this list is not in value-order, the index relates to the lookup table, not the actual cmdcode value
            -- To get the command code value, need to drill down into the constraint set.  This assumes a single value constraint.
            local cmdname = subcommand:get_flattened_name()
            local constraint = subcommand:find_first("CONSTRAINT_SET")
            if (constraint) then
              constraint = constraint:find_first("VALUE_CONSTRAINT")
            end
            if (constraint) then
              constraint = constraint.attributes["value"]
            end
            if (constraint) then
              constraint_values[1 + #constraint_values] = constraint
              constraint_argtype[constraint] = subcommand
            end
          end

          table.sort(constraint_values)
          cmd.cc_list = {}
          for i,constraint in ipairs(constraint_values) do
            local subcommand = constraint_argtype[constraint]
            local macroname = SEDS.to_macro_name(subcommand:get_flattened_name())
            -- If the command name ends in "_CMD" then lop it off
            if (string.sub(macroname, -4, -1) == "_CMD") then
              macroname = string.sub(macroname, 1, -5)
            end
            macroname = macroname .. "_CC"
            cmd.cc_list[i] = { value = constraint, argtype = subcommand, macroname = macroname }
          end

          ds_all_subcmds[1 + #ds_all_subcmds] = { intf=reqintf, cmd=cmd }
        end
      end
    end
  end

  -- Create a "cc.h" header file containing the command/function codes specificed as EDS value constraints
  -- This info was collected earlier, but is easier to use in C files if not mixed with the interface declarations
  -- It also needs to be put in a reasonable order and the macro names need to be scrubbed/fixed to match
  if (#ds_all_subcmds > 0) then
    hdrout = SEDS.output_open(SEDS.to_filename("cc.h", ds.name),ds.xml_filename)
    for _,sc in ipairs(ds_all_subcmds) do
      hdrout:section_marker(string.format("Command Codes for \'%s\' Interface",sc.intf.name))
      for _,cc in ipairs(sc.cmd.cc_list) do
        hdrout:add_documentation(string.format("Command code associated with %s", SEDS.to_ctype_typedef(cc.argtype)))
        hdrout:write(string.format("#define %-60s %s",  cc.macroname, cc.value))
      end
      hdrout:add_whitespace(1)
    end
    SEDS.output_close(hdrout)
  end

end
