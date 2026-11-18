# Rules for EDS-based CFE table generation

C_INCLUDE_DIRS += $(MISSION_BINARY_DIR)/inc

LOCAL_CFLAGS += $(addprefix -D,$(C_COMPILE_DEFS))
LOCAL_CFLAGS += $(addprefix -I,$(C_INCLUDE_DIRS))
LOCAL_CFLAGS += -DCFE_TABLE_NAME="$(CFE_TABLE_BASENAME)"

AWK ?= awk
M4  ?= m4

DEPENDENCY_FLAGS = -MMD -MF "$(subst /,_,$(@)).d" -MT "$(@)"
SRC = $(shell cat eds/$(*).source)
OBJNAME = $(shell cat eds/$(*).objname)
TYPENAME = $(shell cat eds/$(*).typename)

TBLDEF_C = eds/$(*).tbldef.c
OBJDEF_C = eds/$(*).objdef.c
FILEDEF_C = eds/$(*).filedef.c

.PRECIOUS: eds/%.c eds/%.source eds/%.tbldef.c eds/%.objname eds/%.typename

# The "source" just contains the complete path to the C table file
eds/%.source:
	@mkdir -pv $(dir $(@))
	echo "$(<)" > "$(@)"

# The "tbldef.c" artifact contains only the preprocessed content of the table definition C file
# the key is to pipe it through the C preprocessor so it handles all #include and #ifdef properly,
# then strip out all content from files other than the directly-supplied file.  Importantly, this
# will resolve any C macros used in that file, including CFE_TBL_FILEDEF, leaving only the real definition
eds/%.tbldef.c: eds/%.source $(TABLETOOL_SCRIPT_DIR)/extract_filedef.awk
	$(CC) -E $(DEPENDENCY_FLAGS) $(CFLAGS) $(LOCAL_CFLAGS) "$(SRC)" |\
	    $(AWK) -v SRC="$(SRC)" -f "$(TABLETOOL_SCRIPT_DIR)/extract_filedef.awk" > "$(@).tmp"
	mv "$(@).tmp" "$(@)"

# The "filedef.c" contains only the C FILEDEF macro/object, not the defintion of that object
# Because it is separating the filedef from the object definition, it also needs to remove "sizeof" refs
eds/%.filedef.c: eds/%.tbldef.c $(TABLETOOL_SCRIPT_DIR)/separate_filedef.awk
	$(AWK) -v WANT_FILEDEF=1 -f "$(TABLETOOL_SCRIPT_DIR)/separate_filedef.awk" "$(<)" |\
	    $(M4) -P -E -D sizeof=0 > "$(@).tmp"
	mv "$(@).tmp" "$(@)"

# The "objdef.c" contains only the C object definition/instantiation (not the FILEDEF macro/object)
eds/%.objdef.c: eds/%.tbldef.c $(TABLETOOL_SCRIPT_DIR)/separate_filedef.awk
	$(AWK) -v WANT_FILEDEF=0 -f "$(TABLETOOL_SCRIPT_DIR)/separate_filedef.awk" "$(<)" > "$(@).tmp"
	mv "$(@).tmp" "$(@)"

filedef_extract.o: $(TABLETOOL_SCRIPT_DIR)/filedef_extract.c
	$(CC) $(DEPENDENCY_FLAGS) $(CFLAGS) $(LOCAL_CFLAGS) -c -o "$(@)" "$(<)"

eds/%.filedef.o: eds/%.filedef.c
	$(CC) $(DEPENDENCY_FLAGS) -include cfe_tbl_filedef.h $(CFLAGS) $(LOCAL_CFLAGS) -c -o "$(@)" "$(<)"

# The "objname" file will contain strictly the C object name used in the table definition
# This is a challenge to get because it only is defined in the FileDef object as a string, so
# we must compile the object in isolation and then run a program which prints the string
eds/%.objname: eds/%.filedef.o  filedef_extract.o
	$(CC) $(DEPENDENCY_FLAGS) $(CFLAGS) $(LOCAL_CFLAGS) -o "$(@).getter" $(^)
	$(@).getter > "$(@)"

# The "typename" file will contain strictly the C data type name used in the table definition
eds/%.typename: eds/%.objname eds/%.objdef.c
	$(AWK) -v OBJNAME="$(OBJNAME)" -f "$(TABLETOOL_SCRIPT_DIR)/extract_typename.awk" "$(OBJDEF_C)" > "$(@).tmp"
	mv "$(@).tmp" "$(@)"

# This generates a .c file which "wraps" the original definition and puts it into a function
# This allows the structure initializers to use constructs that are only resolvable at runtime
# The trick here is grab all the preprocessor directives (particularly #include)
# and keep them all at the start outside of the wrapper function, because many of
# these will not work correctly inside a function, and they are necessary to get the typedefs
eds/%.c: eds/%.source eds/%.tbldef.c eds/%.objname eds/%.typename $(TABLETOOL_SCRIPT_DIR)/table_wrapper.c.m4
	grep -E '^\s*#' "$(SRC)" > "$(@).tmp"
	$(M4) -P -E -D TBLDEF_C="$(TBLDEF_C)" -D OBJNAME="$(OBJNAME)" -D TYPENAME=$(TYPENAME) \
	    $(TABLETOOL_SCRIPT_DIR)/table_wrapper.c.m4 >> "$(@).tmp"
	mv "$(@).tmp" "$(@)"

eds/%.o: eds/%.c $(TABLETOOL_SCRIPT_DIR)/eds_tbltool_filedef.h
	$(CC) -Wall -Werror -fPIC -g $(CFLAGS) $(LOCAL_CFLAGS) -include "$(TABLETOOL_SCRIPT_DIR)/eds_tbltool_filedef.h" -o "$(@)" -c "$(<)"

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $(@) $(<)
