![Static Analysis](https://github.com/nasa/sample_lib/workflows/Static%20Analysis/badge.svg)
![Format Check](https://github.com/nasa/sample_lib/workflows/Format%20Check/badge.svg)

# Core Flight System : Framework : App : Sample Lib

This repository contains a sample library (sample_lib), which is a framework component of the Core Flight System.

This sample library is a non-flight example library implementation for the cFS Bundle. It is intended to be located in the `apps/sample_lib` subdirectory of a cFS Mission Tree.  The Core Flight System is bundled at https://github.com/nasa/cFS (which includes sample_lib as a submodule), which includes build and execution instructions.

sample_lib implements SAMPLE_Function, as an example for how to build and link a library in cFS.

## Version History

### Development Build: v1.2.0-rc1+dev38

-  Implement Coding Standard in CodeQL workflow
- See <https://github.com/nasa/sample_lib/pull/64> and <https://github.com/nasa/cFS/pull/270>

### Development Build: v1.2.0-rc1+dev34

- Replace direct ref to ArgPtr with `UT_Hook_GetArgValueByName` macro. Reading the pointer directly is not advised.
- See <https://github.com/nasa/sample_lib/pull/61> and <https://github.com/nasa/cFS/pull/250>

### Development Build: v1.2.0-rc1+dev30

- Replace <> with " for local includes
- Adds CONTRIBUTING.md that links to the main cFS contributing guide.
- See <https://github.com/nasa/sample_lib/pull/55>

### Development Build: v1.2.0-rc1+dev24

- Simplify build to use wrappers and interface libs
- Add Testing Tools to the Security Policy
- See <https://github.com/nasa/sample_lib/pull/50>

### Development Build: v1.2.0-rc1+dev10

- Rename `UT_SetForceFail` to `UT_SetDefaultReturnValue` since some functions that retain more than 1 value are not necessarily failing
- See <https://github.com/nasa/sample_lib/pull/38>

### Development Build: v1.2.0-rc1+dev8

- No behavior changes. All identifiers now use the prefix `SAMPLE_LIB_`. Changes the name of the init function from SAMPLE_LibInit to SAMPLE_LIB_Init which affects the CFE startup script.
- Set REVISION to "99" to indicate development version status
- See <https://github.com/nasa/sample_lib/pull/35>

### Development Build: v1.2.0-rc1+dev3

- Installs unit test to target directory.
- See <https://github.com/nasa/sample_lib/pull/32>

### Development Build: 1.1.0+dev27

- Install unit test as part of cmake recipe. Sample lib test runner now shows up in expected install directory
- Add build number and baseline to version reporting
- See <https://github.com/nasa/sample_lib/pull/28>

### Development Build: 1.1.4

- Apply code style
- See <https://github.com/nasa/sample_lib/pull/24>

### Development Build: 1.1.3

- Coverage data `make lcov` includes the sample_lib code
- See <https://github.com/nasa/sample_lib/pull/22>

### Development Build: 1.1.2

- Added coverage test and a stub library to facilitate unit test
- See <https://github.com/nasa/sample_lib/pull/16>

### Development Build: 1.1.1

- See <https://github.com/nasa/sample_lib/pull/14>

### ***OFFICIAL RELEASE: 1.1.0 - Aquila***

- Released as part of cFE 6.7.0, Apache 2.0
- See <https://github.com/nasa/sample_lib/pull/6>

### ***OFFICIAL RELEASE: 1.0.0a***

- Released as part of cFE 6.6.0a, Apache 2.0

## Known issues

As a lab library, extensive testing is not performed prior to release and only minimal functionality is included.

## Getting Help

For best results, submit issues:questions or issues:help wanted requests at https://github.com/nasa/cFS.

Official cFS page: http://cfs.gsfc.nasa.gov
