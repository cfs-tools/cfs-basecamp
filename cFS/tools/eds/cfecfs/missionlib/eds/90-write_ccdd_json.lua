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


local function dump_eds_info(params)
  print(string.format("dump_eds_info type=%s", SEDS.edslib.GetMetaData(params).TypeName))
  for iname,iinf in SEDS.edslib.IterateFields(params) do
    for vname,vinf in SEDS.edslib.IterateFields(iinf) do
      print(string.format(" %s.%s=%s", iname, tostring(vname), tostring(vinf)))
    end
  end

end

-- -------------------------------------------------------------------------
-- Helper function: drill down the bindings to locate a specific type of intf param
-- -------------------------------------------------------------------------
local function find_param_of_type(instance,typename)
  local result
  for i,binding in ipairs(instance.required_links) do
    if (binding.reqintf.type:get_qualified_name() == typename) then
      result = binding.reqinst.params
    elseif (binding.provinst) then
      result = find_param_of_type(binding.provinst,typename)
    end

    if (result) then
      break
    end
  end

  return result
end

-- -------------------------------------------------------------------------
-- Helper function: write json-formatted fragments of the container parameters
-- -------------------------------------------------------------------------
local function write_json_container_members(output,ptype,mode)
  local num_pads
  for _,ntype,refnode in ptype:iterate_members() do
    -- By checking refnode this includes only direct entries, not basetypes
    if (refnode) then
      local array_size
      local ctype
      local json_name

      if (ntype) then
        json_name = refnode.name

        -- If this is an array, move to the underlying type
        if (ntype.entity_type == "ARRAY_DATATYPE") then
          array_size = (array_size or 1) * ntype.total_elements
          ntype = ntype.datatyperef
        end

        -- Force certain output type translations that CCDD natively understands
        if (ntype.entity_type == "STRING_DATATYPE") then
          ctype = "char"
          array_size = (array_size or 1) * ntype.length
        elseif (SEDS.index_datatype_filter(ntype)) then
          ctype = (ntype.is_signed and "int") or "uint"
          ctype = ctype .. tostring(ntype.resolved_size.bits)
        elseif (array_size and ntype.entity_type == "BOOLEAN_DATATYPE") then
          -- HACK, fixup for boolean arrays, which are expressed as packed uint32s
          ctype = "uint32"
          array_size = math.ceil(array_size / 32)
        else
          ctype = (ntype.header_data and ntype.header_data.typedef_name) or ntype:get_flattened_name()
        end

      elseif (refnode.entity_type == "CONTAINER_PADDING_ENTRY") then

        -- Padding entry, output as an array of uint8 spare bytes
        -- Note that the name "Spare" is important, the CTF scripts do reference this field by name.
        if (num_pads) then
          num_pads = 1 + num_pads
          json_name = string.format("Spare%d", num_pads)
        else
          num_pads = 1
          json_name = "Spare"
        end
        if (refnode.sizeinbits <= 8) then
          array_size = 0
        else
          array_size = math.ceil(refnode.sizeinbits / 8)
        end
        ctype = "uint8"
      end

      -- If the result was translatable to JSON, then output now
      if (json_name) then
        output:append_previous(",")
        output:start_group("{")
        output:write(string.format("\"name\": \"%s\",", json_name))
        output:write(string.format("\"description\": \"%s\",", refnode.attributes.shortdescription or ""))

        -- Existing examples seem to write array size of 0 if it is not an array
        output:write(string.format("\"array_size\": \"%s\",", array_size or 0))
        output:write(string.format("\"data_type\": \"%s\",", ctype))

        -- The ccdd "bit_length" is for bitfields, not used in CFE CMD/TLM, so always output as 0
        output:write(string.format("\"bit_length\": \"%s\",", 0))
        -- HACK, recognize MsgID type values and treat these as atomics, not containers
        if (ntype and SEDS.container_filter(ntype) and ntype:get_qualified_name() ~= "CFE_SB/MsgId") then
          output:start_group("\"parameters\": [")
          write_json_container_members(output,ntype)
          output:end_group("]")
        else
          output:write("\"parameters\": []")
        end

        output:end_group("}")
      end
    end
  end
