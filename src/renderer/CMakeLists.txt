cmake_minimum_required (VERSION 2.8.11)
project (gxy_renderer)

if (COMMAND cmake_policy)
	if(POLICY CMP0042)
		cmake_policy(SET CMP0042 NEW)
	endif(POLICY CMP0042)
endif (COMMAND cmake_policy)

find_package(MPI REQUIRED)
find_package(TBB REQUIRED)
include(FindPNG)

include_directories(${gxy_framework_SOURCE_DIR}
										${gxy_renderer_SOURCE_DIR}
										${gxy_data_SOURCE_DIR}
										${gxy_data_BINARY_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}/..
                    ${rapidjson_SOURCE_DIR}/..
										${MPI_INCLUDE_PATH}
                    ${TBB_INCLUDE_DIRS}
                    )

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

if (ospray_DIR)
  ospray_create_library(gxy_renderer ${CPP_SOURCES} ${ISPC_SOURCES} LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_data gxy_framework COMPONENT lib)
  install(TARGETS gxy_renderer DESTINATION lib)
endif()
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
        DESTINATION include/gxy)
 
