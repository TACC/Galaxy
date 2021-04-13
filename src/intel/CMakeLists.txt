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

find_package(embree 3.0.0 REQUIRED)
find_package(openvkl 0.11.0)

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
    # message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

ispc_include_directories(
    ${Galaxy_BINARY_DIR}/src/intel
    ${Galaxy_BINARY_DIR}/src/renderer
    ${Galaxy_SOURCE_DIR}/src/renderer
    ${openvkl_DIR}/../../../include
    ${embree_DIR}/include
)

include_directories( 
    ${Galaxy_BINARY_DIR}/src 
    ${Galaxy_BINARY_DIR}/src/renderer
    ${Galaxy_SOURCE_DIR}/src/framework
    ${Galaxy_SOURCE_DIR}/src/data
    ${Galaxy_SOURCE_DIR}/src/renderer
    ${Galaxy_SOURCE_DIR}/src/ospray
    ${openvkl_DIR}/../../../include
    ${embree_DIR}/include
)

set(GALAXY_LIBRARIES gxy_framework gxy_data gxy_renderer gxy_intel ${EMBREE_LIBRARIES} ${MPI_C_LIBRARIES})

set (ISPC_SOURCES 
    IntelModel.ispc
    IntelData.ispc
    EmbreeGeometry.ispc
    EmbreeSpheres.ispc
    EmbreePathLines.ispc
    VklVolume.ispc
    test.ispc
    )

set (CPP_SOURCES 
  IntelDevice.cpp
  IntelModel.cpp
  IntelData.cpp
  EmbreeGeometry.cpp
  EmbreeTriangles.cpp
  EmbreeSpheres.cpp
  EmbreePathLines.cpp
  VklVolume.cpp
  )

install(FILES
  IntelDevice.h
  IntelModel.h
  IntelData.h
  EmbreeGeometry.h
  EmbreeTriangles.h
  DESTINATION include/gxy)

add_library(gxy_intel SHARED ${CPP_SOURCES})
ispc_target_add_sources(gxy_intel ${ISPC_SOURCES})
target_link_libraries(gxy_intel gxy_framework ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES})
set_target_properties(gxy_intel PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
install(TARGETS gxy_intel DESTINATION lib)

add_executable(em_test em_test.cpp)
target_link_libraries(em_test gxy_framework gxy_data gxy_renderer gxy_intel embree openvkl::openvkl_module_ispc_driver ${MPI_C_LIBRARIES})
install(TARGETS em_test DESTINATION tests)

add_executable(vkl_test vkl_test.cpp)
target_link_libraries(vkl_test gxy_framework gxy_data gxy_renderer gxy_intel embree openvkl::openvkl_module_ispc_driver ${MPI_C_LIBRARIES})
install(TARGETS vkl_test DESTINATION tests)
