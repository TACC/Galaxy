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
project (gxy_intel VERSION ${GALAXY_VERSION})

if (COMMAND cmake_policy)
  # use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # interpret if args as vars only if unquoted
  cmake_policy(SET CMP0054 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)
endif(COMMAND cmake_policy)

ispc_include_directories(
    ${EMBREE_INCLUDE_DIRS} 
    ${Galaxy_BINARY_DIR}/src/intel
    ${Galaxy_SOURCE_DIR}/src/ispc
    ${Galaxy_SOURCE_DIR}/src/renderer
)

include_directories( 
    ${Galaxy_BINARY_DIR}/src 
    ${Galaxy_BINARY_DIR}/src/renderer
    ${Galaxy_SOURCE_DIR}/src/framework
    ${Galaxy_SOURCE_DIR}/src/data
    ${Galaxy_SOURCE_DIR}/src/renderer
    ${Galaxy_SOURCE_DIR}/src/ospray
    ${EMBREE_INCLUDE_DIRS}
    ${openvkl_DIR}/../../../include
)

set(GALAXY_LIBRARIES gxy_framework gxy_data gxy_renderer gxy_intel ${EMBREE_LIBRARIES} ${MPI_C_LIBRARIES})

set (ISPC_SOURCES 
    EmbreeGeometry.ispc
    EmbreeSpheres.ispc
    EmbreePathLines.ispc
    EmbreeModel.ispc
    )

set (CPP_SOURCES 
  Embree.cpp
  EmbreeGeometry.cpp
  EmbreeModel.cpp
  EmbreeTriangles.cpp
  EmbreeSpheres.cpp
  EmbreePathLines.cpp
  )

install(FILES
  Embree.h
  EmbreeGeometry.h
  EmbreeModel.h
  EmbreeTriangles.h
  DESTINATION include/gxy)

add_library(gxy_intel SHARED ${CPP_SOURCES})
ispc_target_add_sources(gxy_intel ${ISPC_SOURCES})
target_link_libraries(gxy_intel gxy_framework ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES})
set_target_properties(gxy_intel PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
install(TARGETS gxy_intel DESTINATION lib)

add_executable(em_test em_test.cpp)
target_link_libraries(em_test ${GALAXY_LIBRARIES})
install(TARGETS em_test DESTINATION tests)
