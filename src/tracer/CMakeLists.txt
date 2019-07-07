cmake_minimum_required (VERSION 3.9)

set(ospray_DIR ~/galaxy.dev/Galaxy/third-party/ospray/install/lib/cmake/ospray-1.6.1/)
set(embree_DIR ~/galaxy.dev/Galaxy/third-party/embree/install/lib/cmake/embree-3.2.0)

find_package(embree 3 QUIET)
find_package(ospray)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

find_package(ospray)
link_directories(${OSPRAY_LIBRARY_DIR})

find_package(embree 3)

find_package(GALAXY REQUIRED)
include_directories(${Galaxy_INCLUDE_DIRS}/gxy ${Galaxy_INCLUDE_DIRS})
link_directories(${Galaxy_LIBRARY_DIR})

find_package(VTK REQUIRED NO_MODULE)
include(${VTK_USE_FILE})

set(CMAKE_CXX_STANDARD 11)

set(Galaxy_LIBRARIES gxy_framework gxy_renderer gxy_data gxy_ospray ${OSPRAY_LIBRARIES} ${MPI_C_LIBRARIES})

add_executable(tester tester.cpp RungeKutta.cpp TraceToPathLines.cpp)
target_link_libraries(tester ${VTK_LIBRARIES} ${Galaxy_LIBRARIES} ${MPI_C_LIBRARIES})


