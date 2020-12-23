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


ispc_include_directories(${Galaxy_BINARY_DIR}/src/ispc ${EMBREE_INCLUDE_DIRS})

include_directories(${Galaxy_BINARY_DIR}/src
                    ${Galaxy_SOURCE_DIR}/src/framework
                    ${Galaxy_SOURCE_DIR}/src/ispc
                    ${EMBREE_INCLUDE_DIRS})

set(GALAXY_LIBRARIES gxy_framework gxy_ispc)

set (ISPC_SOURCES TransferFunction.ispc )

set (CPP_SOURCES TransferFunction.cpp)

install(FILES
  TransferFunction.h
  DESTINATION include/gxy)

if (ospray_DIR)
  add_library(gxy_ispc SHARED ${CPP_SOURCES})
  ispc_target_add_sources(gxy_ispc ${ISPC_SOURCES})
  target_link_libraries(gxy_ispc gxy_framework ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES})
  set_target_properties(gxy_ispc PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
  install(TARGETS gxy_ispc DESTINATION lib)
endif()

add_executable(tf_test tf_test.cpp)
target_link_libraries(tf_test ${GALAXY_LIBRARIES})
install(TARGETS tf_test DESTINATION tests)
