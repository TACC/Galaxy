## ========================================================================== ##
## Copyright (c) 2014-2020 The University of Texas at Austin.                 ##
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
FROM pnav/galaxy-base:v0.3.0

USER galaxy:galaxy
ENV GXY_ROOT /opt/local/galaxy
RUN echo "GALAXY: cloning Galaxy git repo"
RUN git clone https://github.com/TACC/Galaxy.git galaxy 

RUN echo "GALAXY: prepping Galaxy prerequisites"
WORKDIR ${GXY_ROOT}
RUN scripts/prep-third-party.sh

RUN echo "GALAXY: building Intel Embree"
RUN mkdir -p ${GXY_ROOT}/third-party/embree/build
WORKDIR ${GXY_ROOT}/third-party/embree/build
RUN cmake -D EMBREE_TBB_ROOT:PATH=/opt/local/tbb \
					-D TBB_INCLUDE_DIR:PATH=/opt/local/tbb/include \
					-D TBB_LIBRARY:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb.so \
	 				-D TBB_LIBRARY_MALLOC:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb_malloc.so \
	 				-D CMAKE_INSTALL_PREFIX=${GXY_ROOT}/third-party/install \
					-D CMAKE_C_FLAGS:STRING="-Wno-undef -Wno-deprecated-declarations" \
					-D CMAKE_CXX_FLAGS:STRING="-Wno-undef -Wno-deprecated-declarations" \
					-D EMBREE_ISPC_EXECUTABLE=${GXY_ROOT}/third-party/install/bin/ispc \
					-D EMBREE_STATIC_LIB=ON \
					-D EMBREE_TUTORIALS=OFF \
					-D EMBREE_ISA_SSE2=OFF \
					.. && make -j 2 install

RUN echo "GALAXY: building Intel OSPRay"
RUN mkdir -p ${GXY_ROOT}/third-party/ospray/build
WORKDIR ${GXY_ROOT}/third-party/ospray/build
RUN cmake -D TBB_INCLUDE_DIR:PATH=/opt/local/tbb/include \
					-D TBB_LIBRARY:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb.so \
					-D TBB_LIBRARY_DEBUG:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb_debug.so \
					-D TBB_LIBRARY_MALLOC:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbbmalloc.so \
					-D TBB_LIBRARY_MALLOC_DEBUG:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbbmalloc_debug.so \
					-D TBB_ROOT=/opt/local/tbb \
	 				-D CMAKE_INSTALL_PREFIX=${GXY_ROOT}/third-party/install \
					-D CMAKE_C_FLAGS:STRING="-I${GXY_ROOT}/third-party/install/include -Wno-undef -Wno-deprecated-declarations" \
					-D CMAKE_CXX_FLAGS:STRING="-I${GXY_ROOT}/third-party/install/include -Wno-undef -Wno-deprecated-declarations" \
					-D embree_DIR=${GXY_ROOT}/third-party/install/lib/cmake/embree-3.6.1 \
					-D EMBREE_ISPC_EXECUTABLE=${GXY_ROOT}/third-party/install/bin/ispc \
					.. && make -j 2 install

RUN echo "GALAXY: building RapidJSON"
RUN mkdir -p ${GXY_ROOT}/third-party/rapidjson/build
WORKDIR ${GXY_ROOT}/third-party/rapidjson/build
RUN cmake -D CMAKE_INSTALL_PREFIX=${GXY_ROOT}/third-party/install \
					-D RAPIDJSON_BUILD_DOC=OFF \
					-D RAPIDJSON_BUILD_EXAMPLES=OFF \
					-D RAPIDJSON_BUILD_TESTS=OFF \
					.. && make install

RUN echo "GALAXY: done building prerequisites"
RUN mkdir -p ${GXY_ROOT}/.galaxy
RUN date > ${GXY_ROOT}/.galaxy/gxy_third_party_installed

RUN echo "GALAXY: building Galaxy asynchronous viewer"
RUN mkdir -p ${GXY_ROOT}/build
WORKDIR ${GXY_ROOT}/build
RUN cmake -D VTK_DIR:PATH=/usr/local/lib/cmake/vtk-8.1 \
					-D TBB_ROOT:PATH=/opt/local/tbb \
					-D TBB_INCLUDE_DIR:PATH=/opt/local/tbb/include \
					-D TBB_INCLUDE_DIRS:PATH=/opt/local/tbb/include \
					-D TBB_LIBRARY:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb.so \
					-D TBB_LIBRARY_DEBUG:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb_debug.so \
					-D TBB_LIBRARY_MALLOC:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbbmalloc.so \
					-D TBB_LIBRARY_MALLOC_DEBUG:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbbmalloc_debug.so \
					-D TBB_tbb_LIBRARY_DEBUG:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb_debug.so \
					-D TBB_tbb_LIBRARY_RELEASE:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb.so \
					-D TBB_tbb_preview_LIBRARY_DEBUG:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb_preview_debug.so \
					-D TBB_tbb_preview_LIBRARY_RELEASE:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbb_preview.so \
					-D TBB_tbbmalloc_LIBRARY_DEBUG:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbbmalloc_debug.so \
					-D TBB_tbbmalloc_LIBRARY_RELEASE:FILEPATH=/opt/local/tbb/lib/intel64/gcc4.7/libtbbmalloc.so \
					-D GLUT_INCLUDE_DIR:PATH=/usr/include \
					-D GLUT_glut_LIBRARY:FILEPATH=/usr/lib64/libglut.so \
					-D CMAKE_VERBOSE_MAKEFILE:BOOL=ON \
					.. && make install
RUN echo "GALAXY: build done!"

RUN echo "GALAXY: building Galaxy image writer"
WORKDIR ${GXY_ROOT}/build
RUN cmake -D GXY_WRITE_IMAGES:BOOL=ON . && make install
RUN echo "GALAXY: build done!"

RUN echo "GALAXY: running image tests"
WORKDIR ${GXY_ROOT}
RUN tests/image-gold-tests.sh
RUN echo "GALAXY: image tests done!"

# RUN echo "GXY: running unit tests"



