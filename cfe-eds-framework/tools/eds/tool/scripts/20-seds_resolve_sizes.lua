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
-- Lua implementation of "resolve sizes" processing step
--
-- This computes the final values for all remaining attributes.  Most
-- importantly the exact size and binary layout of all data types is
-- determined.   While doing this, a checksum for every binary format
-- is computed, which assists in spotting any significant change.
--
-- The size of objects is computed according to the rules prescribed
-- in CCSDS book 876.0
-- -------------------------------------------------------------------------
SEDS.info ("SEDS resolve sizes START")

local checksum_table = {}
local resolve_error = false
local resolve_table = {}
local derived_node_table = {}

-- -----------------------------------------------------------------------------------------
--                              Internal helper functions
-- -----------------------------------------------------------------------------------------

-- -------------------------------------------------------------------------
-- Helper function to determine the number of encoded storage bits required to
-- represent a given integer range.  Returns the total number of bits required
-- as the first return value.  The second return value is a boolean indicating
-- whether the range has any negative values, which would necessitate an encoding
-- type that supports signed values.
local function get_integer_bits_from_range(range, enctype)
  local min
  local max
  local positivedigits
  local negativedigits
  local maxdigits
  local bitsperdigit

  if (range.max and range.max.value) then
    max = range.max.value
    if (not range.max.inclusive) then
      max = max - 1
    end
  end

  if (range.min and range.min.value) then
    min = range.min.value
    if (not range.min.inclusive) then
      min = min + 1
    end
  else
    min = 0
  end

  if (not max or min > max) then
    return
  end

  signed = false
  if (enctype == "packedbcd" or enctype == "bcd") then
    -- determine number of digits in positive range
    if (max > 0) then
      local e = math.log10(max)
      -- currently no sign indicator for positive values
      positivedigits = math.ceil(math.abs(e))
    else
      positivedigits = 0
    end

    -- determine number of digits in negative range
    if (min < 0) then
      local e = math.log10(-min)
      -- add 1 for the "minus" indicator
      negativedigits = 1 + math.ceil(math.abs(e))
    else
      negativedigits = 0
    end

    if (enctype == "packedbcd") then
      bitsperdigit = 4
    else
      bitsperdigit = 8
    end

  else
    bitsperdigit = 1

    -- determine number of digits in positive range
    if (max > 0) then
      local v,e = math.frexp(max)
      positivedigits = (e > 1) and e or 1
      -- Because numbers are internally "double" values,
      -- anything above ~52 bits may result in an extra
      -- digit due to rounding up.  This adds a "slop factor"
      -- to avoid triggering false errors (the cost is that
      -- this might not flag a borderline case that is an error)
      if (positivedigits > 52 and v < 0.55) then
        positivedigits = positivedigits - 1
      end
    else
      positivedigits = 0
    end

    -- determine number of digits in negative range
    if (min < 0) then
      local v,e = math.frexp(-min)
      negativedigits = (e > 1) and (e+1) or 2
      -- same as positive case
      if (negativedigits > 52 and math.abs(v) < 0.55) then
        negativedigits = negativedigits - 1
      elseif (v == 0.5 and (not enctype or enctype == "twoscomplement")) then
        -- special handling for twos complement, has one extra
        -- value of range on the negative side
        negativedigits = negativedigits - 1
      end

      -- In base 2 a sign bit is always needed
      -- account for extra bit on the positive side, too
      positivedigits = 1 + positivedigits
    else
      negativedigits = 0
    end

  end

  -- the size of the field will be the larger of either
  -- the positive required digits or negative required digits
  if (positivedigits > negativedigits) then
    maxdigits = positivedigits
  else
    maxdigits = negativedigits
  end

  return (maxdigits * bitsperdigit), (negativedigits > 0)
end

