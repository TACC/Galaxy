## ========================================================================== ##
##                                                                            ##
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
project (gxy_insitu VERSION ${GALAXY_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

include_directories(${gxy_multiserver_SOURCE_DIR} 
                    ${gxy_data_SOURCE_DIR} 
                    ${gxy_framework_SOURCE_DIR} 
                    ${gxy_ospray_SOURCE_DIR}
                    ${Galaxy_BINARY_DIR}/src 
                    ${OSPRAY_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS} )

add_library(gxy_module_insitu MODULE SocketConnector.cpp SocketConnectorClientServer.cpp)
target_link_libraries(gxy_module_insitu gxy_multiserver gxy_framework ${VTK_LIBRARIES} ${MPI_C_LIBRARIES})
set(SERVERS gxy_module_insitu ${SERVERS})

install(TARGETS gxy_module_insitu DESTINATION ${CMAKE_INSTALL_LIBDIR})
