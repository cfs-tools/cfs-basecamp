#!/bin/sh
# This script is part of the cFS Basecamp 42-Simulation project. It performs
# the following functions:
#
#   1. Install system utilities required by 42
#   2. Clone 42 and checkout version used by Basecamp
#   3. Add Basecamp customizations to 42
#   4. Build 42
#
# Notes:
#   1. $1 = Password
#   2. The non-zero return status of 42 is a convention to signal that a project
#      installation did not succeed
#   3. 42 does not tag releases therefore a hash is used to retreive the
#      version of 42 that is compatible with cFS Basecamp
#
_42_VERSION=5e257bb
ret_status=0

cd ../../tools

###################################
## 1. Install system utilities   ##
###################################

#sudo apt update
#sudo apt upgrade -y
#sudo apt install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev
echo $1 | sudo -S apt install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev
#if [ $? -eq 0 ]; then
#   ret_status=42
#fi

###################################
## 2. Install system utilities   ##
###################################

git clone https://github.com/ericstoneking/42.git
cd 42
git checkout $_42_VERSION


###################################
## 3. Add Basecamp customization ##
###################################

cp -r ../../projects/42-sim/BC42 .
cp ../../projects/42-sim/Makefile .


###################################
## 4. Build 42                   ##
###################################

make clean
make


exit $ret_status
