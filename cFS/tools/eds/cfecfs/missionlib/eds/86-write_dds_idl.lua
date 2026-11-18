local write_idl_field
local write_idl_array_members
local write_idl_container_members

-- -- -------------------------------------------------------------------------
-- -- Helper function: assemble the combined element name
-- -- -------------------------------------------------------------------------
local append_qual_prefix = function(old_prefix, add_prefix)
  local merged_prefix

  if (old_prefix ~= nil) then
    merged_prefix = tostring(old_prefix) .. "_"
  else
    merged_prefix = ""
  end

  if (add_prefix ~= nil) then
    merged_prefix = merged_prefix .. tostring(add_prefix)
  else
    merged_prefix = merged_prefix .. "?"
  end

  return merged_prefix
end

local get_annotation = function(attribs)
  local min = attribs.min
  local max = attribs.max

  if (min == nil and max == nil) then
    -- No bounds, no annotation
    return ""
  elseif (min == nil) then
    return string.format("@max(%.0f)", max)
  elseif (max == nil) then
    return string.format("@min(%.0f)", min)
  else
    return string.format("@range(min=%.0f, max=%.0f)", min, max)
  end
end

local update_field_name = function(type, name)
  -- Some things that are legal in EDS are not according to fastddsgen
  -- Resolve the idiosyncracies here

  if string.lower(type) == string.lower(name) then
    -- Type and field names cannot match, case insensitive
    name = "_" .. name
  elseif string.lower(name) == string.lower("EventType") or
      string.lower(name) == string.lower("BitMask") then
    -- Handle reserved names
    name = "_" .. name
  end

  return name
end

local write_field = function(output, type, name, count)
  local count_str = ""
  if (count > 1) then
    count_str = string.format("[%u]", count)
  end

  output:write(string.format("%s %s%s;", type, update_field_name(type, name), count_str))
end

function byte_align(size_bits, count)
  local total_bits = size_bits * count

  -- Round up to the nearest full byte
  local size_bytes = math.ceil(size_bits / 8)

  local new_count = math.ceil(total_bits / (size_bytes * 8))

  return size_bytes, new_count
end

-- -- -------------------------------------------------------------------------
-- -- Helper function: Write a single line-item to a CMD definition (APPEND_PARAMETER)
-- -- -------------------------------------------------------------------------
write_idl_field = function(output, attribs, count)
  if count == nil then
    count = 1
  end

  -- Ensure that the field is byte-aligned
  assert((attribs.bitsize * count) % 8 == 0)
  -- Convert any arrays of bits into smaller arrays of bytes
  local byte_size, count = byte_align(attribs.bitsize, count)

  -- For unknown reasons, when a struct has a base type defined it shows up
  -- here as "_". Ignore it because we already write the inheritance relationship
  if (attribs.name == "_") then
    return
  end

  if (attribs.descr) then
    output:write(string.format("// %s", attribs.descr))
  end

  -- Write any pre-member annotation
  output:write(get_annotation(attribs))

  if (attribs.ctype == "STRING") then
    -- TODO: Deal with array of strings..
    assert(count == 1);
    write_field(output, "char", attribs.name, byte_size)
  elseif (attribs.ctype == "UINT") then
    if (byte_size == 1) then
      write_field(output, "octet", attribs.name, count)
    elseif (byte_size == 2) then
      write_field(output, "unsigned short", attribs.name, count)
    elseif (byte_size == 4) then
      write_field(output, "unsigned long", attribs.name, count)
    elseif (byte_size == 8) then
      write_field(output, "unsigned long long", attribs.name, count)
    else
      error("Unsupported UINT byte length")
    end
  elseif (attribs.ctype == "INT") then
    if (byte_size == 1) then
      write_field(output, "char", attribs.name, count)
    elseif (byte_size == 2) then
      write_field(output, "short", attribs.name, count)
    elseif (byte_size == 4) then
      write_field(output, "long", attribs.name, count)
    elseif (byte_size == 8) then
      write_field(output, "long long", attribs.name, count)
    else
      error("Unsupported UINT byte length")
    end
  elseif (attribs.ctype == "FLOAT") then
    if (byte_size == 4) then
      write_field(output, "float", attribs.name, count)
    elseif (byte_size == 8) then
      write_field(output, "double", attribs.name, count)
    else
      error("Unsupported FLOAT byte length")
    end
  elseif (attribs.ctype == "BOOL") then
    write_field(output, "boolean", attribs.name, count)
  else
    -- Assume it's a custom container
    write_field(output, attribs.ctype, attribs.name, count)
  end

  output:add_whitespace(1)
end

