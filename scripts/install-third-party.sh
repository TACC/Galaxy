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
CMAKE_BIN=`which cmake`
CMAKE_FLAGS="-D CMAKE_C_FLAGS:STRING=\"-Wno-undef -Wno-deprecated-declarations\" -D CMAKE_CXX_FLAGS:STRING=\"-Wno-undef -Wno-deprecated-declarations\""

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
for tp_lib_dir in embree ospray rapidjson; do
	report "building ${tp_lib_dir}..."
	pushd $tp_lib_dir
	mkdir build
	cd build
	${CMAKE_BIN} ${CMAKE_FLAGS} .. && make -j 4 install
	if [ $? != 0 ]; then
		fail "Build failed for ${tp_lib_dir}."
	fi
	popd
done

report "done!"
cd ${GXY_ROOT}
mkdir -p ${GXY_ROOT}/.galaxy
touch ${GXY_ROOT}/.galaxy/${GXY_DONE_TAG}
exit 0



