#!/bin/bash

if [ "x$1" == "xlinux" ]; then
	TARGET_OS="linux"
elif [ "x$1" == "xosx" ]; then
	TARGET_OS="osx"
else
	echo "usage: get-ispc.sh [ linux | osx ]"
	exit 1
fi

VERSION="1.9.2"
TARGET_DIR="ispc-v${VERSION}-${TARGET_OS}"
TARBALL="${TARGET_DIR}.tar.gz"

if [ -x $TARGET_DIR/ispc ]; then
	echo "ispc for ${TARGET_OS} already exists. Nothing more to do."
	exit 1
fi

if [ -f ${TARBALL} ]; then
	echo "found ${TARBALL}"
else 
	echo "downloading ispc ${VERSION} for ${OS}"
	wget -O ${TARBALL} http://sourceforge.net/projects/ispcmirror/files/v${VERSION}/${TARBALL}/download
fi

if [ -f ${TARBALL} ]; then
	echo "untarring ${TARBALL}"
	tar xf ${TARBALL}
else
	echo "could not find ${TARBALL}"
	echo "bailing out...."
	exit 1
fi

if [ -x ${TARGET_DIR}/ispc ]; then
	echo "ispc for ${TARGET_OS} successfully retrieved!"
	rm ${TARBALL}
else
	echo "executable ${TARGET_DIR}/ispc not found"
	echo "bailing out...."
	exit 1
fi

exit 0
