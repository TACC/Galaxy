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


ispc_include_directories(${EMBREE_INCLUDE_DIRS}
                         ${OSPRAY_INCLUDE_DIRS}
                         )

include_directories(${Galaxy_BINARY_DIR}/src
                    ${Galaxy_SOURCE_DIR}/src/framework
                    ${Galaxy_SOURCE_DIR}/src/data
                    ${Galaxy_SOURCE_DIR}/src/ospray
                    ${OSPRAY_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS})

set (ISPC_SOURCES 
  DataDrivenPathLines.ispc 
  DataDrivenSpheres.ispc 
  DataDrivenTriangleMesh.ispc
  )

set (CPP_SOURCES 
  force_ospray_library_load.cpp
  DataDrivenPathLines.cpp
  DataDrivenSpheres.cpp
  DataDrivenTriangleMesh.cpp
  OsprayHandle.cpp
  OsprayObject.cpp
  OsprayTriangles.cpp
  OsprayParticles.cpp
  OsprayPathLines.cpp
  OsprayVolume.cpp
  OsprayUtil.cpp
  )

install(FILES
  OsprayHandle.h
  OsprayObject.h
  OsprayUtil.h
  DESTINATION include/gxy)

if (ospray_DIR)
  add_library(gxy_ospray SHARED ${CPP_SOURCES})
  ispc_target_add_sources(gxy_ospray ${ISPC_SOURCES})
  target_link_libraries(gxy_ospray gxy_framework ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES})
  set_target_properties(gxy_ospray PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
  install(TARGETS gxy_ospray DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

