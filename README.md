# core Flight System (cFS) Basecamp
*cFS Basecamp* provides a lightweight environment to help you learn NASA’s core Flight System (cFS) and create app-based solutions for your projects. Basecamp's default cFS target runs on Linux and includes an app suite that provides a complete operational environment including support for onboard file management and transferring files between the ground and flight systems. The built-in hands-on tutorials allow you to be immediately productive. The [Open Mission Stack](https://openmissionstack.com/) website contains cFS educational material that includes [cFS-based projects](https://openmissionstack.com/projects). These goal-oriented software/hardware projects rely on [cFS Basecamp's Apps](https://github.com/orgs/cfs-apps/repositories) to provide the project functionality.  The cFS Basecamp Python GUI automates the app integration process.

This 'App Store' approach is made possible by using a standard application framework that includes runtime JSON initialization files and by using the [cfe-eds-framework](https://github.com/jphickey/cfe-eds-framework) cFS distribution that includes a CCSDS Electronic Data Sheets (EDS) toolchain. Each cFS application interface is defined using EDS specifications and the cfe-eds-framework build toolchain generates artifacts that are used by both the flight and ground software systems. App specification and packaging standards are being defined that will allow the cFS community to easily share apps. 

For users who are working on a flight mission, plans are underway to create an automated transition process from Basecamp’s command and telemetry GUI to a fully functional ground system such as [OpenC3]( https://openc3.com/). The [cFS Platform List](https://github.com/cfs-tools/cfs-platform-list) provides links to additional cFS ports.

# Getting Started

## Prerequisites
The system can be developed on any GNU/Linux development host. The following commands install the development packages for
a Debian/Ubuntu environment. Other Linux distributions should provide a similar set of packages but, the package names and
installation tool names may vary. If you're running on a Raspberry Pi with a 32-bit Raspbian operating system then refer to
the [GPIO Demo tutorial](https://github.com/cfs-tools/cfs-basecamp/tree/main/gnd-sys/tutorials/6-pi-gpio-demo) for details on how to configure and build the cFS. 

    sudo apt-get update -y 
    sudo apt-get install -y build-essential
    sudo apt-get install -y cmake
    sudo apt-get install -y libexpat1-dev
    sudo apt-get install -y liblua5.3-dev
    sudo apt-get install -y libjson-c-dev
    sudo apt-get install -y python3-dev
    sudo apt-get install -y python3-pip
    sudo apt-get install -y python3-tk
    sudo apt install -y default-jre
   
Package Notes:
- *sudo apt-get update* updates a platform's current package respositories
- *build-essential* contains a C developer tool suite including gcc, libc-dev, make, etc.* 
- *cmake* must be at least v3.12 (This excludes Ubuntu 18.04 and earlier)
- *liblua5.3-dev* must be at least v5.1
- You can skip installing pip and replace the 'pip3 install' below with 'python3 -m pip install'
- The Java Runtime Environment (JRE) is required to run the cFS performance monitor

The python appplication uses [PySimpleGUI](https://pysimplegui.readthedocs.io/en/latest/), [Requests](https://docs.python-requests.org/en/latest/), [paho-mqtt](https://pypi.org/project/paho-mqtt/), and [NumPy](https://numpy.org/) that can be installed with the following command:

    pip3 install PySimpleGUI requests paho-mqtt numpy pymupdf

## Clone Basecamp Repository
    git clone https://github.com/cfs-tools/cfs-basecamp.git

# Using Basecamp

## Build the core Flight System Target
This must be done prior to running the python ground system because it creates python libraries that define the cFS app interfaces.

    cd cfs-basecamp/cfe-eds-framework
    make SIMULATION=native prep
    make topicids

## Run the Python Ground System Applcation 

    cd ../gnd-sys/app
    . ./setvars.sh
    python3 basecamp.py

## Next Steps

![](https://github.com/cfs-tools/cfs-basecamp/blob/main/docs/images/next-steps.png)

[![cFS Basecamp](https://i.ytimg.com/vi/jwV3_9W8dcY/maxresdefault.jpg)](https://youtu.be/jwV3_9W8dcY)
