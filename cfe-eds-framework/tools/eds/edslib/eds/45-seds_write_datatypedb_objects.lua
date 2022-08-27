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
-- Lua implementation of "write data type db objects" EdsLib processing step
--
-- This creates the C source files containing the objects that map the EDS
-- binary blobs into native C structures and vice versa.
-- -------------------------------------------------------------------------
SEDS.info ("SEDS write basic objects START")

-- -----------------------------------------------------------------------------------------
--                              Helper functions
-- -----------------------------------------------------------------------------------------

-- -----------------------------------------------------------------------
-- Calls a function to get a set of structure field values, and merges
-- that function output into a summary table
-- -----------------------------------------------------------------------
local function do_get_fields(source,pfields,...)
  local lfields
  if (type(source) == "function") then
    lfields = source(...)
  elseif(type(source) == "table") then
    for _,subsrc in ipairs(source) do
      lfields = do_get_fields(subsrc,lfields,...)
    end
  end

  if (lfields) then
    if (pfields) then
      -- Merge the local fields into the parent fields
      for k,v in pairs(lfields) do
        pfields[k] = v
      end
    else
      -- No parent fields, so just use local fields
      pfields = lfields
    end
  elseif (not pfields) then
    pfields = {}  -- be sure not to return nil
  end

  return pfields
end

-- -----------------------------------------------------------------------
-- Outputs selected fields in a field value table
-- The fields will be output in the order specified by fieldlist
-- -----------------------------------------------------------------------
local function do_write_field_list(output,pfields,fieldlist)
  for _,k in ipairs(fieldlist) do
    if (pfields[k]) then
      output:append_previous(",")
      output:write(string.format(".%s = %s",k,pfields[k]))
    end
  end
end

-- -----------------------------------------------------------------------
-- Outputs all fields in a field value table
-- The fields will be in alphanumeric order by field name
-- -----------------------------------------------------------------------
local function do_write_all_fields(output,pfields)
  for k in SEDS.sorted_keys(pfields) do
    output:append_previous(",")
    output:write(string.format(".%s = %s",k,pfields[k]))
  end
end


-- -----------------------------------------------------------------------
-- Get standard fields for a "container entry" in the C DB
-- -----------------------------------------------------------------------
local function write_normal_entry_handler(output,ds,parent_name)
  return {
    EntryType = "EDSLIB_ENTRYTYPE_" .. (ds.entry and ds.entry.entity_type or "BASE_TYPE"),
    Offset = string.format("{ .Bits = %d, .Bytes = offsetof(struct %s,%s) }",
        ds.bit, parent_name, ds.name or ds.type.name),
    RefObj = ds.type and ds.type.edslib_refobj_initializer
  }
end



