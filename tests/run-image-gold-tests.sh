#!/bin/bash
## ========================================================================== ##
## Copyright (c) 2014-2018 The University of Texas at Austin.                 ##
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
	echo "usage: run-image-gold-tests.sh"
	echo "This script will generate a radial dataset, render it with Galaxy, and compare the results against the golden images in this directory."
	exit 1
fi

function report
{
	echo "GALAXY: $1"
}

function fail
{
	echo "GALAXY: ERROR - $1"
	exit 1
}

GXY_ROOT=$PWD

if [ -f ${GXY_ROOT}/run-image-gold-tests.sh ]; then
	# already in test dir, help a user out
	GXY_ROOT=${PWD}/..
elif [ -d ${GXY_ROOT}/tests ]; then
	# in root dir, move to tests
	cd ${GXY_ROOT}/tests
else 
	fail "could not find tests dir from PWD: $PWD"
fi


GXY_BIN=${GXY_ROOT}/install/bin
GXY_RADIAL=${GXY_BIN}/radial
GXY_VTI2VOL=${GXY_BIN}/vti2vol
GXY_IMAGE_WRITER=${GXY_BIN}/vis
GXY_ENV=${GXY_ROOT}/install/galaxy.env
IMAGEMAGICK_COMPARE=`which compare`
IMAGEMAGICK_IDENTIFY=`which identify`

if [ ! -x ${GXY_RADIAL} ]; then
	fail "Could not find or execute the Galaxy radial generator '${GXY_RADIAL}'"
fi
if [ ! -x ${GXY_VTI2VOL} ]; then
	fail "Could not find or execute the Galaxy vti2vol '${GXY_VTI2VOL}'"
fi
if [ ! -x ${GXY_IMAGE_WRITER} ]; then
	fail "Could not find or execute the Galaxy image writer '${GXY_IMAGE_WRITER}'. Ensure Galaxy was configured with -D GXY_WRITE_IMAGES=ON"
fi
if [ ! -f ${GXY_ENV} ]; then
	fail "Could not find Galaxy environment file at '${GXY_ENV}'"
fi
if [ -z ${IMAGEMAGICK_COMPARE} ]; then
	fail "Could not find ImageMagick compare"
fi
if [ -z ${IMAGEMAGICK_IDENTIFY} ]; then
	fail "Could not find ImageMagick identify"
fi

report "Sourcing Galaxy environment"
. ${GXY_ENV}
# reasonable settings for Travis-CI VM environment
export GXY_APP_NTHREADS=1
export GXY_NTHREADS=4

report "Generating radial-0.vti with ${GXY_RADIAL}"
${GXY_RADIAL} -r 256 256 256
if [ $? != 0 ]; then
	fail "$GXY_RADIAL exited with code $?"
fi

report "Converting vti to vol with ${GXY_VTI2VOL}"
# TODO: remove this hack once galaxy.env includes VTK on PYTHONPATH
if [ ${TRAVIS_OS_NAME} == "linux" ]; then
	export PYTHONPATH=${GXY_ROOT}/third-party/VTK-8.1.2/install/lib/python2.7/site-packages:${PYTHONPATH}
	report "PYTHONPATH is now ${PYTHONPATH}"
fi
${GXY_VTI2VOL} radial-0.vti oneBall eightBalls xramp yramp zramp
if [ $? != 0 ]; then
	fail "$GXY_VTI2VOL exited with code $?"
fi

RESOLUTION="-s 512 512"
FAILS=0
for state in *.state; do
	TEST=$(echo ${state} | sed s/\.state//)
	report "Generating ${TEST} images"
	${GXY_IMAGE_WRITER} ${RESOLUTION} ${state}
	if [ $? != 0 ]; then
		fail "$GXY_IMAGE_WRITER exited with code $?"
	fi

	report "Comparing generated images with golds"
	for image in image*png; do
		GOLD=golds/$(echo ${image} | sed s/image/${TEST}/)
		report "  comparing ${image} to ${GOLD}"
		${IMAGEMAGICK_IDENTIFY} ${image} ${GOLD}
		${IMAGEMAGICK_COMPARE} ${image} $GOLD diff.png
		if [ $? == 0 ]; then
			report "  test passed: ${image} ${GOLD}"
		else
			report "  test FAILED: ${image} ${GOLD} ====="
			FAILS=$((${FAILS} + 1))
		fi
	done 
done

if [ ${FAILS} == 0 ]; then
	report "all image comparison tests passed"
else
	fail "${FAILS} image comparisons failed"
fi

report "done!"
cd ${GXY_ROOT}
exit 0
