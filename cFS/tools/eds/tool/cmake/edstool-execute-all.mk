#####################################################
# 
# CFE/CFS mission build makefile helper
# This calls the EDS tooling to generate all artifacts
# The process also includes building those artifacts for the host
# 
#####################################################

# This checks the sanity of the calling environment.  If any of these fail
# it indicates a problem in the CMake target that invokes this makefile.
ifeq ($(EDSTOOL_PROJECT_NAME),)
$(error EDSTOOL_PROJECT_NAME not defined)
endif
ifeq ($(O),)
$(error O is not defined)
endif
ifeq ($(S),)
$(error S is not defined)
endif

EDSNAMESPACE := edstool-namespace-${EDSTOOL_PROJECT_NAME}.mk
EDSSOURCES   := edstool-sources-${EDSTOOL_PROJECT_NAME}.mk

LOCAL_STAMPFILE := $(O)/edstool-execute-${EDSTOOL_PROJECT_NAME}.stamp

# Read all context info from files exported from CMake
include $(EDSNAMESPACE)
include $(EDSSOURCES)
include $(O)/edstool-buildenv.mk 

# Sanity check: all of these vars should have been part of the context info
ifeq ($(EDSTOOL_OUTPUT_DIR),)
$(error EDSTOOL_OUTPUT_DIR not defined)
endif
ifeq ($(EDS_FILE_PREFIX),)
$(error EDS_FILE_PREFIX not defined)
endif
ifeq ($(EDS_SYMBOL_PREFIX),)
$(error EDS_SYMBOL_PREFIX not defined)
endif

# NOTE: At the mission scope, this runs the full EDS tool to generate all artifacts
# As part of this, it invokes the db_objects make scripts as well, so there is noting
# extra that needs to be invoked from here.

.PHONY: all
all: $(LOCAL_STAMPFILE)
	@echo All host side global EDS objects built

# The file lists here came from CMake, and so they have the semicolon separator,
# which needs to be changed to a space separator for interpretation here.
ALL_EDS_FILES += $(subst ;, ,$(EDSTOOL_FILELIST))
# The scripts are prefixed with a number that indiciate its execution order
# sorting it here just before using it makes it easier to assemble the lists piecemeal
ALL_EDS_FILES += $(sort $(subst ;, ,$(EDSTOOL_SCRIPTLIST)))

$(LOCAL_STAMPFILE): $(EDSTOOL) $(EDSNAMESPACE) $(EDSBUILDENV) $(EDSSOURCES) $(ALL_EDS_FILES)
	+$(EDSTOOL) -DMAKE_PROGRAM="$(MAKE)" -DOBJDIR="$(O)" -DSRCDIR="$(S)" $(ALL_EDS_FILES)
	$(CMAKE) -E touch "$(@)"
