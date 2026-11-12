echo "Build cFS"
cd $1
make distclean
make native_eds.prep
make native_eds.install

