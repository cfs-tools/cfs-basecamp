#
# Example mission_build_custom.cmake
# ----------------------------------
#
# This file will be automatically included in the top level ("mission") build scope
#
# Definitions and options specified here will be used when building local tools and
# other code that runs on the development host, but do _NOT_ apply to flight software
# (embedded) code or anything built for the target machine.
#
# These options assume a GCC toolchain but a similar set should be applicable to clang.
#
add_compile_options(
    -std=c99                # Target the C99 standard (without gcc extensions)
    -pedantic               # Issue all the warnings demanded by strict ISO C
    -Wall                   # Warn about most questionable operations
    -Wstrict-prototypes     # Warn about missing prototypes
    -Wwrite-strings         # Warn if not treating string literals as "const"
    -Wpointer-arith         # Warn about suspicious pointer operations
    -Wcast-align            # Warn about casts that increase alignment requirements
    -Wno-format-truncation    # Many false positives/non-issues
    -Wno-stringop-truncation  # Many false positives/non-issues
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


# The _XOPEN_SOURCE directive is required for glibc to enable conformance with the
# the X/Open standard version 6, which includes POSIX.1c as well as SUSv2/UNIX98 extensions.
add_definitions(
    -D_XOPEN_SOURCE=600
    -DOMIT_DEPRECATED=true
)
