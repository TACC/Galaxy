set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
add_subdirectory(framework)
add_subdirectory(renderer)
add_subdirectory(rapidjson)
add_subdirectory(data)
add_subdirectory(apps)

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/pvol_cmake_config.cmake)