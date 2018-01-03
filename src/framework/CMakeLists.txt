cmake_minimum_required (VERSION 2.8.11)
project (Framework)

if (COMMAND cmake_policy)
	if(POLICY CMP0042)
		cmake_policy(SET CMP0042 NEW)
	endif(POLICY CMP0042)
endif (COMMAND cmake_policy)

find_package(MPI REQUIRED)
find_package(TBB REQUIRED)

include_directories(${MPI_INCLUDE_PATH}
			${rapidjson_SOURCE_DIR}/..
			${Framework_SOURCE_DIR}
			${Data_SOURCE_DIR}
			${Renderer_SOURCE_DIR}
			${TBB_INCLUDE_DIRS})


set (CPP_SOURCES Application.cpp
			MessageManager.cpp 
			KeyedObject.cpp
			Events.cpp
			ClientServer.cpp
			Message.cpp
			MessageQ.cpp 
			Work.cpp
			smem.cpp
			debug.cpp)

ospray_create_library(Framework ${CPP_SOURCES} LINK ${OSPRAY_LIBRARIES} ${TBB_LIBRARIES} threadpool11 ${MPI_LIBRARIES} COMPONENT lib) 
set_target_properties(Framework PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
set_target_properties(Framework PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")

install(TARGETS Framework DESTINATION lib)
install(FILES Application.h smem.h Events.h
				ClientServer.h  KeyedObject.h
				MessageManager.h Message.h MessageQ.h 
				Timer.h Work.h debug.h DESTINATION include)
