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

cmake_minimum_required (VERSION 3.9)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

include_directories(${gxy_framework_SOURCE_DIR} 
									  ${gxy_data_SOURCE_DIR}
									  ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES gxy_framework gxy_renderer gxy_data ${MPI_C_LIBRARIES})

set(BINS "")
set(SERVERS "")

include_directories(${GLUT_INCLUDE_DIR})

add_library(gxy_multiserver SHARED MultiServer.cpp MultiServerSocket.cpp)
target_link_libraries(gxy_multiserver gxy_framework)
install(TARGETS gxy_multiserver DESTINATION lib)

add_executable(multiserver server.cpp)
target_link_libraries(multiserver gxy_multiserver gxy_data gxy_framework)
set(BINS multiserver ${BINS})

add_executable(multiserver-viewer viewer.cpp ClientWindow.cpp IF.cpp)
target_link_libraries(multiserver-viewer gxy_multiserver ${PNG_LIBRARY_RELEASE} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES} ${OSPRAY_LIBRARIES} pthread)
set(BINS multiserver-viewer ${BINS})

add_library(gxy_module_viewer MODULE viewerlib.cpp ServerRendering.cpp IF.cpp)
target_link_libraries(gxy_module_viewer gxy_multiserver ${GALAXY_LIBRARIES})
install(TARGETS gxy_module_viewer DESTINATION lib RENAME viewerlib.so)

add_executable(ping ping.cpp)
target_link_libraries(ping gxy_multiserver)
set(BINS ping ${BINS})

add_executable(dataloader dataloader.cpp)
target_link_libraries(dataloader gxy_multiserver gxy_framework)
set(BINS dataloader ${BINS})

add_executable(multiserver_test test.cpp)
target_link_libraries(multiserver_test gxy_multiserver)
set(BINS multiserver_test ${BINS})

add_library(gxy_module_ping MODULE pinglib.cpp)
target_link_libraries(gxy_module_ping gxy_multiserver gxy_framework)
set(SERVERS gxy_module_ping ${SERVERS})

add_library(gxy_module_dataloader MODULE dataloaderlib.cpp)
target_link_libraries(gxy_module_dataloader gxy_multiserver gxy_data gxy_framework)
set(SERVERS gxy_module_dataloader ${SERVERS})

add_library(gxy_module_test MODULE testlib.cpp)
target_link_libraries(gxy_module_test gxy_multiserver gxy_data gxy_framework)
set(SERVERS gxy_module_test ${SERVERS})

install(TARGETS ${BINS} DESTINATION bin)
install(TARGETS ${SERVERS} DESTINATION lib)

install(FILES
  MultiServer.h
  MultiServerSocket.h
  DESTINATION include/gxy)


