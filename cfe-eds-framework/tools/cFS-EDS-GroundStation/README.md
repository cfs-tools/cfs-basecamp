# cFS-EDS-GroundStation

This is a Python based ground station that interfaces with an Electronic Data Sheets supported Core Flight Systems instance.  
Also, there are several utility python scripts that provide basic Telemetry and Telecommand functionality in a non-GUI format.

The user's manual can be found at docs/cFS-EDS-GroundStation Users Manual.docx

# Prerequisistes

  -python3-dev
  -python3-pyqt5

# Configuring and running

This software is designed to work with the cfe-eds-framework repository which can be found at:
https://github.com/jphickey/cfe-eds-framework

To incorporate the cFS-EDS-GroundStation software within this version of core flight, download the repository to the
${CFS_HOME}/tools/ directory and add the subdirectory in the cfs build process (cfe/cmake/mission_build.cmake)

```
  # Include all the EDS libraries and tools which are built for the host system
  include_directories(${MISSION_BINARY_DIR}/inc)
  add_subdirectory(${MISSION_SOURCE_DIR}/tools/eds/edslib eds/edslib)
  add_subdirectory(${MISSION_SOURCE_DIR}/tools/eds/tool   eds/tool)
  add_subdirectory(${MISSION_SOURCE_DIR}/tools/eds/cfecfs eds/cfecfs)
  add_subdirectory(${MISSION_SOURCE_DIR}/tools/cFS-EDS-GroundStation eds/cFS-EDS-GroundStation)
```

To enable the build process for the cFS-EDS-GroundStation, set the "CONFIGURE_CFS_EDS_GROUNDSTATION" cmake variable to "ON".
This variable can be set in the root Makefile or in the CMakeCache.txt of the build directory after the "make prep" step.
The build process automatically configures the python files with the defined mission name and outputs them
to the ${CMAKE_BINARY_DIR}/exe/host/cFS-EDS-GroundStation/ folder.

The cFS-EDS-GroundStation software requries the EdsLib and CFE_MissionLib python modules from the cfe-eds-framework repository.
These are both built by turning on the following cmake variables:

```
EDSLIB_PYTHON_BUILD_STANDALONE_MODULE:BOOL=ON
CFE_MISSIONLIB_PYTHON_BUILD_STANDALONE_MODULE:BOOL=ON
```

The folder where the python modules are installed by default is:
${CMAKE_BINARY_DIR}/exe/lib/python

This folder needs to be added to the PYTHON_PATH environment variable so the modules can be imported into Python.
The folder that contains EdsLib and CFE_MissionLib .so files also needs to be added to the LD_LIBRARY_PATH
enviroment variable so Python can load these libraries.  For example the following lines can be added to ~/.bashrc

CFS_BUILD = /path/to/cfs/build/directory/
export PYTHONPATH=$PYTHONPATH:$CFS_BUILD/exe/lib/python
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CFS_BUILD/exe/lib

With everything set up the cFS-EDS-GroundStation can be run with the following command:

python3 cFS-EDS-GroundStation.py

# Utility python scripts

python3 cmdUtil.py

This script runs through several prompts to send a command to a core flight instance.  The user inputs the instance name,
topic, subcommand (if applicable), payload values (if applicable), and destination IP address/port number.  The script will create, 
fill, pack, and send a command to the desired IP address.

python3 tlm_decode.py -p <port=1235>

This script will listen in on the specified port for telemetry messages.  When messages come in they are decoded
and the contents are displayed on the screen.

python3 convert_tlm_file.py -f <filename>       or
python3 convert_tlm_file.py --file=<filename>   (recommended as this allows tab completion of file names)

This script takes the binary telementry files from the Telemetry System of the cFS-EDS-GroundStation, reads through all
of the messages in the file, and writes the decoded packet information in a csv fie of the same base name.
This csv file can be imported into any Excel type program for further analysis.
