# core Flight System (cFS) Basecamp
*Basecamp* provides a lightweight environment to help you learn NASA’s core Flight System (cFS) and create app-based solutions for your projects.  The built-in hands-on tutorials allow you to be immediately productive.  Basecamp’s ‘app store’ model lets you download and integrate apps with only a few mouse clicks. 

Basecamp is ideal for cFS training and STEM educational products. For example, if you want to learn how to use the cFS to interface to a hardware device, the *Raspberry Pi GPIO Demo* tutorial walks you through the following steps so you can have a running system in a very short time. 

1.	Wire an LED to a Raspberry Pi’s interface pins
2.	Install Basecamp on a Raspberry Pi
3.	From the Basecamp GUI:
 * Clone the [Pi IO Library](https://github.com/cfs-apps/pi_iolib)
 * Clone the [GPIO Demo App]( https://github.com/cfs-apps/gpio_demo) 
 * Run Basecamp’s app integration/build tool
4.	Restart Basecamp and run the new cFS target
5.	Operate the GPIO Demo app using the GUI’s command and telemetry menus

Basecamp uses goal-oriented software and hardware projects, so users learn how to create app-based solutions to meet their needs. These projects and additional cFS learning material can be found at [Open Mission Stack](https://openmissionstack.com/).  This approach is made possible because Basecamp’s cFS target app suite provides a complete operational environment including transferring files between the ground and flight systems and managing onboard files. 
For users who are working on a flight mission, plans are underway to create an automated transition process from Basecamp’s command and telemetry GUI to a fully functional ground system such as [OpenC3]( https://openc3.com/) 

Basecamp’s app store approach is made possible by using a standard application framework that includes runtime JSON initialization files and by using the [cfe-eds-framework](https://github.com/jphickey/cfe-eds-framework) cFS distribution that includes a CCSDS Electronic Data Sheets (EDS) toolchain. Each cFS application interface is defined using EDS specifications and the cfe-eds-framework build toolchain generates artifacts that are used by both the flight and ground software systems. App specification and packaging standards are being defined that will allow the cFS community to easily share apps. 

# Getting Started

## Prerequisites
The system can be developed on any GNU/Linux development host. The following commands install the development packages for
a Debian/Ubuntu environment. Other Linux distributions should provide a similar set of packages but, the package names and
installation tool names may vary. If you're running on a Raspberry Pi with a 32-bit Raspbian operating system then refer to
the [GPIO Demo tutorial](https://github.com/cfs-tools/cfs-basecamp/tree/main/gnd-sys/tutorials/6-pi-gpio-demo) for details on how to configure and build the cFS. 

    sudo apt-get update -y 
    sudo apt-get install build-essential
    sudo apt-get install cmake
    sudo apt-get install libexpat1-dev
    sudo apt-get install liblua5.3-dev
    sudo apt-get install libjson-c-dev
    sudo apt-get install python3-dev
    sudo apt-get install python3-pip
    sudo apt-get install python3-tk
    sudo apt install default-jre
   
Package Notes:
- *sudo apt-get update* updates a platform's current package respositories
- *build-essential* contains a C developer tool suite including gcc, libc-dev, make, etc.* 
- *cmake* must be at least v2.8.12
- *liblua5.3-dev* must be at least v5.1
- You can skip installing pip and replace the 'pip3 install' below with 'python3 -m pip install'
- The Java Runtime Environment (JRE) is required to run the cFS performance monitor

The python appplication uses [PySimpleGUI](https://pysimplegui.readthedocs.io/en/latest/), [Requests]](https://docs.python-requests.org/en/latest/), [paho-mqtt](https://pypi.org/project/paho-mqtt/), and [NumPy]https;//numpy.org/) that can be installed with the following command:

    pip3 install PySimpleGUI requests paho-mqtt numpy

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

