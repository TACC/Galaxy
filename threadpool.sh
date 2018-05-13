echo CONFIGURING THREADPOOL11
mkdir threadpool11
cd threadpool11

cmake $1/third-party/threadpool11 \
 -DCMAKE_INSTALL_PREFIX=$2/threadpool11 \
 -DCMAKE_CXX_FLAGS=-std=c++11 

make install
