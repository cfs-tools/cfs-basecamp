# OpenSatKit Base Camp
Provides a framework and tools for developing, downloading, and integrating core Flight System (cFS) applications into an operational system. A python Graphical User Interface (GUI) allows the user to send commands to and receve telemetry from the cFS target. The GUI also facilitates user workflows for working with apps.  

*Base Camp* includes the [cfe-eds-framework](https://github.com/jphickey/cfe-eds-framework) cFS distribution which includes NASA's core Flight Executive (cFE) and CCSDS Electronic Data Sheets (EDS) support. EDS specifications are Each cFS application interface is defined using EDS and the cfe-eds-framework build toolchain generates artifacts that are used by both the flight and ground software systems. The [OpenSatKit App repositories](https://github.com/OpenSatKit-Apps) contain apps that include EDS interface specifications and they can be downloaded and integrated into the *Base Camp* system with minimal effort. 

# Getting Started

## Prerequisites
The system can be developed on any GNU/Linux development host. The following commands install the development packages for
a Debian/Ubuntu environment. Other Linux distributions should provide a similar set of packages but, the package names and
installation tool names may vary. 

    sudo apt-get update -y 
    sudo apt-get install build-essential
    sudo apt-get install cmake
    sudo apt-get install libexpat1-dev
    sudo apt-get install liblua5.3-dev
    sudo apt-get install libjson-c-dev
    sudo apt-get install python3-dev
    sudo apt-get install python3-pip
    sudo apt-get install python3-tk
   
Package Notes:
- *sudo apt-get update* updates a platform's current package respositories
- *build-essential* contains a C developer tool suite including gcc, libc-dev, make, etc.* 
- *cmake* must be at least v2.8.12
- *liblua5.3-dev* must be at least v5.1
- *You can skip installing pip and replace the 'pip3 install' below with 'python3 -m pip install'

The python appplication uses [PySimpleGUI](https://pysimplegui.readthedocs.io/en/latest/) and [Requests]](https://docs.python-requests.org/en/latest/ that can be installed with the following command:

    pip3 install PySimpleGUI requests

## Clone cFSAT Repository
    git clone https://github.com/OpenSatKit/base-camp.git

# Using Base Camp

## Build the core Flight System Target
This must be done prior to runing the python ground system because it creates python libraries that define the cFS app interfaces

    cd base-camp/cfsat/cfe-eds-framework
    make SIMULATION=native prep
    make install

## Run the Python Ground System Applcation 

    cd ../gnd-sys/app
    . ./setvars.sh
    python3 cfsat.py

## Run the cFS

![](https://github.com/OpenSatKit/base-camp/blob/main/docs/start-cfs.png)

## Next Steps
- [OpenSatKit](https://opensatkit.org/) is being updated to serve as the hub to all OpenSatKit resources 
- *Base Camp Quick Start Guide* (coming soon)

