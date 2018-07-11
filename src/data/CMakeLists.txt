cmake_minimum_required(VERSION 2.8)
project (gxy_data)

find_library(PNG_LIBRARY_RELEASE png)

include_directories(${gxy_framework_SOURCE_DIR}
                    ${gxy_renderer_SOURCE_DIR}
                    ${gxy_data_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}/..
                    ${MPI_INCLUDE_PATH}
                    ${rapidjson_SOURCE_DIR}/..
                    ${ICET_OBJDIR}/include
                    ${TBB_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS}
                    )

set (CPP_SOURCES     Visualization.cpp
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
                     DataObjects.cpp
                     OSPRayObject.cpp
                     OSPUtil.cpp
                     mypng.cpp)

set (ISPC_SOURCES   ISPCObject.ispc
      	            Visualization.ispc 
                    Vis.ispc 
      	            MappedVis.ispc 
      	            ParticlesVis.ispc
      	            VolumeVis.ispc
                    )

if(ospray_DIR)
  ospray_create_library(gxy_data ${CPP_SOURCES} ${ISPC_SOURCES} LINK ${OSPRAY_LIBRARIES} ${VTK_LIBRARIES} gxy_framework ${PNG_LIBRARY_RELEASE} COMPONENT lib)
  INSTALL(TARGETS gxy_data DESTINATION lib)
endif()
INSTALL(FILES Visualization.h 
              KeyedDataObject.h 
              Particles.h 
              Volume.h 
              Box.h 
              ISPCObject.h
              Geometry.h 
              OSPRayObject.h 
              OSPUtil.h
              Datasets.h 
              Vis.h 
              VolumeVis.h 
              ParticlesVis.h
              MappedVis.h 
              DataObjects.h 
              mypng.h
        DESTINATION include/gxy)

