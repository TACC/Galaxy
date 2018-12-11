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

cmake_minimum_required (VERSION 3.9)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

include_directories(${gxy_framework_SOURCE_DIR} 
									  ${gxy_renderer_SOURCE_DIR}
									  ${gxy_sampler_SOURCE_DIR}
									  ${gxy_data_SOURCE_DIR}
									  ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES gxy_framework gxy_renderer gxy_data ${MPI_C_LIBRARIES})

set(BINS "")

if (GXY_WRITE_IMAGES)

	add_executable(vis vis.cpp)
	target_link_libraries(vis ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
	set(BINS vis ${BINS})

  # add_executable(samray samray.cpp)
  # target_link_libraries(samray ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} gxy_sampler)
  # set(BINS samray ${BINS})

  add_executable(sample sample.cpp)
  target_link_libraries(sample ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
  set(BINS sample ${BINS})

else(GXY_WRITE_IMAGES)

	add_executable(async-server async-server.cpp Socket.cpp ServerRendering.cpp quat.cpp)
	target_link_libraries(async-server ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
	set(BINS async-server ${BINS})

	add_executable(async-client-no-x async-client-no-x.cpp Socket.cpp)
	target_link_libraries(async-client-no-x pthread ${PNG_LIBRARY_RELEASE})
	set(BINS async-client-no-x ${BINS})

	find_package(X11)
	find_package(OpenGL)
	find_package(GLUT)

	if (X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)
		include_directories(${X11_INCLUDE_DIRS} ${GLUT_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR})
		link_libraries(${X11_LIBRARIES})

		add_executable(amh amh.cpp quat.cpp AsyncRendering.cpp)
		target_link_libraries(amh ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES})
		set(BINS amh ${BINS})

		add_executable(async async.cpp quat.cpp AsyncRendering.cpp)
		target_link_libraries(async ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES})

		add_executable(async-worker async-worker.cpp AsyncRendering.cpp)
		# target_link_libraries(async-worker ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} ${OPENGL_LIBRARIES})
		target_link_libraries(async-worker ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})

		add_executable(async-client async-client.cpp Socket.cpp)
		target_link_libraries(async-client ${PNG_LIBRARY_RELEASE} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES} pthread)

		set(BINS async-worker async async-client ${BINS})
  else(X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)
		message(WARNING "Could not find all GUI components for async client: X11 ${X11_FOUND}, OpenGL ${OpenGL_FOUND}, GLUT ${GLUT_FOUND}")
	endif(X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)

endif (GXY_WRITE_IMAGES)

add_executable(mh mh.cpp)
target_link_libraries(mh ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
set(BINS mh ${BINS})

option(GXP_APP_CINEMA "Build cimema database generator" ON)
if (WITH_CINEMA)
	set(GALAXY_LIBRARIES Cinema ${GALAXY_LIBRARIES})
  add_executable(cinema cinema.cpp)
  target_link_libraries(cinema ${GALAXY_LIBRARIES})
	set(BINS ${BINS} cinema)
endif (WITH_CINEMA)

option(GXY_APP_STATE "Build simple Galaxy renderer" OFF)
if (GXY_APP_STATE)
	add_executable(state state.cpp)
	target_link_libraries(state ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
	set(BINS ${BINS} state)
endif (GXY_APP_STATE)

option(GXY_APP_PISO "Build the Galaxy piso app (requires VTK 8.x)" OFF)
if(GXY_APP_PISO)
	add_executable(piso piso.cpp)
	target_link_libraries(piso ${VTK_LIBRARIES} ${MPI_C_LIBRARIES})
	set(BINS piso ${BINS})
endif(GXY_APP_PISO)

add_executable(client client.cpp)
target_link_libraries(client gxy_framework)
set(BINS client ${BINS})

add_executable(ppong ppong.cpp)
target_link_libraries(ppong gxy_framework)
set(BINS ppong ${BINS})

add_executable(bcast bcast.cpp)
target_link_libraries(bcast gxy_framework)
set(BINS bcast ${BINS})

add_executable(drop drop.cpp TestObject.cpp)
target_link_libraries(drop gxy_framework)
set(BINS drop ${BINS})

add_executable(gvt gvt.cpp)
target_link_libraries(gvt ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
set(BINS gvt ${BINS})

add_executable(radial radial.cpp)
target_link_libraries(radial ${VTK_LIBRARIES} ${MPI_LIBRARIES})
set(BINS radial ${BINS})

add_executable(mkraw mkraw.cpp)
set(BINS mkraw ${BINS})

add_executable(particles particles.cpp)
set(BINS particles ${BINS})

add_executable(partition partition.cpp)
target_link_libraries(partition ${MPI_LIBRARIES})
set(BINS partition ${BINS})

install(PROGRAMS blazar DESTINATION python)

install(TARGETS ${BINS} DESTINATION bin)
