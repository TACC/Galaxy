## ========================================================================== ##
## Copyright (c) 2014-2018 The University of Texas at Austin.                 ##
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
project (gxy_renderer)

include_directories(${gxy_framework_SOURCE_DIR}
										${gxy_renderer_SOURCE_DIR}
										${gxy_data_SOURCE_DIR}
										${gxy_data_BINARY_DIR}
                    ${Galaxy_BINARY_DIR}/src)

link_directories(${Galaxy_BINARY_DIR}/src/data 
                 ${Galaxy_BINARY_DIR}/src/framework)

set (ISPC_SOURCES 
  Rays.ispc
  TraceRays.ispc)


set (CPP_SOURCES 
  Camera.cpp 
  RayQManager.cpp
  Rays.cpp 
  Renderer.cpp
  Rendering.cpp 
  RenderingSet.cpp 
  TraceRays.cpp)

if (ospray_DIR)
  ospray_create_library(gxy_renderer ${CPP_SOURCES} ${ISPC_SOURCES} 
    LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_data gxy_framework COMPONENT lib)
  install(TARGETS gxy_renderer DESTINATION lib)
endif()

install(FILES 
  Camera.h 
  dtypes.h  
  Pixel.h 
  RayQManager.h 
  Rays.h 
  Renderer.h
  Rendering.h 
  RenderingSet.h 
  TraceRays.h 
  DESTINATION include/gxy)
 