end

-- -------------------------------------------------------------------------
-- Helper function: write json-formatted fragments of command codes
-- -------------------------------------------------------------------------
local function write_cc_json_parameters(output,cc)

  output:write(string.format("\"cc_name\": \"%s\",", cc.macroname))
  output:write(string.format("\"cc_value\": \"%s\",", cc.value))
  output:write(string.format("\"cc_description\": \"%s\",", cc.argtype.attributes.shortdescription or ""))

  local ctype = cc.argtype.header_data and cc.argtype.header_data.typedef_name or cc.argtype:get_flattened_name()
  output:write(string.format("\"cc_data_type\": \"%s\",", ctype))

  -- Peek in the tree to see if this has its own entries
  -- If not then just output an empty JSON array for parameters
  if (cc.argtype:find_first("CONTAINER_ENTRY_LIST")) then
    output:start_group("\"cc_parameters\": [")
    write_json_container_members(output,cc.argtype)
    output:end_group("]")
  else
    -- no parameters, output an empty array
    output:write("\"cc_parameters\": []")
  end
end

-- -------------------------
-- MAIN ROUTINE
-- -------------------------

SEDS.output_mkdir("json")

for _,instance in ipairs(SEDS.highlevel_interfaces) do
  local ds = instance.component:find_parent(SEDS.basenode_filter)

  -- The various interfaces should be attached under required_links
  for _,binding in ipairs(instance.required_links) do
    local reqintf = binding.reqintf
    local intf_type_str = reqintf.type:get_qualified_name() 
    if (intf_type_str == "CFE_SB/Telemetry" or intf_type_str == "CFE_SB/Telecommand") then
      local cmd = reqintf.intf_commands and reqintf.intf_commands[1]

      -- SB interfaces should only have one "command" of the indication type (i.e. message appears on the bus)
      -- Note "command" here is the EDS terminology, not CFE - the CFE intf is referred to as "telecommand"
      -- The indication has one argument, which is the data associated with the message
      if (cmd.refnode.name == "indication") then
        local sb_params = find_param_of_type(binding.provinst,"CFE_SB/SoftwareBusRouting")
        local argtype = cmd.args[1].type
        local prefix

        if (reqintf.type.name == "Telecommand") then
          prefix = "cmd_"
        elseif (reqintf.type.name == "Telemetry") then
          prefix = "tlm_"
        else
          prefix = ""
        end

        if (sb_params) then
          local output = SEDS.output_open("json/" .. SEDS.to_filename(reqintf.name .. "_interface.json", ds.name));
          local msgid

          -- For now, just make up a symbolic name according to typical CFE patterns
          msgid = SEDS.to_macro_name(ds.name .. "_" .. reqintf.name .. "_MID")
          --msgid = sb_params.PubSub.MsgId -- This would be the actual hex msgid value

          output:start_group("{")
          output:write(string.format("\"%smid_name\": \"%s\",", prefix, msgid))
          output:write(string.format("\"%sdescription\": \"%s\",", prefix, reqintf.attributes.shortdescription or ""))
          if (cmd.cc_list) then
            output:start_group("\"cmd_codes\": [")
            for _,cc in ipairs(cmd.cc_list) do
              output:append_previous(",")
              output:start_group("{")
              write_cc_json_parameters(output, cc)
              output:end_group("}")
            end
            output:end_group("]")
          else
            local ctype = argtype.header_data and argtype.header_data.typedef_name or argtype:get_flattened_name()
            output:write(string.format("\"%sdata_type\": \"%s\",", prefix, ctype))
            output:start_group(string.format("\"%sparameters\": [", prefix, ctype))
            write_json_container_members(output, argtype)
            output:end_group("]")
          end
          output:end_group("}")

          SEDS.output_close(output)
        end

      end

    end

  end
end

--SEDS.error("Stop")
