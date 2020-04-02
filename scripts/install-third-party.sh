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


if [ $1 ]; then
	echo "usage: install-third-party.sh"
	echo "This script will build and install the third-party libraries for Galaxy's use assuming reasonable defaults."
	exit 1
fi

function fail
{
	echo "GALAXY: ERROR - $1"
	exit 1
}

function report
{
	echo "GALAXY: $1"
}

GXY_ROOT=$PWD
GXY_PREP_SCRIPT=prep-third-party.sh
GXY_DONE_TAG="gxy_third_party_installed"
CMAKE_BIN=$(which cmake)
QMAKE_BIN=$(which qmake)
QMAKE_DIR=$(dirname $QMAKE_BIN)/..
CMAKE_FLAGS="-DCMAKE_INSTALL_PREFIX=${GXY_ROOT}/third-party/install/ \
             -Wno-undef -Wno-deprecated-declarations"
NODEEDITOR_CMAKE_FLAGS="-DQt5_DIR=${QMAKE_DIR}"
EMBREE_CMAKE_FLAGS="-DEMBREE_ISPC_EXECUTABLE=${GXY_ROOT}/third-party/install/bin/ispc \
						 				-DEMBREE_STATIC_LIB=ON \
						 				-DEMBREE_TUTORIALS=OFF \
						 				-DEMBREE_ISA_SSE2=OFF"
OSPRAY_CMAKE_FLAGS="-Dembree_DIR=${GXY_ROOT}/third-party/install/lib/cmake/embree-3.6.1 \
						  			-DEMBREE_ISPC_EXECUTABLE=${GXY_ROOT}/third-party/install/bin/ispc \
						  			-DCMAKE_CXX_FLAGS=-I${GXY_ROOT}/third-party/install/include"
RAPIDJSON_CMAKE_FLAGS="-DRAPIDJSON_BUILD_DOC=OFF \
											 -DRAPIDJSON_BUILD_EXAMPLES=OFF \
											 -DRAPIDJSON_BUILD_TESTS=OFF"

if [ $TRAVIS_OS_NAME == "osx" ]; then 
	report "setting osx cmake flags"
	CMAKE_MODULE_PATH="${CMAKE_MODULE_PATH}:/usr/local/opt/qt/lib/cmake/Qt5"
	CMAKE_FLAGS="${CMAKE_FLAGS} \
				-D GLUT_INCLUDE_DIR:PATH=/usr/local/Cellar/freeglut/3.2.1/include \
        -D GLUT_glut_LIBRARY:FILEPATH=/usr/local/Cellar/freeglut/3.2.1/lib/libglut.dylib \
        -D Qt5_DIR=/usr/local/opt/qt/lib/cmake/Qt5 \
      	"
elif [ $TRAVIS_OS_NAME == "linux" ]; then 
	report "setting linux cmake flags"
	CMAKE_MODULE_PATH="${CMAKE_MODULE_PATH}:/usr/lib/x86_64-linux-gnu/cmake/Qt5"
	CMAKE_FLAGS="${CMAKE_FLAGS} \
				-D VTK_DIR:PATH=$PWD/../third-party/VTK-8.1.2/install/lib/cmake/vtk-8.1 \
        -D GLUT_INCLUDE_DIR:PATH=/usr/include \
        -D GLUT_glut_LIBRARY:FILEPATH=/usr/lib/x86_64-linux-gnu/libglut.so \
        -D Qt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5 \
        "
fi

if [ -f ${GXY_ROOT}/install-third-party.sh ]; then
	# running from script dir, help a user out
	GXY_ROOT=${PWD}/..
fi

if [ ! -d ${GXY_ROOT}/third-party ]; then
	fail "Please run this script from the root directory of the Galaxy repository."
fi

if [ -f ${GXY_ROOT}/.galaxy/${GXY_DONE_TAG} ]; then
	report "third-party libraries already built and installed!"
	exit 0
fi

if [ ! -x ${CMAKE_BIN} ]; then
	fail "Could not find or run cmake. Please make sure CMake is installed and on your \$PATH."
fi

if [ ! -x ${QMAKE_BIN} ]; then
	fail "Could not find or run qmake. Please make sure Qt5 is installed and on your \$PATH."
fi

# ensure third-party libraries are prepped
# the prep script should bail if libs are already prepped
if [ ! -x ${GXY_ROOT}/scripts/${GXY_PREP_SCRIPT} ]; then 
	fail "Could not run the third-pary prep script. Please make sure ${GXY_PREP_SCRIPT} is present and executable."
else
	${GXY_ROOT}/scripts/${GXY_PREP_SCRIPT}
fi

if [ $? != 0 ]; then
	fail "Could not prep the third-party libraries. Bailing out."
fi

cd ${GXY_ROOT}/third-party
for tp_lib_dir in embree ospray nodeeditor rapidjson; do
	if [ -d install/include/$tp_lib_dir ] || [ -d install/include/${tp_lib_dir}3 ]; then # silly embree uses embree3
		report "$tp_lib_dir already installed."
	else 
		report "building ${tp_lib_dir}..."
		pushd $tp_lib_dir
		mkdir -p build
		cd build
		ALL_CMAKE_FLAGS="${CMAKE_FLAGS}"
		case $tp_lib_dir in 
			nodeeditor)
			ALL_CMAKE_FLAGS="${ALL_CMAKE_FLAGS} ${NODEEDITOR_CMAKE_FLAGS}"
			;;
			embree)
			ALL_CMAKE_FLAGS="${ALL_CMAKE_FLAGS} ${EMBREE_CMAKE_FLAGS}"
			;;
			ospray)
			ALL_CMAKE_FLAGS="${ALL_CMAKE_FLAGS} ${OSPRAY_CMAKE_FLAGS}"
			;;
			rapidjson)
			ALL_CMAKE_FLAGS="${ALL_CMAKE_FLAGS} ${RAPIDJSON_CMAKE_FLAGS}"
			;;
			*)
			report "unknown tp_lib_dir '${tp_lib_dir}'"
			;;
		esac
		${CMAKE_BIN} ${ALL_CMAKE_FLAGS} .. && make -j 4 install
		if [ $? != 0 ]; then
			fail "Build failed for ${tp_lib_dir}."
		fi
		popd
	fi
done

report "done!"
cd ${GXY_ROOT}
mkdir -p ${GXY_ROOT}/.galaxy
date > ${GXY_ROOT}/.galaxy/${GXY_DONE_TAG}
exit 0



