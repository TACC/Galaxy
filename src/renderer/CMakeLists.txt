cmake_minimum_required (VERSION 2.8.11)
project (Renderer)

if (COMMAND cmake_policy)
	if(POLICY CMP0042)
		cmake_policy(SET CMP0042 NEW)
	endif(POLICY CMP0042)
endif (COMMAND cmake_policy)

find_package(MPI REQUIRED)
find_package(TBB REQUIRED)
include(FindPNG)

include_directories(${MPI_INCLUDE_PATH}
										${Framework_SOURCE_DIR}
										${Renderer_SOURCE_DIR}
										${rapidjson_SOURCE_DIR}/..
										${Data_SOURCE_DIR}
										${Data_BINARY_DIR}
										${TBB_INCLUDE_DIRS})

link_directories(${CMAKE_BINARY_DIR}/src/data ${CMAKE_BINARY_DIR}/src/framework)

set (ISPC_SOURCES Lighting.ispc TraceRays.ispc Rays.ispc)

set (CPP_SOURCES 
					Camera.cpp 
					Rendering.cpp 
					RenderingSet.cpp 
					Lighting.cpp 
                    Renderer.cpp
                    TraceRays.cpp
                    Rays.cpp 
					RayQManager.cpp)

ospray_create_library(Renderer ${CPP_SOURCES} ${ISPC_SOURCES} LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} Data Framework COMPONENT lib)

install(TARGETS Renderer DESTINATION lib)
install(FILES Renderer.h
        Rendering.h 
        RenderingSet.h 
        Camera.h 
        Pixel.h 
        Lighting.h 
        TraceRays.h 
        Rays.h 
        RayQManager.h 
        dtypes.h 
            DESTINATION include/pvol)
 