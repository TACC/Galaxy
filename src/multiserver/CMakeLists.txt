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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

include_directories(${gxy_framework_SOURCE_DIR} 
									  ${gxy_renderer_SOURCE_DIR}
									  ${gxy_sampler_SOURCE_DIR}
									  ${gxy_data_SOURCE_DIR}
									  ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES gxy_framework gxy_renderer gxy_data ${MPI_C_LIBRARIES})

set(BINS "")
set(WORKERS "")

# include_directories(${X11_INCLUDE_DIRS} ${GLUT_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR})
include_directories(${GLUT_INCLUDE_DIR})

add_executable(multiserver server.cpp MultiServer.cpp MultiServerSocket.cpp)
target_link_libraries(multiserver ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
set(BINS multiserver ${BINS})

add_executable(multiserver-viewer viewer.cpp ClientWindow.cpp MultiServerSocket.cpp IF.cpp)
target_link_libraries(multiserver-viewer ${PNG_LIBRARY_RELEASE} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES} pthread)
set(BINS multiserver-viewer ${BINS})

add_library(viewer-if MODULE viewer-if.cpp MultiServer.cpp MultiServerSocket.cpp ServerRendering.cpp IF.cpp)
target_link_libraries(viewer-if ${GALAXY_LIBRARIES})
install(TARGETS viewer-if DESTINATION lib RENAME viewer-if.so)

install(TARGETS ${BINS} DESTINATION bin)

