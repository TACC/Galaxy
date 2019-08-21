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
	echo "usage: prep-third-party.sh"
	echo "This script will init and update submodules, download ispc, and apply patches to the third-party libraries."
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
GXY_DONE_TAG="gxy_third_party_prep_done"

if [ -f ${GXY_ROOT}/prep-third-party.sh ]; then
	# running from script dir, help a user out
	GXY_ROOT=${PWD}/..
fi

if [ ! -d ${GXY_ROOT}/third-party ]; then
	fail "Please run this script from the root directory of the Galaxy repository."
fi

if [ -f ${GXY_ROOT}/.galaxy/${GXY_DONE_TAG} ]; then
	report "third-party libraries already prepped!"
	exit 0
fi

report "initializing submodules..."
git submodule --quiet init
if [ $? != 0 ]; then
	fail "submodule init returned error code $?"
fi

report "updating submodules..."
git submodule --quiet update
if [ $? != 0 ]; then
	fail "submodule update returned error code $?"
fi

report "checking ispc..."
cd ${GXY_ROOT}/third-party/ispc
./get-ispc.sh
if [ $? != 0 ]; then
	fail
fi

report "applying patches..."
PATCH_DIR=${GXY_ROOT}/third-party/patches
cd ${PATCH_DIR}
for patch in *.patch ; do
	PATCH_TARGET=`echo ${patch} | sed -e 's/\.patch//'`
	if [ ! -d ${GXY_ROOT}/third-party/${PATCH_TARGET} ]; then
		fail "could not find ${PATCH_TARGET} at ${GXY_ROOT}/third-party/${PATCH_TARGET}"
	fi
	report "  patching ${PATCH_TARGET} ..."
	cd ${GXY_ROOT}/third-party/${PATCH_TARGET}
	git apply ${PATCH_DIR}/${patch}
	if [ $? != 0 ]; then
		fail "patch application for ${PATCH_TARGET} returned error code $?"
	fi
done

cd ${GXY_ROOT}
mkdir -p ${GXY_ROOT}/.galaxy
date > ${GXY_ROOT}/.galaxy/${GXY_DONE_TAG}
report "done!"
exit 0






