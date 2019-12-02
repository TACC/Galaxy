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
project (gxy_multiserver VERSION ${GALAXY_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

include_directories(${gxy_framework_SOURCE_DIR} 
									  ${gxy_data_SOURCE_DIR}
									  ${gxy_ospray_SOURCE_DIR}
									  ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES gxy_framework gxy_renderer gxy_data gxy_ospray ${MPI_C_LIBRARIES})

set(BINS "")
set(SERVERS "")
set(LIBS "")

include_directories(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
include_directories(${GLUT_INCLUDE_DIR})

add_library(gxy_multiserver SHARED MultiServer.cpp ServerRendering.cpp JsonInterface.cpp MultiServerHandler.cpp ServerClientConnection.hpp SocketHandler.cpp DynamicLibraryManager.cpp)
target_link_libraries(gxy_multiserver gxy_data gxy_framework gxy_renderer)
set(LIBS gxy_multiserver ${LIBS})

add_library(gxy_multiserver_client SHARED ClientWindow.cpp JsonInterface.cpp SocketHandler.cpp)
target_link_libraries(gxy_multiserver_client gxy_data gxy_framework)
set(LIBS gxy_multiserver_client ${LIBS})

add_executable(msserver msserver.cpp CommandLine.cpp)
target_link_libraries(msserver gxy_multiserver gxy_data gxy_framework)
set(BINS msserver ${BINS})

add_executable(msviewer msviewer.cpp ClientWindow.cpp JsonInterface.cpp CommandLine.cpp)
target_link_libraries(msviewer gxy_multiserver ${PNG_LIBRARY_RELEASE} ${Z_LIBRARY_RELEASE} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES} ${OSPRAY_LIBRARIES} pthread)
set(BINS msviewer ${BINS})

add_executable(msclient msclient.cpp SocketHandler.cpp CommandLine.cpp)
target_link_libraries(msclient pthread)
set(BINS msclient ${BINS})

add_library(gxy_module_viewer MODULE ViewerClientServer.cpp)
target_link_libraries(gxy_module_viewer gxy_multiserver gxy_renderer ${GALAXY_LIBRARIES})
set(SERVERS gxy_module_viewer ${SERVERS})

add_library(gxy_module_data MODULE DataClientServer.cpp)
target_link_libraries(gxy_module_data gxy_multiserver gxy_framework)
set(SERVERS gxy_module_data ${SERVERS})

add_library(gxy_module_ping MODULE PingClientServer.cpp)
target_link_libraries(gxy_module_ping gxy_multiserver gxy_framework)
set(SERVERS gxy_module_ping ${SERVERS})

add_library(gxy_module_test MODULE TestClientServer.cpp)
target_link_libraries(gxy_module_test gxy_multiserver gxy_framework)
set(SERVERS gxy_module_test ${SERVERS})

install(TARGETS ${BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${SERVERS} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS ${LIBS} DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES
  CommandLine.h
  DynamicLibraryManager.h
  MultiServer.h
  MultiServerHandler.h
  ServerClientConnection.hpp
  SocketHandler.h
  DESTINATION include/gxy)

