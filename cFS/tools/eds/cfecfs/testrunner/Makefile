ifeq ($(O),)
$(error $$(O) must point to build dir)
endif

export O

.PHONY: refresh_tests run_test

run_test: export PYTHONPATH=$(CURDIR):$(O)/exe/lib/python
run_test: export LD_LIBRARY_PATH=$(O)/exe/lib

refresh_tests:
	mkdir -p $(O)/exe/lib/python/cfs_app_tests
	make -C $(O)/exe/lib/python/cfs_app_tests -f $(CURDIR)/cfs_app_test_groups.mk __init__.py

run_test: refresh_tests
	python3 ${SCRIPT}
