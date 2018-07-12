cmake_minimum_required(VERSION 2.8)
project (Data)

find_library(PNG_LIBRARY_RELEASE png)

include_directories(${MPI_INCLUDE_PATH}
                    ${rapidjson_SOURCE_DIR}/..
                    ${Framework_SOURCE_DIR}
                    ${Renderer_SOURCE_DIR}
                    ${Data_SOURCE_DIR}
                    ${TBB_INCLUDE_DIRS}
										${CMAKE_CURRENT_BINARY_DIR}
                    ${EMBREE_INCLUDE_DIRS}
                    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../framework ${CMAKE_CURRENT_SOURCE_DIR}/../renderer)

set (CPP_SOURCES     Visualization.cpp
										 Lighting.cpp
                     Triangles.cpp 
										 Particles.cpp 
                     Volume.cpp 
                     Box.cpp 
                     Datasets.cpp
                     VolumeVis.cpp
                     KeyedDataObject.cpp
                     Geometry.cpp 
                     ISPCObject.cpp
                     Vis.cpp
                     MappedVis.cpp
                     ParticlesVis.cpp
                     TrianglesVis.cpp
                     DataObjects.cpp
                     OSPRayObject.cpp
                     OSPUtil.cpp)

set (ISPC_SOURCES   ISPCObject.ispc 
										 Lighting.ispc
										Visualization.ispc 
										Vis.ispc 
										MappedVis.ispc 
										ParticlesVis.ispc
										TrianglesVis.ispc
										VolumeVis.ispc)

ospray_create_library(Data ${CPP_SOURCES} ${ISPC_SOURCES} LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} Framework ${PNG_LIBRARY_RELEASE} COMPONENT lib)

INSTALL(TARGETS Data DESTINATION lib)
INSTALL(FILES Visualization.h KeyedDataObject.h  Lighting.h
            Triangles.h Particles.h Volume.h Box.h ISPCObject.h
            Geometry.h OSPRayObject.h OSPUtil.h
            Datasets.h Vis.h VolumeVis.h ParticlesVis.h TrianglesVis.h
            MappedVis.h DataObjects.h ImageWriter.h
        DESTINATION include/pvol)

