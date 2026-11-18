###########################################
#
# Helper script for building COSMOS gem file
#
# This copies all the test methods supplied directly with apps
# into the staging area for inclusion in the gem file
#


# use the generated file with the dependencies from selected apps
include $(O)/cfs_app_tests.mk

# The "gemfiles" target is the entry point, called from the parent script
.PHONY: gemfiles
gemfiles: $(ALL_CFS_APP_TESTS)

# For now, installing the script is a simple copy (this could be something else)
$(ALL_CFS_APP_TESTS):
	cp $(^) $(@)
