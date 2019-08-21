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

include_directories(${gxy_framework_SOURCE_DIR}
										${gxy_unit_tests_SOURCE_DIR}/framework
										${Galaxy_BINARY_DIR}/src
										)

set(GALAXY_LIBRARIES gxy_framework ${MPI_C_LIBRARIES})
set(BINS "")

add_executable(gxytest-framework-Application Application.cpp)
target_link_libraries(gxytest-framework-Application  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-Application ${BINS})

add_executable(gxytest-framework-ClientServer ClientServer.cpp)
target_link_libraries(gxytest-framework-ClientServer  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-ClientServer ${BINS})

add_executable(gxytest-framework-Events Events.cpp)
target_link_libraries(gxytest-framework-Events  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-Events ${BINS})

add_executable(gxytest-framework-KeyedObject KeyedObject.cpp)
target_link_libraries(gxytest-framework-KeyedObject  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-KeyedObject ${BINS})

add_executable(gxytest-framework-Messaging Messaging.cpp)
target_link_libraries(gxytest-framework-Messaging  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-Messaging ${BINS})

add_executable(gxytest-framework-smem smem.cpp)
target_link_libraries(gxytest-framework-smem  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-smem ${BINS})

add_executable(gxytest-framework-Threading Threading.cpp)
target_link_libraries(gxytest-framework-Threading  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-Threading ${BINS})

add_executable(gxytest-framework-Timer Timer.cpp)
target_link_libraries(gxytest-framework-Timer  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-Timer ${BINS})

add_executable(gxytest-framework-Work Work.cpp)
target_link_libraries(gxytest-framework-Work  ${GALAXY_LIBRARIES})
set(BINS gxytest-framework-Work ${BINS})

install(TARGETS ${BINS} DESTINATION tests/framework)