-- -------------------------------------------------------------------------
-- Helper function to determine the storage size of integer data types
-- This is shared between integers and enumerations.  If a specific encoding
-- is identified then the number of bits is based on the encoding.  If a range
-- is specified then the number of bits is extrapolated from the range.  If
-- both are specified, then it verifies that the range fully is within the representable
-- range of the encoding.
local function get_integer_size(encnode,rangenode)
  local sizebits
  local signed
  local maxrange
  local actualencoding

  if (encnode) then
    sizebits = encnode.sizeinbits
    signed = (encnode.encoding ~= "unsigned")
    actualencoding = encnode.encoding
  end

  if (rangenode and rangenode.resolved_range) then
    maxrange = rangenode.resolved_range
    local rsize,rsigned = get_integer_bits_from_range(rangenode.resolved_range, actualencoding)
    if (rsize) then
      if (not sizebits) then
        sizebits = rsize
        signed = rsigned
      elseif(sizebits < rsize or (rsigned and not signed)) then
        return rangenode:error(string.format("range requires %d bits (%s)",rsize, rsigned and "signed" or "unsigned"))
      end
    end
  end

  -- Size is undefined
  if (not sizebits) then
    return
  end

  local sizecalc = SEDS.new_size_object(sizebits)
  local packstyle

  sizecalc:flavor(signed)
  if (encnode) then
    if ((signed and encnode.encoding == "twoscomplement") or
        (not signed and encnode.encoding == "unsigned")) then
      if (encnode.byteorder == "bigendian") then
        packstyle = "BE"
      elseif (encnode.byteorder == "littleendian") then
        packstyle = "LE"
      end
    end
  end
  sizecalc:setpack(packstyle or "OTHER")

  if (not maxrange and actualencoding) then
    local limit
    local base
    local digits
    local signdigits = signed and 1 or 0

    if (actualencoding == "bcd") then
      base = 10
      digits = sizebits/8
    elseif (actualencoding == "packedbcd") then
      base = 10
      digits = sizebits/4
    else
      -- all other supported encodings are based on powers of 2
      base = 2
      digits = sizebits
    end

    -- the sign consumes space for one digit, which must be subtracted
    limit = math.pow(base,digits - (signed and 1 or 0))

    if (limit > base) then
      if (signed) then
        maxrange = {
          min = { value = -limit, inclusive = false },
          max = { value = limit, inclusive = false }
        }

        -- For twos complement the minimum is inclusive.
        -- All others allow both +0 and -0 to be represented
        -- and as a result the minimum is exclusive.
        if (actualencoding == "twoscomplement") then
          maxrange.min.inclusive = true
        end

      else
        -- Actual Range calculation for an unsigned is fairly
        -- straightforward and there is unlikely to be anything different
        maxrange = {
          min = { value = 0, inclusive = true },
          max = { value = limit, inclusive = false }
        }
      end
    end
  end

  return sizecalc, signed, maxrange
end



-- -----------------------------------------------------------------------------------------
--                              Data type Resolver functions
-- -----------------------------------------------------------------------------------------

-- -------------------------------------------------------------------------
-- Container data type resolution helper
-- -------------------------------------------------------------------------
local function resolve_container_datatype_size(node)

  local final_size = SEDS.new_size_object()
  local offsets = { }
  local missing_size

  for idx,ntype,refnode in node:iterate_members() do
    if (not ntype) then
      if (refnode and refnode.entity_type == "CONTAINER_PADDING_ENTRY") then
        final_size:pad(refnode.attributes.sizeinbits)
      end
    elseif (final_size) then

      local member_size

      if (refnode and ntype.has_derivatives) then
        -- containment relationship: use maximum (buffer) size
        member_size = ntype.max_size
      else
        -- basetype relationship, or simple containment: use basic resolved size
        member_size = ntype.resolved_size
      end

      if (not member_size) then
        missing_size = ntype or refnode
        final_size = nil
        break
      end

      local byte_offset,bit_offset = final_size:add(member_size)
      -- Skip any zero byte members entirely in the decode sequence
      -- (these are inconsequential for interpreting the binary data, and
      -- generated C code can trip up on these e.g. zero byte structures)
      if (member_size.bits > 0) then
        offsets[1 + #offsets] = {
          entry = refnode,
          name = refnode and refnode.name,
          type = ntype,
          byte = byte_offset,
          bit = bit_offset
        }
      end
      if (refnode) then
        final_size:flavor(refnode.name)
      end
    end
  end

  if (missing_size) then
    return missing_size
  end

  -- Do additional pass through entry list to determine:
  --  1. whether or not we have any non-simple entries (error control, length, list, etc)
  --  2. what the possible range of each entry is
  local special_fields = false
  for idx,off in ipairs(offsets) do
    local refnode = off.entry
    local vrange
    local resolved_range
    if (refnode) then
      special_fields = special_fields or refnode.entity_type ~= "CONTAINER_ENTRY"
      vrange = refnode:find_first("VALID_RANGE")
      resolved_range = vrange and vrange.resolved_range
    end
    if (not resolved_range and off.type) then
      resolved_range = off.type.resolved_range
      if (not resolved_range) then
        vrange = off.type:find_first("RANGE")
        resolved_range = vrange and vrange.resolved_range
      end
    end
    off.resolved_range = resolved_range
  end

  if (special_fields) then
    -- Special field(s) present - this forces the is_packed property to be false
    final_size:setpack("OTHER")
  end

  node.decode_sequence = offsets
  node.resolved_size = final_size

  return true
