# This script is part of the cFS Basecamp 42-Simulation project. It performs
# the following functions:
#   1. Install system utilities required by 42
#   2. Clone 42 and checkout version used by Basecamp
#   3. Add Basecamp customizations to 42
#   4. Build 42
#
# Notes:
#   1. 42 does not tag releases therefore a hash is used to retreive the
#      version of 42 that is compatible with cFS Basecamp
#   2. 
#
_42_VERSION=5e257bb
echo "Install 42 Simulator"
cd ../../tools

###################################
## 1. Install system utilities   ##
###################################

announce "Installing libraries required by 42"
confirm "Continue?" 1
#sudo apt update
#sudo apt upgrade -y
sudo apt install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev

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
