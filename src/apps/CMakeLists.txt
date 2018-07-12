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
									  ${gxy_renderer_SOURCE_DIR}
									  ${gxy_data_SOURCE_DIR}
									  ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES gxy_framework gxy_renderer gxy_data ${MPI_C_LIBRARIES})

set(BINS "")

add_executable(vis vis.cpp)
target_link_libraries(vis ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
set(BINS vis ${BINS})

# add_executable(sample sample.cpp)
# target_link_libraries(sample ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
# set(BINS sample ${BINS})

# if (WITH_CINEMA)
	# set(GALAXY_LIBRARIES Cinema ${GALAXY_LIBRARIES})
  # add_executable(cinema cinema.cpp)
  # target_link_libraries(cinema ${GALAXY_LIBRARIES})
	# SET(BINS ${BINS} cinema)
# endif (WITH_CINEMA)

# if (WITH_STATE)
	# add_executable(state state.cpp)
	# target_link_libraries(state ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
	# SET(BINS ${BINS} state)
# endif (WITH_STATE)

# add_executable(ppong ppong.cpp)
# target_link_libraries(ppong Framework)
# set(BINS ppong ${BINS})

# add_executable(drop drop.cpp TestObject.cpp)
# target_link_libraries(drop Framework)
# set(BINS drop ${BINS})

add_executable(radial radial.cpp)
target_link_libraries(radial ${VTK_LIBRARIES} ${MPI_LIBRARIES})
set(BINS radial ${BINS})

add_executable(mkraw mkraw.cpp)
set(BINS mkraw ${BINS})

add_executable(particles particles.cpp)
set(BINS particles ${BINS})

add_executable(partition partition.cpp)
set(BINS partition ${BINS})

install(TARGETS ${BINS} DESTINATION bin)
