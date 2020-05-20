#!/bin/bash
## ========================================================================== ##
## Copyright (c) 2014-2019 The University of Texas at Austin.                 ##
## All rights reserved.                                                       ##
##                                                                            ##
## Licensed under the Apache License, Version 2.0 (the "License");            ##
## you may not use this file except in compliance with the License.           ##
## A copy of the License is included with this software in the file LICENSE.  ##
## If your copy does not contain the License, you may obtain a copy of the    ##
## License at:                                                                ##
##                                                                            ##
##     https://www.apache.org/licenses/LICENSE-2.0                            ##
##                                                                            ##
## Unless required by applicable law or agreed to in writing, software        ##
## distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  ##
## WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           ##
## See the License for the specific language governing permissions and        ##
## limitations under the License.                                             ##
##                                                                            ##
## ========================================================================== ##

function report
{
	echo "GALAXY: $1"
}

function fail
{
	echo "GALAXY: ERROR - $1"
	exit 1
}

report "running $0"

if [ $TRAVIS_OS_NAME == "linux" ]; then
	report "checking for linux VTK build..."
	if [ -d third-party/VTK-8.1.2/install ]; then
		report "  found VTK-8.1.2 in third-party."
	else
		GXY_BUILT_VTK=1
		report "  VTK not found, building and caching for this run"
		report "  that's all we can do within travis-ci time limits, skipping full build"
		pushd third-party
		wget https://www.vtk.org/files/release/8.1/VTK-8.1.2.tar.gz \
		  && tar xf VTK-8.1.2.tar.gz \
		  && cd VTK-8.1.2 \
		  && mkdir build \
		  && cd build \
		  && cmake -D CMAKE_BUILD_TYPE:STRING=MinSizeRel \
		           -D CMAKE_INSTALL_PREFIX:PATH=$PWD/../install \
               -D CMAKE_C_FLAGS:STRING="-Wno-deprecated-register" \
               -D CMAKE_CXX_FLAGS:STRING="-Wno-deprecated-register" \
		           .. \
		  && make -j 4 install 
		if [ $? != 0 ]; then
			fail "VTK build failed with code $?"
		fi
		popd
	fi

  report "checking for VTK python wrapper..."
  if [ -d third-party/VTK-8.1.2/install/lib/python2.7/site-packages/vtk ]; then
    report "  found python wrappers in VTK install."
  elif [ ${GXY_BUILT_VTK} ]; then
  	report "  seems VTK was built this run, no time to build the python wrapper too"
  else
    GXY_BUILT_VTK=1
    report "  VTK python wrappers not found, building and caching for this run"
    report "  that's all we can do within travis-ci time limits, skipping full build"
    pushd third-party/VTK-8.1.2/build \
      && cmake -D VTK_WRAP_PYTHON:BOOL=ON .. \
      && make -j 4 install
    if [ $? != 0 ]; then
      fail "VTK python wrapper failed with code $?"
    fi
    popd
  fi
fi


if [ -z ${GXY_BUILT_VTK} ]; then
	report "ensuring third-party libraries are built..."
	scripts/install-third-party.sh

	report "building interactive interface..."
  mkdir -p build
  pushd build
  if [ $TRAVIS_OS_NAME == "osx" ]; then 
		PATH="${PATH}:/usr/local/opt/qt/bin"
  	cmake -D GLUT_INCLUDE_DIR:PATH=/usr/local/Cellar/freeglut/3.2.1/include \
          -D GLUT_glut_LIBRARY:FILEPATH=/usr/local/Cellar/freeglut/3.2.1/lib/libglut.dylib \
          -D Qt5_DIR:PATH=/usr/local/opt/qt/lib/cmake/Qt5 \
          -D CMAKE_VERBOSE_MAKEFILE:BOOL=ON \
          .. \
  		&& make install
  elif [ $TRAVIS_OS_NAME == "linux" ]; then 
  	cmake -D VTK_DIR:PATH=$PWD/../third-party/VTK-8.1.2/install/lib/cmake/vtk-8.1 \
          -D GLUT_INCLUDE_DIR:PATH=/usr/include \
          -D GLUT_glut_LIBRARY:FILEPATH=/usr/lib/x86_64-linux-gnu/libglut.so \
          -D Qt5_DIR:PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5 \
          -D CMAKE_VERBOSE_MAKEFILE:BOOL=ON \
          .. \
      && make install
	fi
	if [ $? != 0 ]; then
		fail "interactive interface build failed!"
	fi

	report "building image-writing interface..."
	# this should work for both osx and linux, since cmake was configured above
	cmake -D GXY_WRITE_IMAGES:BOOL=ON . && make install 
	if [ $? != 0 ]; then
		fail "image-writing interface build failed!"
	fi

	report "building unit tests..."
	# this should work for both osx and linux, since cmake was configured above
	cmake -D GXY_UNIT_TESTING:BOOL=ON . && make install 
	if [ $? != 0 ]; then
		fail "unit testing build failed!"
	fi	
fi

report "done!"
exit 0
