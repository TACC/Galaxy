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

cmake_minimum_required(VERSION 2.8)
project (gxy_data)

find_library(PNG_LIBRARY_RELEASE png)

include_directories(${gxy_framework_SOURCE_DIR}
                    ${gxy_renderer_SOURCE_DIR}
                    ${gxy_data_SOURCE_DIR}
                    ${Galaxy_BINARY_DIR}/src)

set (CPP_SOURCES     
  Box.cpp 
  DataObjects.cpp
  Datasets.cpp
  Geometry.cpp 
  ISPCObject.cpp
  KeyedDataObject.cpp
  Lighting.cpp
  MappedVis.cpp
  OSPRayObject.cpp
  OSPUtil.cpp
  Particles.cpp 
  ParticlesVis.cpp
  Triangles.cpp 
  TrianglesVis.cpp
  Vis.cpp
  Visualization.cpp
  Volume.cpp 
  AmrVolume.cpp
  VolumeVis.cpp)

set (ISPC_SOURCES   
  ISPCObject.ispc
  Lighting.ispc
  MappedVis.ispc
  ParticlesVis.ispc
  TrianglesVis.ispc
  Vis.ispc
  Visualization.ispc
  VolumeVis.ispc)

if (ospray_DIR)
  ospray_create_library(gxy_data ${CPP_SOURCES} ${ISPC_SOURCES} 
    LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_framework ${PNG_LIBRARY_RELEASE} 
    COMPONENT lib)
  set_target_properties(gxy_data PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
  install(TARGETS gxy_data DESTINATION lib)
endif()

install(FILES 
  Box.h
  data.h
  DataObjects.h
  Datasets.h
  Geometry.h
  ImageWriter.h
  ISPCObject.h
  KeyedDataObject.h
  Lighting.h
  MappedVis.h
  OSPRayObject.h
  OSPUtil.h
  Particles.h
  ParticlesVis.h
  Triangles.h
  TrianglesVis.h
  Vis.h
  Visualization.h
  Volume.h
  AmrVolume.h
  VolumeVis.h
  DESTINATION include/gxy)


