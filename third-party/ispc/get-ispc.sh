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

VERSION="1.10.0"

if [ "x$1" != "x" ]; then
	echo "usage: get-ispc.sh"
	echo "get-ispc will auto-detect the OS and download the appropriate ispc version ${VERSION}"
	exit 1
fi

function fail
{
	echo "ERROR: $1"
	echo "Try downloading ispc manually at https://ispc.github.io/downloads.html"
	exit 1
}

OS_TYPE=$(uname)
if [ "x${OS_TYPE}" == "xLinux" ]; then
	TARGET_OS="linux"
	TARGET_OS_DIR="Linux"
elif [ "x${OS_TYPE}" == "xDarwin" ]; then
	TARGET_OS="osx"
	TARGET_OS_DIR="Darwin"
else
	fail "Unrecognized OS type '${OS_TYPE}'"
fi

TARGET_DIR="ispc-${VERSION}-${TARGET_OS_DIR}"
TARBALL="ispc-v${VERSION}-${TARGET_OS}.tar.gz"

if [ -x $TARGET_DIR/bin/ispc ]; then
	echo "ispc for ${OS_TYPE} already exists. Nothing more to do."
	exit 0
fi

if [ -f ${TARBALL} ]; then
	echo "found ${TARBALL}"
else 
	echo "downloading ispc ${VERSION} for ${OS_TYPE}"
	wget -q -O ${TARBALL} http://sourceforge.net/projects/ispcmirror/files/v${VERSION}/${TARBALL}/download
	if [ $? != 0 ]; then
		fail "Download for ${TARBALL} failed."
	fi
fi

if [ -f ${TARBALL} ]; then
	echo "untarring ${TARBALL}"
	tar xf ${TARBALL}
else
	fail "Could not find ${TARBALL}"
fi

if [ -x ${TARGET_DIR}/bin/ispc ]; then
	echo "ispc for ${OS_TYPE} successfully retrieved!"
	rm ${TARBALL}
else
	fail "Executable ${TARGET_DIR}/bin/ispc not found"
fi

exit 0
