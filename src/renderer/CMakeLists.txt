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
project (gxy_renderer VERSION ${GALAXY_VERSION})

if (COMMAND cmake_policy)
  # use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # interpret if args as vars only if unquoted
  cmake_policy(SET CMP0054 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)
endif(COMMAND cmake_policy)

find_library(PNG_LIBRARY_RELEASE png)
find_library(Z_LIBRARY_RELEASE z)

include_directories(${gxy_framework_SOURCE_DIR}
                    ${gxy_renderer_SOURCE_DIR}
                    ${gxy_renderer_BINARY_DIR}
                    ${Galaxy_SOURCE_DIR}/src/data
                    ${Galaxy_BINARY_DIR}/src/data
                    ${gxy_data_BINARY_DIR}
                    ${Galaxy_BINARY_DIR}/src
)

link_directories(${Galaxy_BINARY_DIR}/src/data 
                 ${Galaxy_BINARY_DIR}/src/framework
)

ispc_include_directories(${gxy_ispc_SOURCE_DIR} ${gxy_ispc_BINARY_DIR})

set (ISPC_SOURCES
  IspcObject.ispc
  MappedVis.ispc
  Rays.ispc
  TrianglesVis.ispc
  Visualization.ispc
  Lighting.ispc
  ParticlesVis.ispc
  PathLinesVis.ispc
  Vis.ispc
  VolumeVis.ispc
  Rays.ispc
  TransferFunction.ispc
  TraceRays.ispc
)

set (CPP_SOURCES 
  Device.cpp
  Model.cpp
  mypng.cpp
  Camera.cpp 
  IspcObject.cpp
  ImageWriter.cpp
  Lighting.cpp
  MappedVis.cpp
  GeometryVis.cpp
  ParticlesVis.cpp
  PathLinesVis.cpp
  RayQManager.cpp
  Rays.cpp 
  Renderer.cpp
  Rendering.cpp 
  RenderingEvents.cpp
  RenderingSet.cpp 
  TransferFunction.cpp
  # TraceRays.cpp
  TrianglesVis.cpp
  Vis.cpp
  Visualization.cpp
  VolumeVis.cpp
)

add_library(gxy_renderer SHARED ${CPP_SOURCES})
ispc_target_add_sources(gxy_renderer ${ISPC_SOURCES})
target_link_libraries(gxy_renderer ${VTK_LIBRARIES} gxy_data gxy_framework ${PNG_LIBRARY_RELEASE} ${Z_LIBRARY_RELEASE})
set_target_properties(gxy_renderer PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
install(TARGETS gxy_renderer DESTINATION lib)

install(FILES 
  Model.h 
  Device.h
  Camera.h 
  # ImageWriter.h 
  Lighting.h
  Pixel.h 
  RayQManager.h 
  Rays.h 
  render.h
  Renderer.h
  Rendering.h 
  RenderingSet.h 
  TraceRays.h 
  MappedVis.h
  GeometryVis.h
  ParticlesVis.h
  PathLinesVis.h
  TrianglesVis.h
  Vis.h
  VolumeVis.h
  IspcObject.h
  Visualization.h
  RenderingEvents.h
  DESTINATION include/gxy)
 
