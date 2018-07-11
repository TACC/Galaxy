cmake_minimum_required (VERSION 2.8.11)
project (gxy_framework)

if (COMMAND cmake_policy)
	if(POLICY CMP0042)
		cmake_policy(SET CMP0042 NEW)
	endif(POLICY CMP0042)
endif (COMMAND cmake_policy)

find_package(MPI REQUIRED)
find_package(TBB REQUIRED)

include_directories(
			${GXY_FRAMEWORK_SOURCE_DIR}
			${GXY_DATA_SOURCE_DIR}
			${GXY_RENDERER_SOURCE_DIR}
			${CMAKE_CURRENT_BINARY_DIR}/..
			${TBB_INCLUDE_DIRS}
	    ${MPI_INCLUDE_PATH}
			${rapidjson_SOURCE_DIR}/..
			)


set (CPP_SOURCES 
			Application.cpp
			MessageManager.cpp 
			KeyedObject.cpp
			Events.cpp
			ClientServer.cpp
			Message.cpp
			MessageQ.cpp 
			Work.cpp
			smem.cpp
			debug.cpp
			)

if (ospray_DIR)
  ospray_create_library(gxy_framework ${CPP_SOURCES} LINK ${OSPRAY_LIBRARIES} ${TBB_LIBRARIES} threadpool11 ${MPI_LIBRARIES} COMPONENT lib) 
  set_target_properties(gxy_framework PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
  set_target_properties(gxy_framework PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
  install(TARGETS gxy_framework DESTINATION lib)
endif()
install(FILES 
							Application.h 
							smem.h 
							Events.h
							ClientServer.h  
							KeyedObject.h
							MessageManager.h 
							Message.h 
							MessageQ.h 
							Timer.h 
							Work.h 
							debug.h 
				DESTINATION include/gxy)
