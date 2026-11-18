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
-- Lua script to build the generated sb_dispatchdb C objects
-- -------------------------------------------------------------------------

local objdir = SEDS.get_define("OBJDIR") or "obj"
local srcdir = SEDS.get_define("SRCDIR") or "src"
local libname = SEDS.to_filename("sb_dispatchdb")
local makefilename = SEDS.to_filename("sb_dispatchdb_objects.mk")
local libtypes = { ".a", ".so", ".obj" }

local function get_objnames(n,prefix,suffix)
  local result = ""
  for _,t in ipairs(suffix) do
    result = result .. string.format("%s/%s%s ",prefix,n,t)
  end
  return result
end

-- ------------------------------------------------
-- Generate a makefile for the sb_dispatchdb library
-- ------------------------------------------------
local output = SEDS.output_open(makefilename)
output:write("include edstool-namespace-$(EDSTOOL_PROJECT_NAME).mk")
output:write("include $(O)/edstool-buildenv.mk $(wildcard $(O)/*.d)")
output:write("include $(EDS_REPO_SOURCE_DIR)/cfecfs/missionlib/cmake/dbobj_patternrules.mk")
output:add_whitespace(1)
output:write("# Interface DB Object")
output:write(string.format("%s: $(O)/%s $(O)/%s",
  get_objnames(libname,"$(O)",libtypes),
  SEDS.to_filename("sb_topicdb_impl.o"),
  SEDS.to_filename("sb_global_impl.o")))
output:add_whitespace(1)
SEDS.output_close(output)

-- ------------------------------------------------
-- Execute the build tool (make) to actually build the
-- interfacdb using the makefile generated above.
-- ------------------------------------------------
SEDS.execute_tool("MAKE_PROGRAM",
  string.format("-C \"%s\" -f \"%s\" O=\"%s\" S=\"%s\" %s",
    SEDS.get_define("EDSTOOL_OUTPUT_DIR") or ".",
    makefilename,
    objdir,
    srcdir,
    get_objnames(libname,objdir,libtypes)
  )
)
