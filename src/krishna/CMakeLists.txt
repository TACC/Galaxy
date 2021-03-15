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
project (gxy_krishna VERSION ${GALAXY_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
include_directories(${gxy_framework_SOURCE_DIR}
                    ${gxy_renderer_SOURCE_DIR}
                    ${gxy_sampler_SOURCE_DIR}
                    ${OSPRAY_INCLUDE_DIRS}
                    ${gxy_data_SOURCE_DIR}
                    ${gxy_ospray_SOURCE_DIR}
                    ${gxy_multiserver_SOURCE_DIR}
                    ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES
    gxy_framework
    gxy_renderer
    gxy_sampler
    gxy_data
    gxy_ospray
    ${OSPRAY_LIBRARIES}
    ${MPI_C_LIBRARIES})

add_executable(krcvr rcvr.cpp Receiver.cpp Partitioning.cpp)
target_link_libraries(krcvr gxy_data gxy_framework)

add_executable(ksndr sndr.cpp)
target_link_libraries(ksndr ${VTK_LIBRARIES} ${MPI_LIBRARIES})

add_executable(kparts parts.cpp Partitioning.cpp)
target_link_libraries(kparts gxy_data gxy_framework ${MPI_LIBRARIES})

add_executable(kwrtr wrtr.cpp Receiver.cpp Partitioning.cpp)
target_link_libraries(kwrtr ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
set(BINS kwrtr ${BINS})

install(TARGETS kwrtr krcvr ksndr DESTINATION bin)
