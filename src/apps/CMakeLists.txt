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

include_directories(${gxy_framework_SOURCE_DIR} 
                    ${gxy_renderer_SOURCE_DIR}
                    ${gxy_renderer_BINARY_DIR}
					          ${gxy_sampler_SOURCE_DIR}
                    ${OSPRAY_INCLUDE_DIRS}
					          ${gxy_data_SOURCE_DIR}
					          ${gxy_intel_SOURCE_DIR}
					          ${gxy_multiserver_SOURCE_DIR}
					          ${Galaxy_BINARY_DIR}/src)

set(GALAXY_LIBRARIES 
    gxy_framework 
    gxy_renderer 
    gxy_sampler
    gxy_data 
    gxy_intel 
    ${OSPRAY_LIBRARIES}
    ${MPI_C_LIBRARIES})

set(BINS "")

find_package(X11)
find_package(OpenGL)
find_package(GLUT)
find_package(VTK)

add_executable(simsim simsim.cpp)
target_link_libraries(simsim gxy_multiserver_client ${VTK_LIBRARIES} ${MPI_C_LIBRARIES})

add_executable(partition_vti partition_vti.cpp)
target_link_libraries(partition_vti ${VTK_LIBRARIES})

if (GXY_WRITE_IMAGES)

  add_executable(gxywriter gxywriter.cpp)
  target_link_libraries(gxywriter ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
  set(BINS gxywriter ${BINS})

  add_executable(sample sample.cpp)
  target_link_libraries(sample ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
  set(BINS sample ${BINS})

  # add_executable(particleproc particleproc.cpp)
  # target_link_libraries(particleproc ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
  # set(BINS particleproc ${BINS})

  add_executable(mhwriter mhwriter.cpp)
	target_link_libraries(mhwriter ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
	set(BINS mhwriter ${BINS})

else(GXY_WRITE_IMAGES)

	add_executable(gxyviewer-server gxyviewer-server.cpp Socket.cpp ServerRendering.cpp)
	target_link_libraries(gxyviewer-server ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
	set(BINS gxyviewer-server ${BINS})

	add_executable(gxyviewer-client-no-x gxyviewer-client-no-x.cpp Socket.cpp)
	target_link_libraries(gxyviewer-client-no-x pthread ${PNG_LIBRARY_RELEASE} ${Z_LIBRARY_RELEASE})
	set(BINS gxyviewer-client-no-x ${BINS})

	if (X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)
		include_directories(${X11_INCLUDE_DIRS} ${GLUT_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR})
		link_libraries(${X11_LIBRARIES})

		add_executable(mhviewer mhviewer.cpp AsyncRendering.cpp)
    target_link_libraries(mhviewer ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})
		set(BINS mhviewer ${BINS})

		add_executable(gxyviewer gxyviewer.cpp AsyncRendering.cpp)
    target_link_libraries(gxyviewer ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})

		add_executable(gxyviewer-worker gxyviewer-worker.cpp AsyncRendering.cpp)
		# target_link_libraries(gxyviewer-worker ${VTK_LIBRARIES} ${GALAXY_LIBRARIES} ${OPENGL_LIBRARIES})
		target_link_libraries(gxyviewer-worker ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})

		add_executable(gxyviewer-client gxyviewer-client.cpp Socket.cpp)
    target_link_libraries(gxyviewer-client ${PNG_LIBRARY_RELEASE} ${Z_LIBRARY_RELEASE} ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES} pthread)

		set(BINS gxyviewer-worker gxyviewer gxyviewer-client ${BINS})
  else(X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)
		message(WARNING "Could not find all GUI components for gxyviewer client: X11 ${X11_FOUND}, OpenGL ${OpenGL_FOUND}, GLUT ${GLUT_FOUND}")
	endif(X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)

endif (GXY_WRITE_IMAGES)

option(GXP_APP_CINEMA "Build cimema database generator" ON)
if (WITH_CINEMA)
	set(GALAXY_LIBRARIES Cinema ${GALAXY_LIBRARIES})
  add_executable(cinema cinema.cpp)
  target_link_libraries(cinema ${GALAXY_LIBRARIES})
	set(BINS ${BINS} cinema)
endif (WITH_CINEMA)

option(GXY_APP_PISO "Build the Galaxy piso app (requires VTK 8.x)" OFF)
if(GXY_APP_PISO)
	add_executable(piso piso.cpp)
	target_link_libraries(piso ${VTK_LIBRARIES} ${MPI_C_LIBRARIES})
	set(BINS piso ${BINS})
endif(GXY_APP_PISO)

add_executable(client client.cpp)
target_link_libraries(client gxy_framework ${TBB_LIBRARY})
set(BINS client ${BINS})

add_executable(ppong ppong.cpp)
target_link_libraries(ppong gxy_framework ${TBB_LIBRARY})
set(BINS ppong ${BINS})

add_executable(bcast bcast.cpp)
target_link_libraries(bcast gxy_framework ${TBB_LIBRARY})
set(BINS bcast ${BINS})

add_executable(drop drop.cpp TestObject.cpp)
target_link_libraries(drop gxy_framework ${TBB_LIBRARY})
set(BINS drop ${BINS})

# add_executable(gvt gvt.cpp)
# target_link_libraries(gvt ${VTK_LIBRARIES} ${GALAXY_LIBRARIES})
# set(BINS gvt ${BINS})

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

install(TARGETS ${BINS} DESTINATION bin)
