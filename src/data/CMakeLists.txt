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

cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0048 NEW)
project (gxy_data VERSION ${GALAXY_VERSION})

if (COMMAND cmake_policy)
  # use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # interpret if args as vars only if unquoted
  cmake_policy(SET CMP0054 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)

endif(COMMAND cmake_policy)

include_directories(${GALAXY_INCLUDES}
                    ${Galaxy_SOURCE_DIR}/src/framework
                    ${Galaxy_SOURCE_DIR}/src/ospray
                    ${Galaxy_SOURCE_DIR}/src/data
                    ${Galaxy_BINARY_DIR}/src
                    ${OSPRAY_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS})

set (CPP_SOURCES     
  data.cpp 
  DataObjects.cpp
  Datasets.cpp
  Geometry.cpp 
  KeyedDataObject.cpp
  Particles.cpp 
  PathLines.cpp 
  Triangles.cpp 
  Volume.cpp 
  AmrVolume.cpp)

add_library(gxy_data SHARED ${CPP_SOURCES})
target_link_libraries(gxy_data ${VTK_LIBRARIES} gxy_framework gxy_ospray)
set_target_properties(gxy_data PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
install(TARGETS gxy_data DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES 
  dtypes.h
  data.h
  DataObjects.h
  Datasets.h
  Geometry.h
  KeyedDataObject.h
  Particles.h
  PathLines.h
  Triangles.h
  Volume.h
  AmrVolume.h
  DESTINATION include/gxy)


