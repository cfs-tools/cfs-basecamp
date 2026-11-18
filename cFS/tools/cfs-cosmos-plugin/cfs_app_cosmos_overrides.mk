###########################################
#
# Helper script for building COSMOS gem file
#
# This replaces the original test script with wrappers that use the
# unified test script instead.  This is only done when building with
# "make gem", it will not happen when directly building with rake.
#
# Once an application includes a compatible validation script within its
# own source code, a wrapper around that script should be added here so
# that it will be used in the COSMOS test.
#


UNIFIED_TESTS += cfs_test_groups_for_cfs_lab_apps/cfs_ci_lab.py
UNIFIED_TESTS += cfs_test_groups_for_cfs_lab_apps/cfs_to_lab.py

.PHONY: gemfiles always

gemfiles: $(UNIFIED_TESTS)
always:

# The intent here is to overwrite the file, so force this rule to run
$(UNIFIED_TESTS): always
	cp $(SRC)/wrappers/$(@) $(@)
