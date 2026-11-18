
# If debug mode is set, then create symlinks instead of copies
ifneq ($(TESTRUNNER_INSTALL_SYMLINK),)
%.py:
	ln -s $(^) $(@)
else
%.py:
	cp $(^) $(@)
endif

include $(O)/cfs_app_tests.mk

__init__.py: $(ALL_CFS_APP_TESTS)
	echo "__all__ = [ $(foreach T,$(notdir $(basename $(^))),'$(T)',) ]" > __init__.py