-- -- -------------------------------------------------------------------------
-- -- Helper function: Invoke the line_writer function for the given entry
-- -- This can be CMD or TLM depending on the value of line_writer
-- -- -------------------------------------------------------------------------
write_idl_lineitem = function(output, line_writer, entry, qual_prefix, descr, count, custom_containers)
  local attribs = {}

  attribs.name = qual_prefix and SEDS.to_safe_identifier(qual_prefix)
  -- Note that "resolved_size" only exists within nodes that the tool has calculated a size
  -- If the size was specified directly in the XML as a "sizeinbits" attribute, then it appears
  -- at the top level.
  if (entry.resolved_size) then
    attribs.bitsize = entry.resolved_size.bits
  elseif (entry.sizeinbits) then
    attribs.bitsize = entry.sizeinbits
  else
    attribs.bitsize = 0
  end

  if (not descr) then
    descr = entry.attributes.shortdescription or "Value"
  end

  -- The description should not have newlines or other big chunks of whitespace in it
  descr = descr:gsub("[\t\n ]+", " ")
  descr = descr:gsub("\"", "\'")
  attribs.descr = descr

  -- if the type is an alias it must be followed to get back to the real type
  local ref_entity = entry
  while (ref_entity.entity_type == "ALIAS_DATATYPE") do 
    ref_entity = ref_entity.type
  end

  if (ref_entity.entity_type == "CONTAINER_PADDING_ENTRY") then
    -- For container padding entries, this can be put into the idl file as a series
    -- of 8 bit integers.  However this needs to come up with a unique name for each one.
    local basename = attribs.name
    local bits_remainder = attribs.bitsize
    attribs.ctype = "UINT"
    attribs.bitsize = 8
    while (true) do
      attribs.name = basename .. "_B" .. bits_remainder
      if (bits_remainder <= attribs.bitsize) then
        break
      end
      line_writer(output, attribs)
      bits_remainder = bits_remainder - attribs.bitsize
    end
    attribs.bitsize = bits_remainder
  elseif (ref_entity.entity_type == "STRING_DATATYPE" or ref_entity.entity_type == "BINARY_DATATYPE") then
    attribs.ctype = "STRING"
  elseif (ref_entity.entity_type == "INTEGER_DATATYPE" or ref_entity.entity_type == "ENUMERATION_DATATYPE") then
    attribs.ctype = (entry.is_signed and "INT") or "UINT"
  elseif (ref_entity.entity_type == "FLOAT_DATATYPE") then
    attribs.ctype = "FLOAT"
  elseif (ref_entity.entity_type == "BOOLEAN_DATATYPE") then
    attribs.ctype = "BOOL"
  elseif (ref_entity.entity_type == "CONTAINER_DATATYPE") then
    -- Write the container name - we'll define it as a separate struct elsewhere
    attribs.ctype = entry.name
  else
    error(string.format("Unknown entity type %s for %s", entry, attribs.name))
  end

  -- Set min/max ranges if appropriate
  if (attribs.ctype == "INT" or attribs.ctype == "UINT" or attribs.ctype == "FLOAT") then
    if (entry.resolved_range) then
      if (entry.resolved_range.min) then
        attribs.min = entry.resolved_range.min.value
        if (not entry.resolved_range.min.inclusive) then
          attribs.min = attribs.min + 1
        end
      end
      if (entry.resolved_range.max) then
        attribs.max = entry.resolved_range.max.value
        if (not entry.resolved_range.max.inclusive) then
          attribs.max = attribs.max - 1
        end
      end
    end
  end

  line_writer(output, attribs, count)
end

-- -- -------------------------------------------------------------------------
-- -- Helper function: Convert the given entry to a block of line items
-- -- This can be CMD or TLM depending on the value of line_writer
-- -- -------------------------------------------------------------------------
write_idl_block = function(output, line_writer, entry, qual_prefix, descr, custom_containers)
  if (entry.entity_type == "ARRAY_DATATYPE") then
    write_idl_array_members(output, line_writer, entry, qual_prefix, descr, custom_containers)
  elseif (entry.name) then
    write_idl_lineitem(output, line_writer, entry, qual_prefix, descr, nil, custom_containers)
  end
end

-- -- -------------------------------------------------------------------------
-- -- Helper function: write a flattened array, adding a digit to the name prefix
-- -- -------------------------------------------------------------------------
write_idl_array_members = function(output, line_writer, arr, qual_prefix, descr, custom_containers)
  local count

  for dim in arr:iterate_subtree("DIMENSION") do
    assert(dim.total_elements ~= nil and dim.total_elements > 0, "Array with bad count")
    count = dim.total_elements
  end

  -- Get the underlying data type
  arr = arr.datatyperef

  write_idl_lineitem(output, line_writer, arr, qual_prefix, descr, count, custom_containers)
