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
project (gxy_sampler VERSION ${GALAXY_VERSION})

include_directories(${gxy_framework_SOURCE_DIR}
										${gxy_renderer_SOURCE_DIR}
										${gxy_multiserver_SOURCE_DIR}
										${gxy_sampler_SOURCE_DIR}
										${gxy_data_SOURCE_DIR}
										${gxy_ospray_SOURCE_DIR}
										${gxy_data_BINARY_DIR}
										${gxy_renderer_BINARY_DIR}
                    ${Galaxy_BINARY_DIR}/src)

link_directories(${Galaxy_BINARY_DIR}/src/data 
                 ${Galaxy_BINARY_DIR}/src/framework)

include_directories(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
ispc_include_directories(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/src)

set (ISPC_SOURCES 
 SamplerTraceRays.ispc
 SamplerVis.ispc
 IsoSamplerVis.ispc
 GradientSamplerVis.ispc
)

set (CPP_SOURCES 
  Sampler.cpp
  SamplerTraceRays.cpp
  SamplerVis.cpp
  IsoSamplerVis.cpp
  GradientSamplerVis.cpp
  )

set(SERVERS "")

if (ospray_DIR)
  add_library(gxy_sampler SHARED ${CPP_SOURCES})
  ispc_target_add_sources(gxy_sampler ${ISPC_SOURCES})
  target_link_libraries(gxy_sampler ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_data gxy_renderer gxy_framework)
  set_target_properties(gxy_sampler PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
  install(TARGETS gxy_sampler DESTINATION lib)
endif()

add_library(gxy_module_mhsampler MODULE MHSampleClientServer)
target_link_libraries(gxy_module_mhsampler gxy_multiserver gxy_data gxy_framework)
set(SERVERS gxy_module_mhsampler ${SERVERS})

add_library(gxy_module_densitysampler MODULE DensitySampleClientServer)
target_link_libraries(gxy_module_densitysampler gxy_multiserver gxy_data gxy_framework)
set(SERVERS gxy_module_densitysampler ${SERVERS})

add_library(gxy_module_interpolator MODULE InterpolatorClientServer)
target_link_libraries(gxy_module_interpolator gxy_multiserver gxy_data gxy_framework)
set(SERVERS gxy_module_interpolator ${SERVERS})

install(TARGETS ${BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${SERVERS} DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES 
  Sampler.h 
  SamplerTraceRays.h 
  DESTINATION include)

install(TARGETS ${SERVERS} DESTINATION ${CMAKE_INSTALL_LIBDIR})
 
