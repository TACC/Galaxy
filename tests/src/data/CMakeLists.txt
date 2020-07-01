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
                    ${OSPRAY_INCLUDE_DIRS}
										${gxy_unit_tests_SOURCE_DIR}/data
										${Galaxy_BINARY_DIR}/src
										)

set(GALAXY_LIBRARIES gxy_framework gxy_data ${MPI_C_LIBRARIES})
set(BINS "")

add_executable(gxytest-data-AmrVolume AmrVolume.cpp)
target_link_libraries(gxytest-data-AmrVolume  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-AmrVolume ${BINS})

add_executable(gxytest-data-Box Box.cpp)
target_link_libraries(gxytest-data-Box  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-Box ${BINS})

add_executable(gxytest-data-DataObjects DataObjects.cpp)
target_link_libraries(gxytest-data-DataObjects  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-DataObjects ${BINS})

add_executable(gxytest-data-Datasets Datasets.cpp)
target_link_libraries(gxytest-data-Datasets  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-Datasets ${BINS})

add_executable(gxytest-data-Geometry Geometry.cpp)
target_link_libraries(gxytest-data-Geometry  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-Geometry ${BINS})

add_executable(gxytest-data-KeyedDataObject KeyedDataObject.cpp)
target_link_libraries(gxytest-data-KeyedDataObject  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-KeyedDataObject ${BINS})

add_executable(gxytest-data-Particles Particles.cpp)
target_link_libraries(gxytest-data-Particles  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-Particles ${BINS})

add_executable(gxytest-data-Triangles Triangles.cpp)
target_link_libraries(gxytest-data-Triangles  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-Triangles ${BINS})

add_executable(gxytest-data-Volume Volume.cpp)
target_link_libraries(gxytest-data-Volume  ${GALAXY_LIBRARIES})
set(BINS gxytest-data-Volume ${BINS})

install(TARGETS ${BINS} DESTINATION tests/data)