-- -----------------------------------------------------------------------
-- Generate source for a polynomial calibrator function
-- -----------------------------------------------------------------------
local function write_c_integer_polynomial_calibrator(output,calnode,calname)

  local coefficients = {}
  local fwdname
  local revname
  local maxord = 0
  local divisor = 1

  for term in calnode:iterate_children("POLYNOMIAL_TERM") do
    local exp = math.floor(term.attributes.exponent or 0)
    if (exp >= 0) then
      coefficients[exp] = (coefficients[exp] or 1) * (term.attributes.coefficient or 1)
      if (exp > maxord) then
        maxord = exp
      end
    end
  end

  -- As this is using integer arithmetic, any fractional
  -- part of the coefficients must be converted to an integer
  -- ratio.  This will determine a reasonable integer divisor
  -- to use for this.
  -- Of course this is not always possible, and in cases where
  -- it is not possible this will be an approximation.

  for exp = 0,maxord do
    if (not coefficients[exp]) then
      coefficients[exp] = 0
    elseif (coefficients[exp] ~= 0) then
      local whole = math.abs(coefficients[exp]) * divisor
      local digits = 0

      -- Multiply by 10 until the coefficient becomes a whole number
      -- Put a reasonable limit here - up to 6 digits is probably sufficient,
      -- after this we will just truncate it.
      while (digits < 6 and whole ~= math.floor(whole)) do
        whole = 10 * whole
        divisor = 10 * divisor
        digits = 1 + digits
      end

      -- Reduce the fraction if possible
      for x = 1,digits do
        local test = whole / 5
        if (test ~= math.floor(test)) then
          break
        end
        divisor = divisor / 5
        whole = test
      end
      for x = 1,digits do
        local test = whole / 2
        if (test ~= math.floor(test)) then
          break
        end
        divisor = divisor / 2
        whole = test
      end

    end
  end

  -- Avoid giant values which may cause integer overflow.
  -- This is not perfect of course, but it should give
  -- a fair approximation in case the EDS specified a
  -- bizarre coefficient.  It is almost certain that
  -- anything bigger than 10^8 will overflow an int32
  while (divisor >= 100000000) do
    divisor = divisor / 10
  end

  -- Multiply all coefficients by the final divisor,
  -- and truncate to be an integer
  divisor = math.floor(divisor)
  for exp = 0,maxord do
    coefficients[exp] = math.modf(coefficients[exp] * divisor)
  end

  fwdname = calname .. "_FWD"
  output:write(string.format("static intmax_t %s(intmax_t x)",fwdname))
  output:start_group("{")
  output:write(string.format("intmax_t y = %d;", coefficients[0]))
  if (maxord > 1) then
    output:write("intmax_t exp = x;")
  end
  for exp = 1,maxord do
    local term
    if (exp > 1) then
      output:write("exp *= x;");
      term = " * exp"
    elseif (exp > 0) then
      term = " * x"
    else
      term = ""
    end
    if (coefficients[exp] ~= 0) then
      output:write(string.format("y += %d%s;", coefficients[exp], term))
    end
  end

  output:write(string.format("y /= %d;", divisor))
  output:write("return y;")
  output:end_group("}")
  output:add_whitespace(1)

  -- Also generate a "reverse" version of the polynomial calibration
  -- For sanity reasons this is only done for linear conversions (order=1)
  if (maxord == 1) then
    revname = calname .. "_REV"
    output:write(string.format("static intmax_t %s(intmax_t y)",revname))
    output:start_group("{")
    output:write(string.format("intmax_t x = y * %d;",divisor))
    output:write(string.format("x -= %d;", coefficients[0]))
    if (coefficients[1] ~= 1) then
      output:write(string.format("x /= %d;", coefficients[1]))
    end
    output:write("return x;")
    output:end_group("}")
    output:add_whitespace(1)
  end

  return {
    ["HandlerArg.IntegerCalibrator.Forward"] = fwdname,
    ["HandlerArg.IntegerCalibrator.Reverse"] = revname
  }
end

-- -----------------------------------------------------------------------
-- Generate source for a spline calibrator (placeholder; not currently implemented)
-- -----------------------------------------------------------------------
local function write_c_integer_spline_calibrator(output,calnode)
  output:write(string.format("#error Spline Not implemented"))
end

local write_c_calibrator_code = {
  POLYNOMIAL_CALIBRATOR = write_c_integer_polynomial_calibrator,
  SPLINE_CALIBRATOR = write_c_integer_spline_calibrator
}

-- -----------------------------------------------------------------------
-- Generate fields for a container length entry (uses calibrator)
-- -----------------------------------------------------------------------
local function write_length_entry_handler(output,ds,parent_name)
  local fields
  local cal_name = string.format("%s_%s_CALIBRATOR", parent_name, ds.entry.name)
  for caltype in ds.entry:iterate_children({"POLYNOMIAL_CALIBRATOR","SPLINE_CALIBRATOR"}) do
    fields = do_get_fields(write_c_calibrator_code[caltype.entity_type], fields, output, caltype, cal_name)
  end
  if (not fields["HandlerArg.IntegerCalibrator.Reverse"]) then
    ds.entry:error("Calibration is not reversible as required for a LengthEntry")
  end
  return fields
end

-- -----------------------------------------------------------------------
-- Generate fields for a container Fixed Value entry
-- -----------------------------------------------------------------------
local function write_fixedvalue_entry_handler(output,ds,parent_name)
  local argtype
  local retval = {}
  local argvalue = ds.entry.attributes.fixedvalue
  if (argvalue == nil) then
    ds.entry:error("fixedValue attribute not defined")
  end
  if (ds.type.entity_type == "FLOAT_DATATYPE") then
    retval["HandlerArg.FixedFloat"] = tonumber(argvalue)
  elseif (ds.type.entity_type == "INTEGER_DATATYPE" or
          ds.type.entity_type == "BOOLEAN_DATATYPE") then
    retval["HandlerArg.FixedInteger"] = tonumber(argvalue)
  else
    retval["HandlerArg.FixedString"] = string.format("\"%s\"", argvalue)
  end
  return retval
