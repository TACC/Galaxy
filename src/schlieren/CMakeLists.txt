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
project (gxy_schlieren VERSION ${GALAXY_VERSION})

include_directories_ispc(${GALAXY_INCLUDES}
                    ${OSPRAY_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS}
                    ${CMAKE_BINARY_DIR}/src
)

include_directories(${gxy_framework_SOURCE_DIR}
										${gxy_renderer_SOURCE_DIR}
										${gxy_multiserver_SOURCE_DIR}
										${gxy_schlieren_SOURCE_DIR}
                    ${gxy_ospray_SOURCE_DIR}
										${gxy_data_SOURCE_DIR}
										${gxy_data_BINARY_DIR}
                    ${Galaxy_BINARY_DIR}/src
                    ${GALAXY_INCLUDES}
                    ${OSPRAY_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS}
)

link_directories(${Galaxy_BINARY_DIR}/src/data 
                 ${Galaxy_BINARY_DIR}/src/framework)

include_directories(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})

set (ISPC_SOURCES 
  SchlierenTraceRays.ispc
)

set (CPP_SOURCES 
  Schlieren.cpp
  SchlierenTraceRays.cpp
)

if (ospray_DIR)
  ospray_create_library(gxy_schlieren ${CPP_SOURCES} ${ISPC_SOURCES} LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_data gxy_renderer gxy_framework COMPONENT lib)
  set_target_properties(gxy_schlieren PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
  install(TARGETS gxy_schlieren DESTINATION lib)
endif()

install(FILES Schlieren.h DESTINATION include)

if (GXY_WRITE_IMAGES)

  add_executable(gxyschlieren gxyschlieren.cpp)
  target_link_libraries(gxyschlieren gxy_schlieren ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
  set(BINS gxyschlieren ${BINS})

  install(TARGETS ${BINS} DESTINATION bin)

endif (GXY_WRITE_IMAGES)

