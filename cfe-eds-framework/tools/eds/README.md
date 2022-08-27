# CCSDS SOIS Electronic Data Sheet Tool and Library

This repository contains an implementation of a tool and runtime library
for embedded software to create and interpret data structures defined 
using Electronic Data Sheets per CCSDS book 876.0.

The full specification for Electronic Data Sheets is available here:

[CCSDS 876.0-B-1](https://public.ccsds.org/Pubs/876x0b1.pdf)

This repository contains the basic tool to process EDS files, a
runtime library (EdsLib) for embedded software, and libraries 
to interoperate with JSON, Lua, and Python.

This software is also intended to work with the Core Flight System:

[cFS](https://github.com/nasa/cFS)

A set of patches to CFS to enable EDS features is also available.

## General Overview

The tool reads all EDS files and generates an in-memory document object model (DOM) 
structure which can then be queried by scripts which can in turn generate derived outputs 
based on the EDS information.

This DOM is conceptually similar to the Javascript DOM of HTML documents in a web browser,
but very different in terms of usage and implementation as it represents a very different
type of document.

Scripts currently exist for generating C header files and runtime data structures to work
with the accompanying EdsLib runtime library.  However, applications can supply additional
scripts and generate custom outputs from the same DOM.

## Components

The following subdirectories are contained within this source tree:

- `tools` contains the build tool to read EDS files, generate the DOM tree, and run scripts
- `edslib` contains the runtime C library for dealing with EDS-defined objects
- `doc` contains additional information about the DOM structure
- `cfecfs` contains additional bindings/libraries for use with Core Flight System

A separate CMake script is included in each subdirectory for building each component.

## Execution

Once built, the tool is executed by supplying a set of XML files and processing
scripts on the command line, such as:

```
$ sedstool MyEDSFile1.xml MyEDSFile2.xml MyScript1.lua MyScript2.lua ...
```

A few command line options are recognized:

- `-v` : Increase verbosity level.  Use twice for full debug trace.
- `-D NAME=VALUE` : Sets the symbolic NAME to VALUE for preprocessor substitutions


However, this tool is generally _not_ intended to be executed manually in a standalone
installation, but built and executed as part of a larger application build system, such
as Core Flight System (cFS).

The tool will first read _all_ the supplied XML files and build a DOM tree, and then it
will invoke each Lua script _in alphanumeric order_ of filename.  Ordering is
very important, as each script can build upon the results of the prior scripts.  To preserve
the intended order of operations, each script supplied with this tool contains a two digit
numeric prefix, which indicates the correct position in the set.  This way, when scripts are
executed in a simple alphanumeric order, and this will always produce the correct result.
Furthermore, additional scripts can be added into the sequence simply by choosing an appropriate
prefix number, without needing to specify explicit dependencies or complicated rules.
