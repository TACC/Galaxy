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
									  ${gxy_data_SOURCE_DIR}
									  ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES gxy_framework gxy_renderer gxy_data ${MPI_C_LIBRARIES})

set(BINS "")
set(SERVERS "")

include_directories(${GLUT_INCLUDE_DIR})

add_executable(multiserver server.cpp MultiServer.cpp MultiServerSocket.cpp)
target_link_libraries(multiserver gxy_framework)
set(BINS multiserver ${BINS})

add_executable(multiserver-viewer viewer.cpp ClientWindow.cpp MultiServerSocket.cpp IF.cpp)
target_link_libraries(multiserver-viewer ${PNG_LIBRARY_RELEASE} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES} pthread)
set(BINS multiserver-viewer ${BINS})

add_library(viewerlib MODULE viewerlib.cpp MultiServer.cpp MultiServerSocket.cpp ServerRendering.cpp IF.cpp)
target_link_libraries(viewerlib ${GALAXY_LIBRARIES})
install(TARGETS viewerlib DESTINATION lib RENAME viewerlib.so)

add_executable(ping ping.cpp MultiServerSocket.cpp)
set(BINS ping ${BINS})

add_executable(data data.cpp MultiServerSocket.cpp)
set(BINS data ${BINS})

add_executable(test test.cpp MultiServerSocket.cpp)
set(BINS test ${BINS})

add_library(pinglib MODULE pinglib.cpp MultiServer.cpp MultiServerSocket.cpp)
target_link_libraries(pinglib gxy_framework)
set(SERVERS pinglib ${SERVERS})

add_library(datalib MODULE datalib.cpp MultiServer.cpp MultiServerSocket.cpp)
target_link_libraries(datalib gxy_data gxy_framework)
set(SERVERS datalib ${SERVERS})

add_library(testlib MODULE testlib.cpp MultiServer.cpp MultiServerSocket.cpp)
target_link_libraries(testlib gxy_data gxy_framework)
set(SERVERS testlib ${SERVERS})

install(TARGETS ${BINS} DESTINATION bin)
install(TARGETS ${SERVERS} DESTINATION lib)


