#!/bin/sh
# Assumes only one cFS process is running
echo "Stop cFS Script"
cfs=`pgrep core`
if ! [ -z "$cfs" ]
then
   echo "Killing process $cfs"
   if ! [ -z $1 ]
   then
      echo $1 | sudo -S kill $cfs
   else
      sudo -S kill $cfs
   fi
fi
