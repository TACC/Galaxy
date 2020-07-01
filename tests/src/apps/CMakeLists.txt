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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

include_directories(${Galaxy_SOURCE_DIR}/src/framework
                    ${Galaxy_SOURCE_DIR}/src/data
                    ${Galaxy_SOURCE_DIR}/src/ospray
                    ${Galaxy_SOURCE_DIR}/src/renderer
									  ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES gxy_framework gxy_renderer gxy_data ${MPI_C_LIBRARIES})

set(BINS "")

# executables NOT requiring GUI stuff - eg.

add_executable(events events.cpp)
target_link_libraries(events ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
set(BINS events ${BINS})

add_executable(objects objects.cpp)
target_link_libraries(objects gxy_framework)
set(BINS objects ${BINS})

add_executable(bcast_messages bcast_messages.cpp)
target_link_libraries(bcast_messages gxy_framework)
set(BINS bcast_messages ${BINS})

find_package(X11)
find_package(OpenGL)
find_package(GLUT)

if (X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)

	include_directories(${X11_INCLUDE_DIRS} ${GLUT_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR})
	link_libraries(${X11_LIBRARIES})

	# executables requiring GUI stuff - eg.
	# add_executable(async async.cpp quat.cpp AsyncRendering.cpp)
	# target_link_libraries(async ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES})
	# set(BINS async ${BINS})

endif(X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)

install(TARGETS ${BINS} DESTINATION tests/apps)
