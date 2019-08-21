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
										${OSPRAY_INCLUDE_DIRS}
										${gxy_unit_tests_SOURCE_DIR}/renderer
										${Galaxy_BINARY_DIR}/src
										)

set(GALAXY_LIBRARIES gxy_framework gxy_data gxy_renderer ${MPI_C_LIBRARIES})
set(BINS "")

add_executable(gxytest-renderer-Camera Camera.cpp)
target_link_libraries(gxytest-renderer-Camera  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-Camera ${BINS})

add_executable(gxytest-renderer-Lighting Lighting.cpp)
target_link_libraries(gxytest-renderer-Lighting  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-Lighting ${BINS})

add_executable(gxytest-renderer-MappedVis MappedVis.cpp)
target_link_libraries(gxytest-renderer-MappedVis  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-MappedVis ${BINS})

if (ospray_DIR)
	# ospray_create_application(gxytest-renderer-OsprayObject OsprayObject.cpp 
														# LINK ${GALAXY_LIBRARIES})
	add_executable(gxytest-renderer-OsprayObject OsprayObject.cpp)
	target_link_libraries(gxytest-renderer-OsprayObject  ${GALAXY_LIBRARIES})
	set(BINS gxytest-renderer-OsprayObject ${BINS})

	add_executable(gxytest-renderer-OsprayParticles OsprayParticles.cpp)
	target_link_libraries(gxytest-renderer-OsprayParticles  ${GALAXY_LIBRARIES})
	set(BINS gxytest-renderer-OsprayParticles ${BINS})

	add_executable(gxytest-renderer-OsprayTriangles OsprayTriangles.cpp)
	target_link_libraries(gxytest-renderer-OsprayTriangles  ${GALAXY_LIBRARIES})
	set(BINS gxytest-renderer-OsprayTriangles ${BINS})

	add_executable(gxytest-renderer-OsprayUtil OsprayUtil.cpp)
	target_link_libraries(gxytest-renderer-OsprayUtil  ${GALAXY_LIBRARIES})
	set(BINS gxytest-renderer-OsprayUtil ${BINS})

	add_executable(gxytest-renderer-OsprayVolume OsprayVolume.cpp)
	target_link_libraries(gxytest-renderer-OsprayVolume  ${GALAXY_LIBRARIES})
	set(BINS gxytest-renderer-OsprayVolume ${BINS})
endif(ospray_DIR)

add_executable(gxytest-renderer-ParticlesVis ParticlesVis.cpp)
target_link_libraries(gxytest-renderer-ParticlesVis  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-ParticlesVis ${BINS})

add_executable(gxytest-renderer-RayQManager RayQManager.cpp)
target_link_libraries(gxytest-renderer-RayQManager  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-RayQManager ${BINS})

add_executable(gxytest-renderer-Rays Rays.cpp)
target_link_libraries(gxytest-renderer-Rays  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-Rays ${BINS})

add_executable(gxytest-renderer-Renderer Renderer.cpp)
target_link_libraries(gxytest-renderer-Renderer  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-Renderer ${BINS})

add_executable(gxytest-renderer-Rendering Rendering.cpp)
target_link_libraries(gxytest-renderer-Rendering  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-Rendering ${BINS})

add_executable(gxytest-renderer-RenderingEvents RenderingEvents.cpp)
target_link_libraries(gxytest-renderer-RenderingEvents  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-RenderingEvents ${BINS})

add_executable(gxytest-renderer-RenderingSet RenderingSet.cpp)
target_link_libraries(gxytest-renderer-RenderingSet  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-RenderingSet ${BINS})

add_executable(gxytest-renderer-TraceRays TraceRays.cpp)
target_link_libraries(gxytest-renderer-TraceRays  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-TraceRays ${BINS})

add_executable(gxytest-renderer-TrianglesVis TrianglesVis.cpp)
target_link_libraries(gxytest-renderer-TrianglesVis  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-TrianglesVis ${BINS})

add_executable(gxytest-renderer-Vis Vis.cpp)
target_link_libraries(gxytest-renderer-Vis  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-Vis ${BINS})

add_executable(gxytest-renderer-Visualization Visualization.cpp)
target_link_libraries(gxytest-renderer-Visualization  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-Visualization ${BINS})

add_executable(gxytest-renderer-VolumeVis VolumeVis.cpp)
target_link_libraries(gxytest-renderer-VolumeVis  ${GALAXY_LIBRARIES})
set(BINS gxytest-renderer-VolumeVis ${BINS})

install(TARGETS ${BINS} DESTINATION tests/renderer)
