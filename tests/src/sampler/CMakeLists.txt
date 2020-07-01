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

include_directories(${Galaxy_SOURCE_DIR}/src/framework
                    ${Galaxy_SOURCE_DIR}/src/data
                    ${Galaxy_SOURCE_DIR}/src/ospray
                    ${Galaxy_SOURCE_DIR}/src/renderer
                    ${Galaxy_SOURCE_DIR}/src/sampler
                    ${Galaxy_SOURCE_DIR}/src/multiserver
										${OSPRAY_INCLUDE_DIRS}
										${gxy_unit_tests_SOURCE_DIR}/sampler
										${Galaxy_BINARY_DIR}/src
										)

set(GALAXY_LIBRARIES gxy_framework gxy_data gxy_renderer ${MPI_C_LIBRARIES})
set(BINS "")

add_executable(gxytest-sampler-Sampler Sampler.cpp)
target_link_libraries(gxytest-sampler-Sampler  ${GALAXY_LIBRARIES})
set(BINS gxytest-sampler-Sampler ${BINS})

add_executable(gxytest-sampler-MHSampleClientServer MHSampleClientServer.cpp)
target_link_libraries(gxytest-sampler-MHSampleClientServer  ${GALAXY_LIBRARIES})
set(BINS gxytest-sampler-MHSampleClientServer ${BINS})

add_executable(gxytest-sampler-InterpolatorClientServer InterpolatorClientServer.cpp)
target_link_libraries(gxytest-sampler-InterpolatorClientServer  ${GALAXY_LIBRARIES})
set(BINS gxytest-sampler-InterpolatorClientServer ${BINS})

install(TARGETS ${BINS} DESTINATION tests/sampler)