end

-- -----------------------------------------------------------------------
-- Generate fields for a container error control entry
-- -----------------------------------------------------------------------
local function write_errorcontrol_entry_handler(output,ds,parent_name)
  -- algorithms are predefined by book 876, just reference them
  local impl_name = string.format("EdsLib_ErrorControlType_%s", ds.entry.attributes.errorcontroltype)
  return { ["HandlerArg.ErrorControl"] = impl_name }
end

local function write_list_entry_handler(output,entrynode,parent_checksum)

end


local special_entry_handler_table = {
  CONTAINER_LIST_ENTRY = write_list_entry_handler,
  CONTAINER_LENGTH_ENTRY = write_length_entry_handler,
  CONTAINER_FIXED_VALUE_ENTRY = write_fixedvalue_entry_handler,
  CONTAINER_ERROR_CONTROL_ENTRY = write_errorcontrol_entry_handler
}

-- -----------------------------------------------------------------------------------------
--                              C header output routine
-- -----------------------------------------------------------------------------------------

-- -----------------------------------------------------------------------
-- Generate fields to indicate the size of an object
-- -----------------------------------------------------------------------
-- This operates on anything with a "resolved_size" attribute
local function get_basic_size_fields(output,node)
  local rsize = node.resolved_size
  local objsize
  local flags
  local checksum
  if (rsize) then
    checksum = "0x" .. rsize.checksum
    if (node.header_data and rsize.bits > 0) then
      objsize = string.format("{ .Bits = %d, .Bytes = sizeof(%s) }",rsize.bits,node.header_data.typedef_name)
    end
    flags = rsize.is_packed
    if (flags) then
      flags = "EDSLIB_DATATYPE_FLAG_PACKED_" .. flags
    end
  end
  return { SizeInfo = objsize, Checksum = checksum, Flags = flags }
end