end


-- -- -------------------------------------------------------------------------
-- -- Helper function: write a flattened container, adding member name to the prefix
-- -- -------------------------------------------------------------------------
write_idl_container_members = function(output, line_writer, cont, qual_prefix, custom_containers)
  local num_spares = 0
  for _, ntype, refnode in cont:iterate_members() do
    -- By checking refnode this includes only direct entries, not basetypes
    if (ntype) then
      write_idl_block(output, line_writer, ntype, append_qual_prefix(qual_prefix, refnode and refnode.name),
        refnode and refnode.attributes.shortdescription, custom_containers)
    elseif (refnode and refnode.entity_type == "CONTAINER_PADDING_ENTRY") then
      -- Padding entries do not have a type, but need to be put into the COSMOS DB
      -- nonetheless, and they also need a unqique name.
      num_spares = 1 + num_spares
      write_idl_lineitem(output, line_writer, refnode, append_qual_prefix(qual_prefix, "spare" .. num_spares),
        "Spare bits for padding")
    end
  end
end

write_struct = function(output, container, qual_prefix)
  local basetype = container.basetype
  local inherit_string = ""
  if basetype then
    inherit_string = " : " .. basetype.name
  end

  output:write(string.format("struct %s%s", container.name, inherit_string))
  output:start_group("{")
  write_idl_container_members(output, write_idl_field, container, qual_prefix, nil)
  output:end_group("};")
end

local get_namespace = function(entry)
  local full_qual = entry:get_qualified_name()
  local namespace, _ = string.match(full_qual, "([^/]+)/([^/]+)")
  return namespace
end

local add_struct = function(node, field, cmd_code)
  local structs = node.structs
  if structs[field.name] == nil then
    field.cmd_code = cmd_code
    structs[field.name] = field
    node.num_structs = node.num_structs + 1
    node.struct_order[field.name] = node.num_structs
  end
end

local get_underlying_field = function(field)
  while field.entity_type == "ARRAY_DATATYPE" do
    field = field.datatyperef
  end

  return field
end

get_includes = function(name, msg, namespace, file_node, all_files, is_root)
  -- Extract the actual data type from the array
  msg = get_underlying_field(msg)

  if (msg.entity_type == "CONTAINER_DATATYPE") then
    for _, field, _ in msg:iterate_members() do
      if (field ~= nil and field.name ~= "TelemetryHeader" and field.name ~= "CommandHeader") then
        field = get_underlying_field(field)

        if (field.entity_type == "CONTAINER_DATATYPE") then
          local field_namespace = get_namespace(field)
          local new_file_name
          local new_file_node
          if is_root then
            -- This is a direct command or tlm file, write it here
            new_file_name = file_node.file_name
            new_file_node = file_node
          else
            -- Definition is in a different namespace, put it in the appropriate file
            new_file_name = SEDS.to_filename(string.format("%s.idl", field_namespace), "cfe_")
            new_file_node = get_file_node(new_file_name, all_files)

            -- Since this is a different file, it needs to be included from the original
            if file_node.include_file_names[new_file_name] == nil and file_node.file_name ~= new_file_name then
              file_node.include_file_names[new_file_name] = true
            end
          end

          -- Store the struct info within this file
          add_struct(new_file_node, field, nil)

          -- Recursively look to see if the member is itself a container with its own fields
          get_includes(nil, field, namespace, new_file_node, all_files, false)
        end
      end
    end
  end
end

local parse_cmds = function(cmd, module_node, all_files)
  module_node.include_file_names["header.idl"] = true
  for _, cc in ipairs(cmd.cc_list) do
    local argtype = cc.argtype
    add_struct(module_node, argtype, cc.value)
    get_includes(argtype.name, argtype, get_namespace(argtype), module_node, all_files, true)
  end
end

local parse_tlms = function(argtype, module_node, all_files)
  module_node.include_file_names["header.idl"] = true
  add_struct(module_node, argtype, nil)
  get_includes(argtype.name, argtype, get_namespace(argtype), module_node, all_files, true)
end

local copy_headers = function()
  -- Get the script path for relative pathing
  local script_path = debug.getinfo(1, "S").source:sub(2)
  local script_dir = script_path:match("(.*/)") or "./"

  -- Read the source
  local infile = io.open(script_dir .. "../idl/header.idl", "rb")
  local data = infile:read("*a")
  infile:close()

  -- Use io not SEDS.output_open because the latter breaks the file
  local output = io.open("idl/header.idl", "wb")
  output:write(data)
  output:close()
