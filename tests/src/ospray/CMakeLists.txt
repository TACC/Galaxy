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
										${gxy_data_SOURCE_DIR}
										${gxy_renderer_SOURCE_DIR}
										${gxy_ospray_SOURCE_DIR}
										${OSPRAY_INCLUDE_DIRS}
										${gxy_unit_tests_SOURCE_DIR}/ospray
										${Galaxy_BINARY_DIR}/src
										)


set(GALAXY_LIBRARIES gxy_framework gxy_data gxy_renderer gxy_ospray ${MPI_C_LIBRARIES})
set(BINS "")

add_executable(gxytest-ospray-DataDrivenSpheres DataDrivenSpheres.cpp)
target_link_libraries(gxytest-ospray-DataDrivenSpheres  ${GALAXY_LIBRARIES})
set(BINS gxytest-ospray-DataDrivenSpheres ${BINS})

install(TARGETS ${BINS} DESTINATION tests/ospray)
