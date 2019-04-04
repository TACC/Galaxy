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

cmake_minimum_required (VERSION 3.9)
cmake_policy(SET CMP0048 NEW)
project (gxy_ospray VERSION ${GALAXY_VERSION})

if (COMMAND cmake_policy)
  # use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # interpret if args as vars only if unquoted
  cmake_policy(SET CMP0054 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)
endif(COMMAND cmake_policy)


set(OSPRAY_SOURCEDIR ${Galaxy_SOURCE_DIR}/third-party/ospray CACHE STRING "root of OSPRAY source tree")
include_directories_ispc(${EMBREE_INCLUDE_DIRS} 
                         ${OSPRAY_INCLUDE_DIR}/ospray
                         ${OSPRAY_SOURCEDIR}
                         ${OSPRAY_SOURCEDIR}/ospray
                         ${OSPRAY_SOURCEDIR}/ospray/include)

include_directories(${OSPRAY_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS}
                    ${OSPRAY_SOURCEDIR}/install/include/ospray/SDK/geometry)

set (ISPC_SOURCES DataDrivenPathLines.ispc DataDrivenSpheres.ispc)
set (CPP_SOURCES DataDrivenPathLines.cpp DataDrivenSpheres.cpp)

if (ospray_DIR)
  ospray_create_library(gxy_ospray ${CPP_SOURCES} ${ISPC_SOURCES} 
    LINK ${OSPRAY_LIBRARIES} COMPONENT lib)
  set_target_properties(gxy_ospray PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
endif()