end

local write_module_header = function(output, msg_id, module_name)
  output:add_whitespace(1)
  output:write(string.format("@ccsds_msg(id=%u)", msg_id))
  output:write(string.format("module %s", module_name))
end

function get_file_node(file_name, all_files)
  return get_module_node(file_name, nil, nil, nil, all_files)
end

function get_module_node(file_name, msg_id, module_name, is_cmd, all_files)
  if all_files[file_name] == nil then
    all_files[file_name] = {
      file_name = file_name,
      include_file_names = {},
      structs = {},
      struct_order = {},
      num_structs = 0,
      msg_id = msg_id,
      module_name = module_name,
      is_cmd = is_cmd
    }
  end

  return all_files[file_name]
end

function sort_structs(structs)
  -- Within a file, structs need to be ordered by dependency such that any struct used by another comes first
  local visited = {}
  local sorted = {}

  local function dfs(node)
    if visited[node.name] then
      return true
    end

    visited[node.name] = true

    node = get_underlying_field(node)

    if node.entity_type == "CONTAINER_DATATYPE" then
      for _, member in node:iterate_members() do
        if member ~= nil then
          member = get_underlying_field(member)
        end

        if member ~= nil and member.entity_type == "CONTAINER_DATATYPE" then
          if not dfs(member) then
            return false
          end
        end
      end
    end

    -- Only add structs that were in the original list to add
    if structs[node.name] ~= nil then
      table.insert(sorted, node)
    end
    return true
  end

  for _, field in pairs(structs) do
    if not dfs(field) then
      return nil
    end
  end

  return sorted
end

-- -------------------------
-- MAIN ROUTINE
-- -------------------------

SEDS.output_mkdir("idl")
copy_headers()

local all_files = {}

for _, instance in ipairs(SEDS.highlevel_interfaces) do
  local ds = instance.component:find_parent(SEDS.basenode_filter)

  -- The various interfaces should be attached under required_links
  for _, binding in ipairs(instance.required_links) do
    local reqintf = binding.reqintf
    local cmd = reqintf.intf_commands and reqintf.intf_commands[1]

    -- SB interfaces should only have one "command" of the indication type (i.e. message appears on the bus)
    -- Note "command" here is the EDS terminology, not CFE - the CFE intf is referred to as "telecommand"
    -- The indication has one argument, which is the data associated with the message
    if (cmd and cmd.refnode and cmd.refnode.name == "indication") then
      local sb_params = binding.provinst:find_param_of_type("CFE_SB/SoftwareBusRouting")
      local argtype = cmd.args[1].type

      if (sb_params) then
        local file_name = SEDS.to_filename(reqintf.name .. ".idl", ds.name)
        local msgid = sb_params.PubSub.MsgId -- This is the actual hex msgid value

        if (reqintf.type.name == "Telecommand") then
          if (cmd.cc_list) then
            local module_node = get_module_node(file_name, msgid.Value(), ds.name, true, all_files)
            parse_cmds(cmd, module_node, all_files)
          end
        elseif (reqintf.type.name == "Telemetry") then
          local module_node = get_module_node(file_name, msgid.Value(), ds.name, false, all_files)
          parse_tlms(argtype, module_node, all_files)
        end
      end
    end
  end
end

for file_name, file_data in pairs(all_files) do
  local file_path = "idl/" .. file_name
  local output = SEDS.output_open(file_path)

  for include_file_name, _ in pairs(file_data.include_file_names) do
    output:write("#include \"" .. include_file_name .. "\"")
  end

  output:add_whitespace(1)

  -- This is a cmd or tlm definition with a message ID
  if file_data.module_name ~= nil then
    if file_data.is_cmd then
      write_module_header(output, file_data.msg_id, string.format("%s_cmd", file_data.module_name))
    else
      write_module_header(output, file_data.msg_id, string.format("%s_tlm", file_data.module_name))
    end
    output:start_group("{")
  end

  -- Structs in an IDL file must be ordered such that they are defined before they are referenced
  -- Sort them by that property
  local structs = sort_structs(file_data.structs)
  assert(structs ~= nil, "Failed to sort structs")

  for _, struct in pairs(structs) do
    if struct.cmd_code ~= nil then
      output:write(string.format("@command(fn_code=%u)", struct.cmd_code))
    end
    write_struct(output, struct, nil)
    output:add_whitespace(1)
  end

  if file_data.module_name ~= nil then
    output:end_group("};")
  end

  SEDS.output_close(output)
end
