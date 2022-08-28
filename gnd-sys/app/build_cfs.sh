echo "Build cFS"
cd $1
make distclean
make SIMULATION=native prep
make install

