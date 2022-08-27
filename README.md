# Core Flight System with Electronic Data Sheets

A distribution of the open source CFE framework which includes CCSDS Electronic Data Sheet support.
This repository represents an assembly of CFE framework components, merged together into a single
git repository using "git subtree".

The following NASA open source releases are part of this repository:

    GSC-18128-1, Core Flight Executive Version 6.7
    GSC-18370-1, Operating System Abstraction Layer
    LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation

This comprises the following individual subdirectories:

- [cFE](https://github.com/nasa/cFE) in `./cfe` subtree  
- [osal](https://github.com/nasa/osal) in `./osal` subtree
- [psp](https://github.com/nasa/psp) in `./psp` subtree
- [EdsLib](https://github.com/nasa/EdsLib) in `./tools/eds` subtree
- [ci_lab](https://github.com/nasa/ci_lab) in `./apps/ci_lab` subtree
- [to_lab](https://github.com/nasa/to_lab) in `./apps/to_lab` subtree
- [sch_lab](https://github.com/nasa/sch_lab) in `./apps/sch_lab` subtree
- [sample_app](https://github.com/nasa/sample_app) in `./apps/sample_app` subtree
- [sample_lib](https://github.com/nasa/sample_app) in `./apps/sample_lib` subtree

Each component contains a LICENSE file in its respective directory.


# Getting Started

## Prerequisites

The system can be developed on any GNU/Linux development host, although most testing takes place
on Ubuntu "LTS" distributions.  The following development packages must be installed on the host.
Note these are names of Debian/Ubuntu packages; other Linux distributions should provide a similar 
set but the package names may vary. 

- `build-essential` (contains gcc, libc-dev, make, etc.) 
- `cmake` (at least v2.8.12 recommended)
- `libexpat1-dev`
- `liblua5.3-dev` (older versions of Lua may work, at least v5.1 is required)
- `libjson-c-dev` (optional; for JSON bindings)
- `python3-dev` (optional; for python bindings)


## Initial Setup

The distribution contains a "sample" configuration and wrapper Makefile which is intended as a
starting point for users.  This configuration may be copied to the top level and extended from
there.  

Commands:

    cp -r ./cfe/cmake/sample_defs .
    cp -r ./cfe/cmake/Makefile.sample Makefile

The `sample_defs` directory contains a complete CFE and OSAL configuration.  Once cloned, it can
be modified or extended with project-specific items as needed.  In particular the `targets.cmake`
file controls which applications are built.


## Building the Software

The software uses CMake to generate Unix Makefiles to perform the build.  However, due to the fact
that a CFE mission may contain multiple different targets and several different architectures, the
build is implemented in several tiers.  The "mission" is the top-tier, and the "arch" is the lower-
tier, and multiple different "arch" tiers may exist.  A "mission-all" custom target is defined at 
the top level which builds all tiers.
 
The sample `Makefile` contains wrapper targets for both executing CMake to prepare the build tree,
and for the actual software build.  This wrapper makes it easy to integrate into various IDEs, even
if the IDE is not CMake-aware.

The variable `SIMULATION` may be used to override the system architecture in the configuration
files.  The special keyword `native` is recognized to indicate the native system.  In this mode, 
the default host compiler (e.g. `/usr/bin/cc`) is used to build all binaries, regardless of the
target architecture.  This feature builds an executable suitable for running and debugging directly
on the development host.  

To prepare a build tree, which will be generated in `./build` by default:

    make SIMULATION=native prep

To build all binaries:

    make all

To stage the software for execution:

    make install

The "install" target stages the output by default into `./build/exe`

## Executing the software

If the `SIMULATION=native` flag is supplied in the initial setup, then the resulting binaries can
be executed directly on the development host.  Note that OSAL uses a virtualized file system which
is rooted in the current working directoy (cwd) so one should always `cd` into the staging tree
prior to executing CFE.

To execute the software on the development host:

    cd build/exe/cpu1
    ./core-cpu1

Commands may be sent to the software using the `cmdUtil` host tool.  This is installed in the 
`host` subdirectory.  From another terminal/window:

    cd build/exe/host
    ./cmdUtil -D CFE_ES/Application/CMD.Noop

To view telemetry, first enable telemetry output in TO_LAB and send to localhost:

    ./cmdUtil -D TO_LAB/Application/EnableOutput dest_IP=127.0.0.1

Then to view and decode the telemetry being sent:

    ./tlm_decode


# Next Steps

This distribution may serve as the baseline for a CFE mission.  It may be forked and extended
for mission-specific needs while still retaining the relationship to the original component sources
for future patching/upgrading as needed.

## Changing the Name

The distribution contains a configuration named "SampleMission" in the `sample_defs` directory.  
Generally one of the first steps is to rename this to be more appropriate.

- Rename the `sample_defs` directory to `${name}_defs` (retaining the _defs suffix)
- Rename and update `sample_mission_cfg.h` and `sample_perfids.h` file accordingly
- Update the `MISSION_NAME` and `SPACECRAFT_ID` within the targets.cmake file


## Adding and Updating an App

Third party CFS applications may be obtained through a variety of different channels.  The package 
or upstream app repository should be typically placed as a subdirectory under `./apps`
with a directory name matching the name of the application or library.

If the upstream application is in a git repository, then the `git subtree add` command may be 
used to add the application, which retains a relationship to the original source:

    git remote add ${name} ${repo_url}
    git config remote.${name}.tagOpt --no-tags
    git fetch ${name}
    git subtree add -P apps/${name} ${name}/master
    
See notes below for further explanation of the `--no-tags` option and why this is often necessary.

If/when a new version of the upstream app is released after the initial subtree add, it may be 
merged, for example:

    git fetch ${name}
    git subtree merge -P apps/${name} ${name}/master

If the application is distributed as a tarball or zipfile, then the distribution file may be
simply extracted as a subdirectory within `./apps`.


The application should then be added to `targets.cmake` and `cfe_es_startup.scr` within the 
mission configuration directory (e.g. `sample_defs`) to build and execute it.

**IMPORTANT**: The build scripts will search for applications and modules based on the name, so
the directory name containing the module must match the name of the module listed in 
`targets.cmake` exactly.  It is recommended to use all lowercase names to avoid issues with case
senstivity in file systems, and avoid any sort of punctuation aside from underscores. 

### Subtrees and Tags ###

git implments a single/unified namespace for tags, and as a result the tags of any repository
added as a remote per the above will also fetch the tags of that remote, and create local tags
of the same name.  If tags are simply named, such as e.g. `v2.4.0`, then it is possible to get
duplicate/conflicting tags between subtree repositories.

To avoid this issue, it is recommended to fetch with `--no-tags` for repositories used as subtrees.
This can be configured persistently using the command:

    git config remote.${name}.tagOpt --no-tags
    
Where `${name}` represents the name of the remote.  The result should be equivalent to specifying
the `--no-tags` option on all fetches from this remote.

**OPTIONAL**: Translate tags when fetching and introduce a namespace prefix

It is also fairly simple to add and explicit specification to fetch tags but translate them
into a namespace.  This can be done by editing the `.git/config` file within the base repository,
and adding a line to each remote used with subtrees:

    fetch = +refs/tags/*:refs/tags/${name}-*
    
Again, where `${name}` represents the name of the remote.  This creates a local tag with the same
name but with a prefix based on the remote name, thereby avoiding tag conflicts.  Note that one
still needs to use the `--no-tags` option per above, otherwise two tags will be locally created, 
one with the prefix and one without the prefix.


## Further information

As this repository represents only an assemply of components without any additional code, any
issues should be submitted to the upstream component whenever possible.

Additional resources may be found at:

- [STRS](https://strs.grc.nasa.gov/repository) contains an STRS OE and FCI app for CFE
- [NASA Software Catalog](https://software.nasa.gov) contains some CFE-related software
- [Subtree Tutorial](https://www.atlassian.com/git/tutorials/git-subtree) contains more information 
about "git subtree" and the related commands.





