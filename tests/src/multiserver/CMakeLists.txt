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

include_directories(${Galaxy_SOURCE_DIR}/src/framework
                    ${Galaxy_SOURCE_DIR}/src/data
                    ${Galaxy_SOURCE_DIR}/src/ospray
                    ${Galaxy_SOURCE_DIR}/src/renderer
                    ${Galaxy_SOURCE_DIR}/src/multiserver
										${OSPRAY_INCLUDE_DIRS}
										${gxy_unit_tests_SOURCE_DIR}/multiserver
										${Galaxy_BINARY_DIR}/src
										)

set(GALAXY_LIBRARIES gxy_framework gxy_data gxy_multiserver gxy_ospray ${MPI_C_LIBRARIES})
set(BINS "")

add_executable(gxytest-multiserver-ClientWindow ClientWindow.cpp)
target_link_libraries(gxytest-multiserver-ClientWindow  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-ClientWindow ${BINS})

add_executable(gxytest-multiserver-CommandLine CommandLine.cpp)
target_link_libraries(gxytest-multiserver-CommandLine  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-CommandLine ${BINS})

add_executable(gxytest-multiserver-DataClientServer DataClientServer.cpp)
target_link_libraries(gxytest-multiserver-DataClientServer  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-DataClientServer ${BINS})

add_executable(gxytest-multiserver-DynamicLibraryManager DynamicLibraryManager.cpp)
target_link_libraries(gxytest-multiserver-DynamicLibraryManager  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-DynamicLibraryManager ${BINS})

add_executable(gxytest-multiserver-JsonInterface JsonInterface.cpp)
target_link_libraries(gxytest-multiserver-JsonInterface  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-JsonInterface ${BINS})

add_executable(gxytest-multiserver-MultiServer MultiServer.cpp)
target_link_libraries(gxytest-multiserver-MultiServer  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-MultiServer ${BINS})

add_executable(gxytest-multiserver-MultiServerHandler MultiServerHandler.cpp)
target_link_libraries(gxytest-multiserver-MultiServerHandler  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-MultiServerHandler ${BINS})

add_executable(gxytest-multiserver-MultiServerObject MultiServerObject.cpp)
target_link_libraries(gxytest-multiserver-MultiServerObject  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-MultiServerObject ${BINS})

add_executable(gxytest-multiserver-MultiServerSocket MultiServerSocket.cpp)
target_link_libraries(gxytest-multiserver-MultiServerSocket  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-MultiServerSocket ${BINS})

add_executable(gxytest-multiserver-PingClientServer PingClientServer.cpp)
target_link_libraries(gxytest-multiserver-PingClientServer  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-PingClientServer ${BINS})

add_executable(gxytest-multiserver-ServerRendering ServerRendering.cpp)
target_link_libraries(gxytest-multiserver-ServerRendering  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-ServerRendering ${BINS})

add_executable(gxytest-multiserver-SocketHandler SocketHandler.cpp)
target_link_libraries(gxytest-multiserver-SocketHandler  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-SocketHandler ${BINS})

add_executable(gxytest-multiserver-ViewerClientServer ViewerClientServer.cpp)
target_link_libraries(gxytest-multiserver-ViewerClientServer  ${GALAXY_LIBRARIES})
set(BINS gxytest-multiserver-ViewerClientServer ${BINS})


install(TARGETS ${BINS} DESTINATION tests/multiserver)