-- -----------------------------------------------------------------------
-- Generate fields for the entry list of a container or interface
-- -----------------------------------------------------------------------
-- This operates on anything with a "decode_sequence" property
local function write_c_decode_sequence(output, listnode)

  if (not listnode or
      not listnode.decode_sequence or
      #listnode.decode_sequence == 0) then
    -- Nothing to do
    return
  end

  local checksum = listnode.resolved_size.checksum
  local objname = string.format("%s_%s", output.datasheet_name, checksum)
  local entrylist = {}
  local pfield
  local nameext


  if (listnode.entity_type == "CONTAINER_TRAILER_ENTRY_LIST") then
    pfield = "TrailerEntryList"
    nameext = objname .. "_TRAILER_ENTRY_LIST"
  else
    pfield = "EntryList"
    nameext = objname .. "_ENTRY_LIST"
  end

  -- Call handlers to get C DB field values for every entry in the decode sequence
  if (not output.checksum_table[checksum]) then
    output.checksum_table[checksum] = objname
    local c_type_name = SEDS.to_safe_identifier(listnode:get_qualified_name())

    for i,ds in ipairs(listnode.decode_sequence) do
      entrylist[i] = do_get_fields({
        write_normal_entry_handler,
        ds.entry and special_entry_handler_table[ds.entry.entity_type]
      }, nil, output, ds, c_type_name)
    end

    output:write(string.format("static const EdsLib_FieldDetailEntry_t %s[] =", nameext))
    output:start_group("{")
    for _,fields in ipairs(entrylist) do
      output:append_previous(",")
      output:start_group("{")
      do_write_all_fields(output,fields)
      output:end_group("}")
    end
    output:end_group("};")
    output:add_whitespace(1)

  end



  return { [pfield] = nameext, [pfield .. "Size"] = #listnode.decode_sequence }

end

-- -----------------------------------------------------------------------
-- Helper functions for for the constraint/value list of a container
-- -----------------------------------------------------------------------
-- This is intended to provide a lookup table based on constraint entities
local collect_descisiontree_entities
local collect_descisiontree_values

collect_descisiontree_values = function (state, low, high)

  if (low == high) then
    -- Done, nothing more to do here.
    return
  end

  local median = math.floor((1 + high + low) / 2)
  local value = state.valuelist[median]
  local vtree = state.vtree[value]
  local refidx, resultmark, lessmark, greatermark, entmark, selfidx

  -- Output all dependent entries and record "watermarks" along the way
  -- This because some of these need to point to this value node, but we
  -- do not have a specific position yet, so it has to be filled later
  refidx = #state.list

  -- Add additional values greater than the current value, if any
  collect_descisiontree_values(state,median,high)
  if (refidx ~= #state.list) then
    greatermark = #state.list
    refidx = greatermark
  end

  -- Add additional values less than than the current value, if any
  collect_descisiontree_values(state,low,median-1)
  if (refidx ~= #state.list) then
    lessmark = #state.list
    refidx = lessmark
  end

  -- Add additional entities to check, if any
  collect_descisiontree_entities(state.list, vtree.entities)
  entmark = #state.list

  -- Add "local" results for matches not subject to more identification here
  for _,deriv in ipairs(vtree.derivatives or {}) do
    state.list[1 + #state.list] = { result = deriv }
  end
  resultmark = #state.list

  -- Add the entry for _this value_
  selfidx = 1 + #state.list

  -- Go back in history and set all previously-output items that
  -- need to refer to this value entry, now that we have a specific index
  if (refidx ~= entmark) then
    refidx = entmark
    state.list[entmark].ParentOperation = selfidx
  else
    entmark = nil
  end

  if (refidx ~= resultmark) then
    while (refidx < resultmark) do
      refidx = 1 + refidx
      state.list[refidx].ParentOperation = selfidx
    end
  else
    resultmark = nil
  end


  -- At last, actually add the node for the current value
  state.list[selfidx] = {
    val = value,
    NextOperationGreater = greatermark,
    NextOperationLess = lessmark
  }
  state.idxset[selfidx] = true

end

collect_descisiontree_entities = function (list,etree)
  local ostate = { list = list }
  for entity in SEDS.sorted_keys(etree) do
    ostate.idxset = {}
    ostate.vtree = etree[entity].values
    if (ostate.vtree) then
      ostate.valuelist = {}
      for value in pairs(ostate.vtree) do
        ostate.valuelist[1 + #ostate.valuelist] = value
      end
      table.sort(ostate.valuelist)
      collect_descisiontree_values(ostate,0,#ostate.valuelist)
    end
    list[1 + #list] = { ent = entity }
    for idx in pairs(ostate.idxset) do
      list[idx].ParentOperation = #list
    end
  end
end

-- -----------------------------------------------------------------------
-- Generate fields for the derivative list of a container or interface
-- -----------------------------------------------------------------------
local function write_c_derivative_descriptor(output,basename,node)
  local derivmap = node.derivative_decisiontree_map
  local descriptor_fields = {}
  local derivset = {}   -- unordered set of all derivatives (keys=node, value=position in other lists)
  local derivlist = {}  -- ordered list of all derivatives as output in C tables
  local identseq = {}   -- ordered list of operations for base type identification
  local entitymap = {}
  local entitylist = {}
  local valuemap = {}
  local valuelist = {}

  if (node.max_size and node.max_size.bits > node.resolved_size.bits) then
    maxbits = node.max_size.bits
    bufobj = "_Buffer_t"
  else
    maxbits = node.resolved_size.bits
    bufobj = "_t"
  end

  descriptor_fields["MaxSize"] = string.format("{ .Bits = %d, .Bytes = sizeof(%s) }",
    maxbits, basename .. bufobj)

  -- Collect the "Identification Sequence" list which is a state machine
  -- indicating operations to perform on the base type in order to identify a
  -- derived type.  It is based on the Constraint tags in EDS.
  if (derivmap) then
    -- start with the "unconstrained" derivatives -- these are
    -- derivative types from EDS that have no specific constraints.
    -- It is up to the application to identify these types, but the DB
    -- can still enumerate what the options are.
    for _,deriv in ipairs(derivmap.derivatives or {}) do
      derivset[deriv] = 0
    end

    collect_descisiontree_entities(identseq, derivmap.entities)

    for i,seq in ipairs(identseq) do
      local deriv = seq.result
      local entity = seq.ent
      local value = seq.val
      if (deriv) then
        derivset[deriv] = i
        seq.EntryType = "EDSLIB_IDENT_SEQUENCE_RESULT"
      elseif (entity) then
        seq.ent = nil
        if (not entitymap[entity]) then
          local entidx = 1 + #entitylist
          local ent = {}
          entitymap[entity] = entidx
          entitylist[entidx] = ent

          local entnode = node:find_entity(entity)
          local bit = 0
          local cname
          for _,v in ipairs(entnode) do
            local member = v.node and v.node.name or v.type.name
            if (cname) then
              cname = cname .. "." .. member
            else
              cname = member
            end
            bit = bit + v.offset
          end

          ent.RefObj = entnode[#entnode].type.edslib_refobj_initializer
          ent.Offset = string.format("{ .Bits = %d, .Bytes = offsetof(struct %s,%s) }",
              bit, SEDS.to_safe_identifier(node:get_qualified_name()), cname)

        end

        seq.EntryType = "EDSLIB_IDENT_SEQUENCE_ENTITY_LOCATION"
        seq.RefIdx = string.format("%s /* %s */", entitymap[entity] - 1, entity)

      elseif (value) then
        seq.val = nil
        if (not valuemap[value]) then
          local validx = 1 + #valuelist
          local ent = {}
          valuemap[value] = validx
          valuelist[validx] = ent

          if (type(value) == "number" or type(value) == "boolean") then
            ent["RefValue.Integer"] = value
          else
            -- For strings, include quotes, and escape any embedded quotes
            ent["RefValue.String"] = string.format("\"%s\"", string.gsub(value,"\"", "\\\""))
          end
        end

        seq.EntryType = "EDSLIB_IDENT_SEQUENCE_VALUE_CONDITION"
        seq.RefIdx = string.format("%s /* %s */", valuemap[value] - 1, value)
      end
    end

    descriptor_fields.DerivativeList = string.format("%s_DERIVATIVE_LIST", basename)
    output:write(string.format("static const EdsLib_DerivativeEntry_t %s[] =", descriptor_fields.DerivativeList))
    output:start_group("{")
    local deriviter = SEDS.sorted_keys(derivset, function(a,b)
      return (a.edslib_refobj_initializer < b.edslib_refobj_initializer)
    end)
    for deriv in deriviter do
      output:append_previous(",")
      deriv.edslib_basetype_derivtable_idx = #derivlist -- save in DOM for future scripts
      derivlist[1 + #derivlist] = deriv
      output:write(string.format("{ .IdentSeqIdx =%4d, .RefObj = %40s }",
          derivset[deriv],deriv.edslib_refobj_initializer))
    end
    output:end_group("};")
    output:add_whitespace(1)
    node.edslib_derivtable_list = derivlist -- stash flattened list in DOM for future scripts
    descriptor_fields.DerivativeListSize = #derivlist

    if (#entitylist > 0) then
      descriptor_fields.ConstraintEntityList = string.format("%s_CONSTRAINT_ENTITY_LIST", basename)
      output:write(string.format("static const EdsLib_ConstraintEntity_t %s[] =", descriptor_fields.ConstraintEntityList))
      output:start_group("{")

      for _,ent in ipairs(entitylist) do
        output:append_previous(",")
        output:start_group("{")
        for i in SEDS.sorted_keys(ent) do
          output:append_previous(",")
          output:write(string.format(".%s = %s",i,ent[i]))
        end
        output:end_group("}")
      end
      output:end_group("};")
      output:add_whitespace(1)
      descriptor_fields.ConstraintEntityListSize = #entitylist
    end

    if (#valuelist > 0) then
      descriptor_fields.ValueList = string.format("%s_VALUE_LIST", basename)
      output:write(string.format("static const EdsLib_ValueEntry_t %s[] =", descriptor_fields.ValueList))
      output:start_group("{")

      for _,cond in ipairs(valuelist) do
        output:append_previous(",")
        output:start_group("{")
        for i in SEDS.sorted_keys(cond) do
          output:append_previous(",")
          output:write(string.format(".%s = %s",i,cond[i]))
        end
        output:end_group("}")
      end
      output:end_group("};")
      output:add_whitespace(1)
      descriptor_fields.ValueListSize = #valuelist
    end


    if (#identseq > 0) then
      descriptor_fields.IdentSequenceList = string.format("%s_IDENT_SEQUENCE", basename)
      output:write(string.format("static const EdsLib_IdentSequenceEntry_t %s[] =", descriptor_fields.IdentSequenceList))
      output:start_group("{")
      output:write(string.format("{ .EntryType = %s }","EDSLIB_IDENT_SEQUENCE_INVALID"))
      for i,seq in ipairs(identseq) do
        if (seq.result) then
          seq.RefIdx = string.format("%s /* %s */", seq.result.edslib_basetype_derivtable_idx, seq.result:get_flattened_name())
          seq.result = nil
        end
        output:append_previous(",")
        output:start_group(string.format("[%d] = {",i))
        for i in SEDS.sorted_keys(seq) do
          output:append_previous(",")
          output:write(string.format(".%s = %s",i,seq[i]))
        end
        output:end_group("}")
      end
      output:end_group("};")
      output:add_whitespace(1)
      descriptor_fields.IdentSequenceBase = #identseq
    end
  end

  return descriptor_fields

end

-- -----------------------------------------------------------------------
-- Generate "Detail Object" for a string node
-- -----------------------------------------------------------------------
local function write_c_string_detail_object(output,node)
  local encnode = node:find_first({"STRING_DATA_ENCODING"})
  local conv = {
    ["ascii"] = "EDSLIB_STRINGENCODING_ASCII",
    ["utf-8"] = "EDSLIB_STRINGENCODING_UTF8"
  }
  local strval

  if (encnode) then
    strval = conv[encnode.encoding]
  end

  if (strval) then
    return { ["Detail.String"] = "{ .Encoding = " .. strval .. " }" }
  end
end

-- -----------------------------------------------------------------------
-- Generate "Detail Object" for a boolean node
-- -----------------------------------------------------------------------
local function write_c_boolean_detail_object(output,node)
  local encnode = node:find_first({"BOOLEAN_DATA_ENCODING"})
  if (encnode and encnode.falsevalue == "nonzeroisfalse") then
    return { ["Detail.Number"] = "{ .BitInvertFlag = 1 }" }
  end
end

-- -----------------------------------------------------------------------
-- Generate "Detail Object" for an integer or floating point node
-- -----------------------------------------------------------------------
local function write_c_number_detail_object(output,node)
  local details = {}
  local encnode = node:find_first({"INTEGER_DATA_ENCODING","FLOAT_DATA_ENCODING"})
  local byteconv = {
    ["littleendian"] = "EDSLIB_NUMBERBYTEORDER_LITTLE_ENDIAN",
    ["bigendian"] = "EDSLIB_NUMBERBYTEORDER_BIG_ENDIAN"
  }
  local encconv = {
    ["unsigned"] = "EDSLIB_NUMBERENCODING_UNSIGNED_INTEGER",
    ["signmagnitude"] = "EDSLIB_NUMBERENCODING_SIGN_MAGNITUDE",
    ["onescomplement"] = "EDSLIB_NUMBERENCODING_ONES_COMPLEMENT",
    ["twoscomplement"] = "EDSLIB_NUMBERENCODING_TWOS_COMPLEMENT",
    ["bcd"] = "EDSLIB_NUMBERENCODING_BCD_OCTET",
    ["packedbcd"] = "EDSLIB_NUMBERENCODING_BCD_PACKED",
    ["ieee754_2008_single"] = "EDSLIB_NUMBERENCODING_IEEE_754",
    ["ieee754_2008_double"] = "EDSLIB_NUMBERENCODING_IEEE_754",
    ["ieee754_2008_quad"] = "EDSLIB_NUMBERENCODING_IEEE_754",
    ["milstd_1750a_simple"] = "EDSLIB_NUMBERENCODING_MILSTD_1750A",
    ["milstd_1750a_extended"] = "EDSLIB_NUMBERENCODING_MILSTD_1750A"
  }
  local strval

  if (encnode) then

    details.Encoding = encconv[encnode.encoding or encnode.encodingandprecision]
    details.ByteOrder = byteconv[encnode.byteorder]

  end

  for _,k in ipairs({"Encoding","ByteOrder"}) do
    if (details[k]) then
      strval = string.format("%s.%s = %s", strval and (strval .. ", ") or "", k, details[k])
    end
  end

  if (strval) then
    return { ["Detail.Number"] = "{ " .. strval .. " }" }
  end
end

-- -----------------------------------------------------------------------
-- Generate "Detail Object" for an array node
-- The only extra info needed is the element object type
-- -----------------------------------------------------------------------
local function write_c_array_detail_object(output,node)

  local detail_name = string.format("%s_ARRAY_DETAIL", node:get_flattened_name())
  output:write(string.format("static const EdsLib_ArrayDescriptor_t %s =", detail_name))
  output:start_group("{")
  output:write(string.format(".ElementRefObj = %s", node.datatyperef.edslib_refobj_initializer))
  output:end_group("};")
  output:add_whitespace(1)

  return { ["Detail.Array"] = "&" .. detail_name, NumSubElements = node.total_elements }

end

local function write_c_container_detail_object(output,node)

  local objname
  local detailfields
  local maxbits, bufobj
  local basename = node:get_flattened_name()

  detailfields = do_get_fields(write_c_derivative_descriptor,
    detailfields, output, basename, node)

  detailfields = do_get_fields(write_c_decode_sequence,
    detailfields, output, node)

  local detail_name = string.format("%s_CONTAINER_DETAIL", basename)
  output:write(string.format("static const EdsLib_ContainerDescriptor_t %s =", detail_name))
  output:start_group("{")

  do_write_field_list(output,detailfields,{
    "MaxSize", "EntryList", "TrailerEntryList",
    "DerivativeList", "DerivativeListSize",
    "IdentSequenceList", "IdentSequenceBase",
    "ConstraintEntityList",  "ConstraintEntityListSize",
    "ValueList", "ValueListSize"}
  )

  output:end_group("};")
  output:add_whitespace(1)

  return { ["Detail.Container"] = "&" .. detail_name, NumSubElements = detailfields.EntryListSize }

end


-- -----------------------------------------------------------------------
-- Generate "BasicType" field value for master table
-- -----------------------------------------------------------------------
local function get_object_detail_fields(output,node)
  local retval
  local typemap
  local detailfunc

  if (node.resolved_size) then
    if (node.resolved_size.bits > 0) then
      if (node.entity_type == "INTEGER_DATATYPE" or node.entity_type == "ENUMERATION_DATATYPE") then
        typemap = (node.is_signed and "SIGNED" or "UNSIGNED") .. "_INT"
        detailfunc = write_c_number_detail_object
      elseif (node.entity_type == "BOOLEAN_DATATYPE") then
        typemap = "UNSIGNED_INT"
        detailfunc = write_c_boolean_detail_object
      elseif (node.entity_type == "FLOAT_DATATYPE") then
        typemap = "FLOAT"
        detailfunc = write_c_number_detail_object
      elseif (node.entity_type == "STRING_DATATYPE") then
        detailfunc = write_c_string_detail_object
        typemap = "BINARY"
      elseif (node.entity_type == "ARRAY_DATATYPE") then
        detailfunc = write_c_array_detail_object
        typemap = "ARRAY"
      elseif (node.entity_type == "CONTAINER_DATATYPE" or
              node.entity_type == "PROVIDED_INTERFACE" or
              node.entity_type == "REQUIRED_INTERFACE" or
              node.entity_type == "DECLARED_INTERFACE" or
              node.entity_type == "COMPONENT") then
        detailfunc = write_c_container_detail_object
        typemap = "CONTAINER"
      end
    end

    -- Use "BINARY" as a catch-all for any other type, including
    -- containers without any members (zero bit size)
    if (not typemap) then
      typemap = "BINARY"
    end
  end

  retval = do_get_fields(detailfunc,retval,output,node)

  if (typemap) then
    retval["BasicType"] = "EDSLIB_BASICTYPE_" .. typemap
  end

  return retval
end

-- -----------------------------------------------------------------------
-- MAIN ROUTINE BEGINS HERE
-- -----------------------------------------------------------------------

-- handler table: each function will be called on all types
-- each one may return name/value pairs to be added to the generated output
local datatype_output_handlers =
{
  get_basic_size_fields,
  get_object_detail_fields
}

local global_sym_prefix = SEDS.get_define("MISSION_NAME")
local global_file_prefix = global_sym_prefix and string.lower(global_sym_prefix) or "eds"
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"

-- -----------------------------------------------
-- GENERATE datatypedb_impl.c files
-- -----------------------------------------------
-- this has the main data structure mapping EDS objects to C objects
-- each datasheet has its own file
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do

  local datasheet_objs = { { BasicType = "EDSLIB_BASICTYPE_NONE", DisplayHint = "EDSLIB_DISPLAYHINT_NONE" } }
  local refnames = {}

  local output = SEDS.output_open(SEDS.to_filename("datatypedb_impl.c", ds.name), ds.xml_filename)

  -- references to other objects are based on the master index generated earlier
  output:write(string.format("#include \"edslib_database_types.h\""))
  output:write(string.format("#include \"%s\"", SEDS.to_filename("master_index.h")))
  output:write(string.format("#include \"%s\"", SEDS.to_filename("typedefs.h", ds.name)))
  output.checksum_table = {}
  output.datasheet_name = ds:get_flattened_name()

  output:section_marker("Data Type Objects")
  for node in ds:iterate_subtree() do
    if (node.edslib_refobj_local_index and node.header_data) then
      datasheet_objs[1 + #datasheet_objs] = do_get_fields(datatype_output_handlers,nil,output,node)
      refnames[#datasheet_objs] = node:get_qualified_name()
    end
  end

  local ds_name = SEDS.to_macro_name(ds.name)

  output:section_marker("Lookup Table")
  output:write(string.format("static const EdsLib_DataTypeDB_Entry_t %s_DATADICTIONARY_TABLE[] =", ds_name))
  output:start_group("{")
  for idx,dsobj in ipairs(datasheet_objs) do
    output:append_previous(",")
    output:start_group(string.format("{ /* %s */", refnames[idx] or "(none)"))
    for _,key in ipairs({ "Checksum", "BasicType", "Flags", "NumSubElements", "SizeInfo", "Detail.Array", "Detail.Container", "Detail.Number", "Detail.String" }) do
      if (dsobj[key]) then
        output:append_previous(",")
        output:write(string.format(".%s = %s", key, dsobj[key]))
      end
    end
    output:end_group("}")
  end
  output:end_group("};")

  output:section_marker("Database Object")


  output:write(string.format("const struct EdsLib_App_DataTypeDB %s_DATATYPE_DB =", ds_name))
  output:start_group("{")
  output:write(string.format(".MissionIdx = %s_INDEX_%s,",global_sym_prefix, ds_name));
  output:write(string.format(".DataTypeTableSize = %d,",#datasheet_objs));
  output:write(string.format(".DataTypeTable = %s_DATADICTIONARY_TABLE", ds_name));
  output:end_group("};")

  -- Close the output files
  SEDS.output_close(output)

end

-- -----------------------------------------------
-- GENERATE GLOBAL / MASTER SOURCE FILE
-- -----------------------------------------------
-- This array object has a pointer reference to _all_ generated datasheets
-- It can be linked into tools to easily pull in all database objects
local output = SEDS.output_open(SEDS.to_filename("datatypedb_impl.c"))

output:write(string.format("#include \"edslib_database_types.h\""))
output:write(string.format("#include \"%s\"",SEDS.to_filename("master_index.h")))
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  output:write(string.format("#include \"%s\"",SEDS.to_filename("dictionary.h", ds.name)))
end

output:add_whitespace(1)

output:write(string.format("const EdsLib_DataTypeDB_t %s_DATATYPEDB_APPTBL[%s_MAX_INDEX] =", global_sym_prefix, global_sym_prefix))
output:start_group("{")
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  local ds_name = SEDS.to_macro_name(ds.name)
  output:append_previous(",");
  output:write(string.format("[%s_INDEX_%s] = &%s_DATATYPE_DB",global_sym_prefix,ds_name,ds_name))
end
output:end_group("};")

SEDS.output_close(output)

SEDS.info ("SEDS write basic objects END")
