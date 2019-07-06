cmake_minimum_required (VERSION 3.9)

find_package(GALAXY REQUIRED)
include_directories(${Galaxy_INCLUDE_DIRS})
link_directories(${Galaxy_LIBRARY_DIR})

find_package(VTK REQUIRED NO_MODULE)
include(${VTK_USE_FILE})

set(CMAKE_CXX_STANDARD 11)

set(Galaxy_LIBRARIES gxy_framework gxy_data gxy_ospray ${MPI_C_LIBRARIES})

add_executable(tester tester.cpp RungeKutta.cpp)
target_link_libraries(tester ${VTK_LIBRARIES} ${Galaxy_LIBRARIES})


