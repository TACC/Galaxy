set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
add_subdirectory(framework)
add_subdirectory(renderer)
add_subdirectory(data)
add_subdirectory(apps)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/galaxy.h.in ${CMAKE_CURRENT_BINARY_DIR}/galaxy.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/galaxy.h DESTINATION include/gxy)

#INCLUDE(${CMAKE_SOURCE_DIR}/cmake/galaxy_cmake_config.cmake)
