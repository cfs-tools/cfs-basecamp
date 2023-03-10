#!/bin/sh
echo "exe_dir = $1"
echo "exe_file = $2"
echo "password = $3"
cd $1
echo $3 | sudo -S ./$2
