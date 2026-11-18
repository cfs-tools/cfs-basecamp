
###########################################
#
# Makefile for building COSMOS gem file
#
# This makefile implements logic to create the gem, allowing for 
# test scripts to be sourced from other submodules in the cFS tree.
# (that is, avoiding the need to put all tests locally in this repo)
#
# Instead of building the gem directly with the "rake build" command,
# simply run "make gem" to use the logic this makefile.  All test scripts
# will be staged under the build directory, and the gem file created from
# there.
#

# The "O" variable should point to the build directory, must be set
# by default assume using the standard build
O ?= $(realpath ../../build-native_std)

ifeq ($(O),)
$(error O must point to build dir)
endif

export O

STAGING_DIR ?= $(O)/cosmos/plugin
APP_TEST_STAGING_SUBDIR := cfs_test_groups_for_apps

# Include everything in the lib dir
PLUGIN_LIB_FILES += $(wildcard lib/*.py)

# All the other plugin files are here
PLUGIN_FILES += targets
PLUGIN_FILES += Rakefile
PLUGIN_FILES += openc3-cosmos-cfs.gemspec
PLUGIN_FILES += LICENSE.txt
PLUGIN_FILES += README.md
PLUGIN_FILES += plugin.txt
PLUGIN_FILES += requirements.txt

# Check if the openc3.sh is in the PATH
OPENC3_COMMAND ?= $(shell which openc3.sh)

# If the script is not available, replace with a no-op
# This allows the files to still be staged to the build dir, deferring the gem build
ifeq ($(OPENC3_COMMAND),)
GEM_BUILD_COMMAND = /bin/true
else
GEM_BUILD_COMMAND = $(OPENC3_COMMAND) cli rake build VERSION=$(PLUGIN_VERSION)
endif

# This trick lets the revision number automatically increment each
# time a .gem file is added
PLUGIN_VERSION ?= $(shell git describe --tags --abbrev=0 --match "v[0-9].[0-9].[0-9]" | tr -d v).g$(shell (ls $(STAGING_DIR)/*.gem ||:) | wc -l)

.PHONY: refresh_lib refresh_gemfiles refresh_apptests gem

gem: refresh_apptests refresh_gemfiles refresh_lib
	(cd $(STAGING_DIR) && $(GEM_BUILD_COMMAND))

refresh_lib:
	mkdir -p $(STAGING_DIR)/lib
	cp -rvu -t $(STAGING_DIR)/lib $(PLUGIN_LIB_FILES)

refresh_gemfiles:
	mkdir -p $(STAGING_DIR)
	cp -rvu -t $(STAGING_DIR) $(PLUGIN_FILES)

refresh_apptests: refresh_gemfiles
	mkdir -p $(STAGING_DIR)/targets/CFS/procedures/$(APP_TEST_STAGING_SUBDIR)
	make SRC=$(CURDIR) -C $(STAGING_DIR)/targets/CFS/procedures/$(APP_TEST_STAGING_SUBDIR) -f $(CURDIR)/cfs_app_cosmos_tests.mk gemfiles
	make SRC=$(CURDIR) -C $(STAGING_DIR)/targets/CFS/procedures -f $(CURDIR)/cfs_app_cosmos_overrides.mk gemfiles

