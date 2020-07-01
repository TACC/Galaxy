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

include_directories(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
ispc_include_directories(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/src)

include_directories(${gxy_framework_SOURCE_DIR}
										${gxy_renderer_SOURCE_DIR}
										${gxy_data_SOURCE_DIR}
										${gxy_ospray_SOURCE_DIR}
										${gxy_data_BINARY_DIR}
                    ${Galaxy_BINARY_DIR}/src)

link_directories(${Galaxy_BINARY_DIR}/src/ospray
                 ${Galaxy_BINARY_DIR}/src/data 
                 ${Galaxy_BINARY_DIR}/src/framework)

set (ISPC_SOURCES 
  IspcObject.ispc
 	MappedVis.ispc
	Rays.ispc
	TrianglesVis.ispc
  Visualization.ispc
  Lighting.ispc
	ParticlesVis.ispc
	PathLinesVis.ispc
  TraceRays.ispc
	Vis.ispc
	VolumeVis.ispc
  Rays.ispc
  TraceRays.ispc)

set (CPP_SOURCES 
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
  TraceRays.cpp
  TrianglesVis.cpp
  Vis.cpp
  Visualization.cpp
  VolumeVis.cpp)

if (ospray_DIR)
  add_library(gxy_renderer SHARED ${CPP_SOURCES})
  ispc_target_add_sources(gxy_renderer ${ISPC_SOURCES})
  target_link_libraries(gxy_renderer ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_data gxy_framework gxy_ospray ${PNG_LIBRARY_RELEASE} ${Z_LIBRARY_RELEASE})
  set_target_properties(gxy_renderer PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
  install(TARGETS gxy_renderer DESTINATION lib)
endif()

install(FILES 
  Camera.h 
  ImageWriter.h 
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
 
