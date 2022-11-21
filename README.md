# core Flight System Basecamp
Provides a framework and tools for developing, downloading, and integrating core Flight System (cFS) applications into an operational system. A lightweight python Graphical User Interface (GUI) allows users to send commands, receive telemetry, and transfer files with the cFS target.

*Basecamp* includes the [cfe-eds-framework](https://github.com/jphickey/cfe-eds-framework) cFS distribution which includes NASA's core Flight Executive (cFE) and a CCSDS Electronic Data Sheets (EDS) toolchain. Each cFS application interface is defined using EDS specifications and the cfe-eds-framework build toolchain generates artifacts that are used by both the flight and ground software systems. The [cFS App Repositories](https://github.com/orgs/cfs-apps/repositories) contain apps that include EDS interface specifications so the apps can be downloaded and integrated into a *Basecamp* system with only a few mouse clicks. 

*Basecamp* includes built-in tutorials with hands-on exercises to shorten the path to productivity. Additonal cFS learning material can be found at [Open Mission Stack](https://openmissionstack.com/)

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

The python appplication uses [PySimpleGUI](https://pysimplegui.readthedocs.io/en/latest/), [Requests]](https://docs.python-requests.org/en/latest/), [paho-mqtt](https://pypi.org/project/paho-mqtt/), and [NumPy]https;//numpy.org/) that can be installed with the following command:

    pip3 install PySimpleGUI requests paho-mqtt numpy

## Clone Basecamp Repository
    git clone https://github.com/cfs-tools/cfs-basecamp.git

# Using Basecamp

## Build the core Flight System Target
This must be done prior to running the python ground system because it creates python libraries that define the cFS app interfaces

    cd cfs-basecamp/cfe-eds-framework
    make SIMULATION=native prep
    make topicids

## Run the Python Ground System Applcation 

    cd ../gnd-sys/app
    . ./setvars.sh
    python3 basecamp.py

## Next Steps

![](https://github.com/cfs-tools/cfs-basecamp/blob/main/docs/images/next-steps.png)

