# core Flight System (cFS) Basecamp
*cFS Basecamp* provides a lightweight environment to help you learn NASA’s [core Flight System (cFS)](https://github.com/nasa/cFS) and create app-based solutions for your projects. Basecamp's default cFS target runs on Linux and includes an app suite that provides a complete operational environment including support for onboard file management and transferring files between the ground and flight systems. The built-in hands-on tutorials allow you to be immediately productive. The [Space Steps](https://spacesteps.com/) website contains cFS educational material that includes [cFS-based projects](https://spacesteps.com/category/projects/). These goal-oriented software/hardware projects rely on [cFS Basecamp's Apps](https://github.com/orgs/cfs-apps/repositories) to provide the project functionality.  The cFS Basecamp Python GUI automates the app integration process.

This 'App Store' approach is made possible by using a standard application framework that includes runtime JSON initialization files and by using the [cfe-eds-framework](https://github.com/jphickey/cfe-eds-framework) cFS distribution that includes a CCSDS Electronic Data Sheets (EDS) toolchain. Each cFS application interface is defined using EDS specifications and the cfe-eds-framework build toolchain generates artifacts that are used by both the flight and ground software systems. App specification and packaging standards are being defined that will allow the cFS community to easily share apps. 

For users who are working on a flight mission, the [cFS Platform List](https://github.com/cfs-tools/cfs-platform-list) provides links to additional cFS ports. Currently, there is no automated transition process from Basecamp’s command and telemetry GUI to a fully functional ground system.

# Configure Your Environment

## Operating System Prerequisites
The system can be developed on any GNU/Linux development host. The following commands install the development packages for
a Debian/Ubuntu environment. Other Linux distributions should provide a similar set of packages but, the package names and
installation tool names may vary. If you're running on a Raspberry Pi with a 32-bit Raspbian operating system please refer to
the [cFS Raspberry Pi LED Control Project](https://spacesteps.com/2024/10/12/cfs-raspberry-pi-led-control/) for details on how to configure and build the cFS. 

    sudo apt-get update -y 
    sudo apt-get install -y build-essential
    sudo apt-get install -y cmake
    sudo apt-get install -y libexpat1-dev
    sudo apt-get install -y liblua5.3-dev
    sudo apt-get install -y libjson-c-dev
    sudo apt-get install -y python3-dev
    sudo apt-get install -y python3-pip
    sudo apt-get install -y python3-tk
    sudo apt-get install -y python3-venv
    sudo apt install -y default-jre
   
Package Notes:
- *sudo apt-get update* updates a platform's current package respositories
- *build-essential* contains a C developer tool suite including gcc, libc-dev, make, etc.* 
- *cmake* must be at least v3.12 (This excludes Ubuntu 18.04 and earlier)
- *liblua5.3-dev* must be at least v5.1
- You can skip installing pip and replace the 'pip3 install' below with 'python3 -m pip install'
- The Java Runtime Environment (JRE) is required to run the cFS performance monitor

## Python Prerequisites
The Python Preferred Installer Program (PIP) is used to install [PyPI](https://pypi.org/) packages. Traditionally these packages have been installed from the OS command line. [Python Enhancement Proposal (PEP) 668](https://peps.python.org/pep-0668/) is requiring pip installations to occur within Python virtual environments. This [Python on Raspberry Pi](https://www.raspberrypi.com/documentation/computers/os.html#python-on-raspberry-pi) article provides more information. 

Perform the following steps to create and activate a Python virtual environment for *'your_project'*:

    mkdir your_project
    cd your_project
    python3 -m venv env

    source env/bin/activate
    
Your command line prompt should now begin with (env) to indicate you are running in the virtual environment. Next, use PIP to install the packages required by Basecamp. Note PysimpleGUI is installed after Basecamp is cloned from github.

    pip3 install rsa requests paho-mqtt numpy pymupdf

# Install and Run Basecamp
Begin these steps in *'your_project'* directory created during the Python Prerequisite steps.

## Clone Basecamp Repository and Install Local PySimpleGUI
    git clone https://github.com/cfs-tools/cfs-basecamp.git
    python -m pip install ./cfs-basecamp/gnd-sys/app/PySimpleGUI-5.0.2026.0-py3-none-any.whl
    
## Build the core Flight System Target
This must be done prior to running the Python ground system because it creates Python libraries that define the cFS app interfaces.

    cd cfs-basecamp/cfe-eds-framework
    make SIMULATION=native prep
    make topicids

## Run the Python Ground System Application 
**These steps must be performed within *your_project's* activated Python virtual environment**

    cd ../gnd-sys/app
    . ./setvars.sh
    python3 basecamp.py

## Operating Python Virtual Environments  
To stop/exit a virtual environment issue the *deactivate* directive and your command line prompt should nolonger start with (env). Each time you need to start *your_project's* virtual environment perform the following commands:

    cd your_project
    source env/bin/activate

# Next Steps

See the [cFS Basecamp Wiki](https://github.com/cfs-tools/cfs-basecamp/wiki) for how Basecamp can be used for learning flight software concepts and for developing cfs-based flight software systems.