end

-- -------------------------------------------------------------------------
-- Array data type size resolver
-- -------------------------------------------------------------------------
local function resolve_array_datatype_size(node)

  if (not node.datatyperef) then
    return node:error("Array missing type reference")
  end

  local member_size

  if (node.datatyperef.has_derivatives) then
    -- containment relationsip: use maximum (buffer) size
    member_size = node.datatyperef.max_size
  else
    -- basetype relationship, or simple containment: use basic resolved size
    member_size = node.datatyperef.resolved_size
  end

  if (not member_size) then
    return node.datatyperef
  end

  local total_size = node.total_elements
  if (not total_size) then
    for dim in node:iterate_subtree("DIMENSION") do
      local dim_size = dim.size
      if (not dim_size) then
        if (not dim.indextyperef) then
          dim:error("Dimension requires either indextyperef or size")
        else
          -- The type used must be integer in nature with well defined limits
          -- and no negative values, which are invalid for array indices
          local dim_range = dim.indextyperef.resolved_range
          if (dim_range and dim_range.max) then
            -- Note this uses only the "max" value....
            -- it does not matter how many actual labels are defined, only the value of the highest one
            dim_size = dim_range.max.value or 0
            if (dim_range.max.inclusive) then
              dim_size = dim_size + 1
            end
          end
          if (type(dim_size) ~= "number" or dim_size <= 0) then
            return string.format("%s is not usable as an array index type", dim.indextyperef)
          end
        end
      end
      if (not dim_size) then
        dim:error("no index range identified")
        total_size = 0
      else
        dim.total_elements = dim_size
        if (not total_size) then
          total_size = dim_size
        else
          total_size = total_size * dim_size
        end
      end
    end

    node.total_elements = total_size
  end

  if (not total_size or total_size == 0) then
    return node:error("array dimension missing or invalid")
  end

  local final_size = SEDS.new_size_object(member_size)
  final_size:multiply(total_size)
  node.resolved_size = final_size

  return true
end

-- -------------------------------------------------------------------------
-- Enumerated data type size resolver
-- -------------------------------------------------------------------------
local function resolve_enumeration_datatype_size(node)
  local sizecalc, signed = get_integer_size(node:find_first("INTEGER_DATA_ENCODING"), node)
  if (not sizecalc) then
    return
  end

  sizecalc:flavor(node.name)
  for label in node:iterate_subtree("ENUMERATION_ENTRY") do
    sizecalc:flavor(label.name)
    sizecalc:flavor(label.value)
  end

  node.resolved_size = sizecalc
  node.is_signed = signed
  return true
end

-- -------------------------------------------------------------------------
-- String data type size resolver
-- -------------------------------------------------------------------------
local function resolve_string_datatype_size(node)
  if (not node.length or node.length == 0) then
    return node:error("string length invalid")
  end
  node.resolved_size = SEDS.new_size_object(8 * node.length,0)
  return true
end

-- -------------------------------------------------------------------------
-- Integer data type size resolver
-- -------------------------------------------------------------------------
local function resolve_integer_datatype_size(node)
  local encnode = node:find_first("INTEGER_DATA_ENCODING")
  local rangenode = node:find_first("RANGE")
  local sizecalc, signed, maxrange = get_integer_size(encnode, rangenode)

  -- For integers allow entities with unknown sizes to pass thru
  -- This is only an error if they are actually _used_ somewhere where a
  -- size needs to be known, such as a container entry.
  -- Otherwise it can be ignored.
  node.resolved_size = sizecalc
  node.is_signed = signed
  node.resolved_range = maxrange

  return true
end

