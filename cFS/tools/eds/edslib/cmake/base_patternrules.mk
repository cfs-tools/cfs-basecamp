# Generic pattern rules for building EDS database objects

#
#  ******************************************************************************
#  **               Pattern rule to build a single database file               **
#  ******************************************************************************
#

EDSTOOL_ARCH ?= host

# NOTE: using "abspath" here makes any error reports clickable in IDEs like VSCode
$(O)/%_impl.o: $(S)/%_impl.c
	@echo EDS: Compiling $(<) for $(EDSTOOL_ARCH)
	$(CC) $(CFLAGS) -Iinc -D_EDSLIB_BUILD_ -MMD -c -o $@ $(abspath $<)


#
#  ******************************************************************************
#  **                Pattern rule to build a static archive file               **
#  ******************************************************************************
#

$(O)/%.a:
	@echo EDS: Archiving $(@) for $(EDSTOOL_ARCH)
	-rm -f $(@)
	$(AR) cr $@ $^
