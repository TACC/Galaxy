test ! -d $2/third-party && mkdir $2/third-party

if test ! -d $2/third-party/threadpool11 ; then 

	echo CONFIGURING THREADPOOL11
	if test ! -d threadpool11 ; then 
		mkdir threadpool11
	fi

	cd threadpool11

	cmake $1/third-party/threadpool11 \
	 -DCMAKE_INSTALL_PREFIX=$2/third-party/threadpool11 \
	 -DCMAKE_CXX_FLAGS="-std=c++11 -fPIC"

	make install

else

	echo THREADPOOL11 READY

fi