-- -------------------------------------------------------------------------
-- Float data type size resolver
-- -------------------------------------------------------------------------
local function resolve_float_datatype_size(node)
  local encbits
  local precisionbits
  local packstyle
  local assume_native
  local FLOAT_ENCODING_BITWIDTH = {
    ["ieee754_2008_single"] = 32,
    ["milstd_1750a_simple"] = 32,
    ["milstd_1750a_extended"] = 48,
    ["ieee754_2008_double"] = 64,
    ["ieee754_2008_quad"] = 128
  }
  local FLOAT_NATIVE_ENCODINGS = {
    ["ieee754_2008_single"] = true,
    ["ieee754_2008_double"] = true
  }


  local encnode = node:find_first("FLOAT_DATA_ENCODING")
  if (encnode) then
    encbits = FLOAT_ENCODING_BITWIDTH[encnode.encodingandprecision]
    assume_native = FLOAT_NATIVE_ENCODINGS[encnode.encodingandprecision]
  end

  --[[
   If encoding was NOT specified, look for a precision range as a substitute, which
   also implies the number of bits.
  --]]
  local rangenode = node:find_first("RANGE")
  if (rangenode) then
    rangenode = rangenode:find_first("PRECISION_RANGE")
    if (rangenode and rangenode.xml_cdata) then
      local precision = string.lower(tostring(rangenode.xml_cdata))
      if (precision == "single") then
        precisionbits = 32
      elseif (precision == "double") then
        precisionbits = 64
      elseif (precision == "quad") then
        precisionbits = 128
      end
    end
  end

  if (not encbits) then
    encbits = precisionbits
    assume_native = true
  elseif (precisionbits and encbits > precisionbits) then
    return node:error(string.format("mismatched precision=%d/encoding=%d",precisionbits,encbits))
  end

  if (not encbits) then
    return node:error("missing precision/encoding")
  end

  if (assume_native and encnode) then
    if (encnode.byteorder == "bigendian") then
      packstyle = "BE"
    elseif (encnode.byteorder == "littleendian") then
      packstyle = "LE"
    end
  end

  node.resolved_size = SEDS.new_size_object(encbits)
  node.resolved_size:setpack(packstyle or "OTHER")
  return true
end

-- -------------------------------------------------------------------------
-- Binary data type size resolver
-- -------------------------------------------------------------------------
local function resolve_binary_datatype_size(node)
  node.resolved_size = SEDS.new_size_object(node.sizeinbits,0)
  return true
end

-- -------------------------------------------------------------------------
-- Boolean data type size resolver
-- -------------------------------------------------------------------------
local function resolve_boolean_datatype_size(node)
  node.resolved_size = SEDS.new_size_object(1) -- tbd if anything >1 bit is ever possible for boolean?
  return true
end

-- -------------------------------------------------------------------------
-- Subrange data type size resolver
-- -------------------------------------------------------------------------
-- Note that this is always the same size as the base type, it just further limits the range
local function resolve_subrange_datatype_size(node)
  if (not node.basetype) then
    return node:error("base type missing")
  end

  if (not node.basetype.resolved_size) then
    return node.basetype
  end

  node.resolved_size = SEDS.new_size_object(node.basetype.resolved_size)
  return true
end


-- -----------------------------------------------------------------------------------------
--                              Main object size resolution routine
-- -----------------------------------------------------------------------------------------

local SEDS_SIZE_RESOLVE_TABLE =
{
  CONTAINER_DATATYPE = resolve_container_datatype_size,
  ARRAY_DATATYPE = resolve_array_datatype_size,
  ENUMERATION_DATATYPE = resolve_enumeration_datatype_size,
  STRING_DATATYPE = resolve_string_datatype_size,
  INTEGER_DATATYPE = resolve_integer_datatype_size,
  FLOAT_DATATYPE = resolve_float_datatype_size,
  BINARY_DATATYPE = resolve_binary_datatype_size,
  BOOLEAN_DATATYPE = resolve_boolean_datatype_size,
  SUBRANGE_DATATYPE = resolve_subrange_datatype_size,
  PROVIDED_INTERFACE = resolve_container_datatype_size,
  COMPONENT = resolve_container_datatype_size
}

