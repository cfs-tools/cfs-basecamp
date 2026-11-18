# Generic pattern rules for building EDS database objects

#
#  ******************************************************************************
#  **               Pattern rule to build a single database file               **
#  ******************************************************************************
#

include $(EDS_REPO_SOURCE_DIR)/edslib/cmake/base_patternrules.mk

#
#  ******************************************************************************
#  **            Pattern rule to build a dynamic shared object file            **
#  ******************************************************************************
#

# This is a bit of a hack -
# Embedded targets like VxWorks and RTEMS use relocatable objects (i.e. .obj files) as
# loadable modules.  These are not shared objects (.so) in the Linux/POSIX sense, and
# need a different linker flag(s) to build them.  In lieu of somehow extracting the full
# linker logic that CMake uses, this just assumes a GNU-style LD, and that the file extension
# (.so or .obj) indicates the style of loadable object being used.

$(O)/$(EDS_FILE_PREFIX)_sb_dispatchdb.so: EXTRA_LINK_LIBS=-l:$(EDS_FILE_PREFIX)_db.so

$(O)/%.so:
	@echo EDS: Linking shared object $(@) for $(EDSTOOL_ARCH)
	$(LD) $(SHARED_LDFLAGS) -shared -o $@ $^ -L$(O) $(EXTRA_LINK_LIBS)

$(O)/%.obj:
	@echo EDS: Linking relocatable object $(@) for $(EDSTOOL_ARCH)
	$(LD) $(SHARED_LDFLAGS) -r -o $@ $^
