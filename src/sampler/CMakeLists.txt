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

cmake_minimum_required (VERSION 2.8.11)
cmake_policy(SET CMP0048 NEW)
project (gxy_sampler VERSION ${GALAXY_VERSION})

include_directories(${gxy_framework_SOURCE_DIR}
										${gxy_renderer_SOURCE_DIR}
										${gxy_sampler_SOURCE_DIR}
										${gxy_data_SOURCE_DIR}
										${gxy_data_BINARY_DIR}
                    ${Galaxy_BINARY_DIR}/src)

link_directories(${Galaxy_BINARY_DIR}/src/data 
                 ${Galaxy_BINARY_DIR}/src/framework)

set (ISPC_SOURCES 
)

set (CPP_SOURCES 
  Sampler.cpp)

if (ospray_DIR)
  ospray_create_library(gxy_sampler ${CPP_SOURCES} ${ISPC_SOURCES} 
    LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_data gxy_renderer gxy_framework COMPONENT lib)
  set_target_properties(gxy_sampler PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
  install(TARGETS gxy_sampler DESTINATION lib)
endif()

install(FILES 
  Sampler.h 
  DESTINATION include)
 
