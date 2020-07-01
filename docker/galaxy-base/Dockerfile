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
# Use the CentOS version deployed on TACC systems
FROM centos:7.6.1810
	RUN yum -y install which
	RUN yum -y install sudo
	RUN yum -y install wget
	RUN yum -y install git
	RUN yum -y install vim
	RUN yum -y install python-devel
	RUN yum -y install numpy
	RUN yum -y install ncurses-devel
	RUN yum -y install libXrandr-devel
	RUN yum -y install libXinerama-devel
	RUN yum -y install libXcursor-devel
	RUN yum -y install libXt-devel
	RUN yum -y install centos-release-scl
	RUN yum -y install devtoolset-7
	RUN yum -y install mvapich2-2.2-devel
	RUN yum -y install tbb-devel
	RUN yum -y install boost-devel
	RUN yum -y install freeglut-devel
	RUN yum -y install zlib-devel
	RUN yum -y install libpng-devel

# set devtoolset-7 environment
ENV PCP_DIR=/opt/rh/devtoolset-7/root
ENV PATH=${PCP_DIR}/usr/bin:${PATH}:/usr/lib64/mvapich2-2.2/bin
ENV MANPATH=${PCP_DIR}/usr/share/man:${MANPATH}:/usr/share/man/mvapich2-2.2-x86_64
ENV PERL5LIB=${PCP_DIR}/usr/lib64/perl5/vendor_perl:${PCP_DIR}/usr/lib/perl5:${PCP_DIR}/usr/share/perl5/vendor_perl:${PERL5LIB}
ENV X_SCLS=devtoolset-7 
ENV LD_LIBRARY_PATH=${PCP_DIR}/usr/lib64:${PCP_DIR}/usr/lib:${PCP_DIR}/usr/lib64/dyninst:${PCP_DIR}/usr/lib/dyninst:${PCP_DIR}/usr/lib64:${PCP_DIR}/usr/lib:${LD_LIBRARY_PATH}:/usr/lib64/mvapich2-2.2/lib
ENV PYTHONPATH=${PCP_DIR}/usr/lib64/python2.7/site-packages:${PCP_DIR}/usr/lib/python2.7/site-packages:${PYTHONPATH}
ENV INFOPATH=${PCP_DIR}/usr/share/info:${INFOPATH}

# create 'galaxy' user and give password-less sudo
RUN useradd galaxy
RUN echo "galaxy ALL=(root) NOPASSWD:ALL " > /etc/sudoers.d/galaxy \
		&& chmod 0440 /etc/sudoers.d/galaxy

# install a recent cmake
WORKDIR /opt/local
RUN wget https://github.com/Kitware/CMake/releases/download/v3.14.5/cmake-3.14.5.tar.gz \
		&& tar xf cmake-3.14.5.tar.gz
WORKDIR /opt/local/cmake-3.14.5
RUN ./configure && make install
WORKDIR /opt/local
RUN rm -rf cmake-3.14.5 cmake-3.14.5.tar.gz

# install a recent VTK
WORKDIR /opt/local
RUN wget https://www.vtk.org/files/release/8.1/VTK-8.1.2.tar.gz \
		&& tar xf VTK-8.1.2.tar.gz
WORKDIR /opt/local/VTK-8.1.2
RUN cmake -D CMAKE_BUILD_TYPE:STRING=MinSizeRel \
					-D CMAKE_C_FLAGS:STRING="-Wno-deprecated-register" \
					-D CMAKE_CXX_FLAGS:STRING="-Wno-deprecated-register" \
					-D VTK_WRAP_PYTHON:BOOL=ON \
					-D PYHON_LIBRARY:FILEPATH=/usr/lib64/libpython2.7.so \
					-D PYTHON_INCLUDE_DIR:PATH=/usr/include/python2.7 \
					. \
		&& make -j 4 install
WORKDIR /opt/local
RUN rm -rf VTK-8.1.2 VTK-8.1.2.tar.gz
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib64:/usr/local/lib
ENV PYTHONPATH=${PYTHONPATH}:/usr/local/lib/python2.7/site-packages

# install a recent TBB
WORKDIR /opt/local
RUN wget https://github.com/intel/tbb/releases/download/2019_U8/tbb2019_20190605oss_lin.tgz \
		&& tar xf tbb2019_20190605oss_lin.tgz
RUN mv tbb2019_20190605oss tbb
RUN mv pstl2019_20190605oss pstl
RUN rm -f tbb2019_20190605oss_lin.tgz
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/opt/local/tbb/lib/intel64/gcc4.7:/opt/local/tbb/lib/intel64/gcc4.4:/opt/local/tbb/lib/intel64/gcc4.1

# install perceptualdiff
WORKDIR /opt/local/pdiff
RUN wget -O perceptualdiff-1.1.1-linux.tar.gz \
			https://sourceforge.net/projects/pdiff/files/pdiff/perceptualdiff-1.1.1/perceptualdiff-1.1.1-linux.tar.gz/download \
		&& tar xf perceptualdiff-1.1.1-linux.tar.gz
RUN chown root:root perceptualdiff
RUN chmod a+rx perceptualdiff
RUN mv perceptualdiff /usr/bin
WORKDIR /opt/local
RUN rm -rf pdiff

WORKDIR /opt/local
RUN mkdir galaxy
RUN chown galaxy:galaxy galaxy
USER galaxy