-- Prior to starting the actual resolution process, make an initial pass
-- through the tree to mark those nodes that have derivatives and add a "has_derivatives" field
-- This is useful to simplify later checks.
for node in SEDS.root:iterate_subtree() do
  local deriv_list = node:get_references("derivatives")
  if (deriv_list and #deriv_list > 0) then
    derived_node_table[node] = deriv_list
    node.has_derivatives = true
  end
end

-- ---- Resolution loop ----
--
-- How it works:
-- For any node which contains other nodes (e.g. containers, interfaces, etc) in
-- order to figure out the size of the container, then all contained objects must
-- be resolved first.  Any circular dependency would be an error.

-- To make this work, this loop makes complete passes through the entire tree,
-- and attempts to calculate the size of all data type(s) which do not already have
-- a "resolved_size" attribute.  If the size cannot be calculated because one of the
-- dependent entities is still unknown, it is skipped.
--
-- Two counters are maintained for each pass:
--   unresolved => count of objects which the resolved_size could NOT be computed
--   resolved => count of objects which the resolved_size was successfully computed
--
while (SEDS.get_error_count() == 0) do
  local unresolved_count = 0
  local resolved_count = 0

  -- Make a pass through the tree, grabbing anything that needs resolution
  for node in SEDS.root:iterate_subtree() do
    local resolve = SEDS_SIZE_RESOLVE_TABLE[node.entity_type]
    if(type(resolve) == "function" and not resolve_table[node]) then

      local status = resolve(node)

      -- The resolve function has four possible outcomes:
      -- For nodes which are deemed "OK" this returns boolean true.
      --
      --    a) The item size is successfully determined.
      --       -> The return value is boolean "true"
      --          The "resolved_size" attribute will be populated
      --    b) The item size cannot be determined, but can be safely ignored.
      --        (for instance, if the value is abstract and not actually used in direct form)
      --       -> The return value is boolean "true"
      --          The "resolved_size" attribute will NOT be populated
      --    c) The item size cannot be determined due to a document error or missing info
      --       -> The return value is NOT boolean "true"
      --          Internally calls an appropriate error reporting function
      --    d) The item size cannot be determined because a dependent type size is not known
      --       -> The return value should be the failed dependency node or an error string
      --
      -- Also note case (d) is a transient problem, and requires making multiple passes
      -- through this loop until it goes away.  Cases a,b,c are considered "final"


      if (status == true) then    -- boolean true only, not another "logically true" value
        resolved_count = resolved_count + 1
        resolve_table[node] = true
        if (node.resolved_size) then
          node.resolved_size:flavor(node.entity_type)
          if (not checksum_table[node.resolved_size.checksum]) then
            checksum_table[node.resolved_size.checksum] = node
          end
        end
      else
        unresolved_count = unresolved_count + 1
        if (resolve_error) then
          local message
          if (type(status) == "userdata") then
            message = "failed dependency:" .. tostring(status)
          else
            message = status
          end
          node:error(tostring(node), message)
        end
      end
    end
  end

  -- Make a pass through the "derived" table, to calculate
  -- the _maximum_ size of these objects (which is helpful for sizing buffers)
  for node,deriv_list in pairs(derived_node_table) do
    if (node.resolved_size) then
      local pending_max_size = SEDS.new_size_object(node.resolved_size)
      for i,deriv_node in ipairs(deriv_list) do
        if (derived_node_table[deriv_node]) then
          pending_max_size = nil
          break
        end
        local deriv_size = deriv_node.max_size or deriv_node.resolved_size
        if (not deriv_size) then
          pending_max_size = nil
          break
        end
        pending_max_size:union(deriv_size)
      end
      if (pending_max_size) then
        resolved_count = resolved_count + 1
        node.max_size = pending_max_size
        derived_node_table[node] = nil
      elseif (resolve_error) then
        node:error("Maximum derivative size not resolved")
      else
        unresolved_count = unresolved_count + 1
      end
    end
  end

  -- If the "unresolved" count ends up zero, then everything is satisfied - job done
  -- Otherwise, as long as the "resolved" count is non zero, then this indicates
  -- progress and the next pass may resolve more items.
  -- The other possibilty, a resolved count of zero and a nonzero unresolved count,
  -- constitutes an error.  Since no changes were made to the tree, subsquent passes
  -- will be no different.  The most likely cause of this would be a circular dependency.

  -- In this case, one more pass will be made, this time triggering an error message
  -- on every unresolved size, so the user can investigate the issue.  This final pass
  -- is purely for the purposes of error collection.

  -- This approach avoids the "trap" of trying to recursively follow a circular dependency
  -- which of course would ultimately crash the process.
  if (unresolved_count == 0 or resolve_error) then
    break
  end

  resolve_error = (resolved_count == 0)

end

SEDS.info ("SEDS resolve sizes END")
