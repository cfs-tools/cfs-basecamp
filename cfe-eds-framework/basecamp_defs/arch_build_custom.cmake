#
# Example arch_build_custom.cmake
# -------------------------------
#
# This file will be automatically included in the arch-specific build scope
#
# Definitions and options specified here will be used when cross-compiling
# _all_ FSW code for _all_ targets defined in targets.cmake.
#
# Avoid machine-specific code generation options in this file (e.g. -f,-m options); such 
# options should be localized to the toolchain file such that they will only be
# included on the machines where they apply.
#
# CAUTION: In heterogeneous environments where different cross compilers are
# used for different CPUs, particularly if from different vendors, it is likely
# that compile options will need to be different as well.
#
# In general, options in this file can only be used in cases where all CPUs use a
# compiler from the same vendor and/or are all GCC based such that they accept similar
# command line options.
#
# This file can alternatively be named as "arch_build_custom_${TARGETSYSTEM}.cmake"
# where ${TARGETSYSTEM} represents the system type, matching the toolchain.
#
# These example options assume a GCC-style toolchain is used for cross compilation,
# and uses the same warning options that are applied at the mission level.
#
add_compile_options(
    -std=c99                # Target the C99 standard (without gcc extensions)
    -pedantic               # Issue all the warnings demanded by strict ISO C
    -Wall                   # Warn about most questionable operations
    -Wstrict-prototypes     # Warn about missing prototypes
    -Wwrite-strings         # Warn if not treating string literals as "const"
    -Wpointer-arith         # Warn about suspicious pointer operations
    -Wcast-align            # Warn about casts that increase alignment requirements
    -Wno-format-truncation   # Many false positives/non-issues
    -Wno-stringop-truncation # Many false positives/non-issues
)

#
# Ideally system should always be built with -Werror option. However, until
# basecamp is updated to use cFS version 7.x a work around is required if
# the platform is using Python 3.13 or greater. In 3.13 PyWeakref_GetObject
# was deprecated and it is used in the EDS toolchain C code that produces
# the Python libraries. This logic adds the 
#
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.12)
   find_package(Python3 REQUIRED COMPONENTS Interpreter)
   if(Python3_VERSION VERSION_LESS "3.13")
      message(STATUS "Compiling with '-Werror' option")
      add_compile_options(
          -Werror  # Treat warnings as errors (code should be clean)
      )
   else()
      message(STATUS "Compiling without '-Werror' option due to Python version and PyWeakref_GetObject deprecation")   
   endif()
else()
   message(STATUS "Compiling without '-Werror' option because Python version could not be determined")
endif()
