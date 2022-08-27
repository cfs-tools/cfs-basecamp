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
-- Lua script to build the generated interfacedb C objects
-- -------------------------------------------------------------------------

local objdir = SEDS.get_define("OBJDIR") or "obj"
local libname = SEDS.to_filename("interfacedb")
local makefilename = SEDS.to_filename("interfacedb_objects.mk")
local libtypes = { ".a", ".so", ".obj" }

local function get_objnames(n,prefix,suffix)
  local result = ""
  for _,t in ipairs(suffix) do
    result = result .. string.format("%s/%s%s ",prefix,n,t)
  end
  return result
end

-- ------------------------------------------------
-- Generate a makefile for the interfacedb library
-- This is currently only one file, the "interfacedb_impl"
-- ------------------------------------------------
local output = SEDS.output_open(makefilename)
output:write(string.format("include %s $(wildcard $(O)/*.d)", SEDS.to_filename("patternrules.mk")))
output:add_whitespace(1)
output:write("# Interface DB Object")
output:write(string.format("%s: $(O)/%s",
  get_objnames(libname,"$(O)",libtypes),
  SEDS.to_filename("interfacedb_impl.o")))
output:add_whitespace(1)
SEDS.output_close(output)

-- ------------------------------------------------
-- Execute the build tool (make) to actually build the
-- interfacdb using the makefile generated above.
-- ------------------------------------------------
SEDS.execute_tool("BUILD_TOOL",
  string.format("-j1 -C %s -f %s O=\"%s\" CC=\"%s\" LD=\"%s\" AR=\"%s\" CFLAGS=\"%s\" LDFLAGS=\"%s\" %s",
    SEDS.get_define("MISSION_BINARY_DIR") or ".",
    makefilename,
    objdir,
    SEDS.get_define("CC") or "cc",
    SEDS.get_define("LD") or "ld",
    SEDS.get_define("AR") or "ar",
    SEDS.get_define("CFLAGS") or "-Wall -Werror -std=c99 -pedantic",
    SEDS.get_define("LDFLAGS") or "",
    get_objnames(libname,objdir,libtypes)
  )
)

