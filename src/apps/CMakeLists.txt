cmake_minimum_required (VERSION 2.8.11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

include_directories(${Framework_SOURCE_DIR} 
									  ${Renderer_SOURCE_DIR} 
									  ${rapidjson_SOURCE_DIR}/..
									  ${Data_SOURCE_DIR})

set(PVOL_LIBRARIES Framework Renderer Data ${MPI_C_LIBRARIES})

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(BINS "")

if (PVOL_SYNCHRONOUS)

	add_executable(vis vis.cpp)
	target_link_libraries(vis ${VTK_LIBRARIES} ${PVOL_LIBRARIES})
	set(BINS vis ${BINS})

else()

	add_executable(async-server async-server.cpp Socket.cpp ServerRendering.cpp quat.cpp)
	target_link_libraries(async-server ${VTK_LIBRARIES} ${PVOL_LIBRARIES})
	set(BINS async-server ${BINS})

	add_executable(async-client-no-x async-client-no-x.cpp Socket.cpp)
	target_link_libraries(async-client-no-x pthread ${PNG_LIBRARY_RELEASE})
	set(BINS async-client-no-x ${BINS})

	find_package(X11)
	if (X11_FOUND)
		message("X11 OK")
	endif()
	
	find_package(OpenGL)
	if (OpenGL_FOUND)
		message("OpenGL OK")
	endif()

	find_package(GLUT)
	if (GLUT_FOUND)
		message("GLUT OK")
	endif()

	if (X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)
		include_directories(${X11_INCLUDE_DIRS} ${GLUT_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR})
		link_directories(${X11_LIBRARIES})

		add_executable(async async.cpp quat.cpp AsyncRendering.cpp)
		target_link_libraries(async ${VTK_LIBRARIES} ${PVOL_LIBRARIES} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES})

		add_executable(async-worker async-worker.cpp AsyncRendering.cpp)
		# target_link_libraries(async-worker ${VTK_LIBRARIES} ${PVOL_LIBRARIES} ${OPENGL_LIBRARIES})
		target_link_libraries(async-worker ${VTK_LIBRARIES} ${PVOL_LIBRARIES})

		add_executable(async-client async-client.cpp Socket.cpp)
		target_link_libraries(async-client ${PNG_LIBRARY_RELEASE} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES} pthread)

		set(BINS async-worker async async-client ${BINS})
  else(X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)
		message("No GUI stuff... async AND async-client will not be built")
	endif(X11_FOUND AND OpenGL_FOUND AND GLUT_FOUND)

endif (PVOL_SYNCHRONOUS)

# add_executable(sample sample.cpp)
# target_link_libraries(sample ${VTK_LIBRARIES} ${PVOL_LIBRARIES})
# set(BINS sample ${BINS})

# if (WITH_CINEMA)
	# set(PVOL_LIBRARIES Cinema ${PVOL_LIBRARIES})
  # add_executable(cinema cinema.cpp)
  # target_link_libraries(cinema ${PVOL_LIBRARIES})
	# SET(BINS ${BINS} cinema)
# endif (WITH_CINEMA)

# if (WITH_STATE)
	# add_executable(state state.cpp)
	# target_link_libraries(state ${VTK_LIBRARIES} ${PVOL_LIBRARIES})
	# SET(BINS ${BINS} state)
# endif (WITH_STATE)

add_executable(client client.cpp)
target_link_libraries(client Framework)
set(BINS client ${BINS})

add_executable(ppong ppong.cpp)
target_link_libraries(ppong Framework)
set(BINS ppong ${BINS})

add_executable(bcast bcast.cpp)
target_link_libraries(bcast Framework)
set(BINS bcast ${BINS})

add_executable(drop drop.cpp TestObject.cpp)
target_link_libraries(drop Framework)
set(BINS drop ${BINS})

add_executable(gvt gvt.cpp)
target_link_libraries(gvt ${VTK_LIBRARIES} ${PVOL_LIBRARIES})
set(BINS gvt ${BINS})

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
