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


if [ "x$1" != "x" ]; then
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

GXY_ROOT=..
GXY_BIN=${GXY_ROOT}/install/bin
GXY_RADIAL_BIN=${GXY_BIN}/radial
GXY_IMAGE_BIN=${GXY_BIN}/vis

if [ ! -x ${GXY_RADIAL_BIN} ]; then
	fail "Could not find or execute the Galaxy radial generator '${GXY_RADIAL_BIN}'"
fi

if [ ! -x ${GXY_IMAGE_BIN} ]; then
	fail "Could not find or execute the Galaxy image writer '${GXY_IMAGE_BIN}'. Ensure Galaxy was configured with -D GXY_WRITE_IMAGES=ON"
fi

report "Generating radial-0.vti with ${GXY_RADIAL_BIN}"
${GXY_RADIAL_BIN}

if [ $? != 0 ]; then
	fail "$GXY_RADIAL_BIN exited with code $?"
fi

