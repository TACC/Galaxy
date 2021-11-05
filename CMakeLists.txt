## ========================================================================== ##
## Copyright (c) 2014-2020 The University of Texas at Austin.                 ##
## All rights reserved.                                                       ##
##                                                                            ##
## Licensed under the Apache License, Version 2.0 (the "License");            ##
## you may not use this file except in compliance with the License.           ##
## A copy of the License is included with this software in the file LICENSE.  ##
## If your copy does not contain the License, you may obtain a copy of the    ##
## License at:                                                                ##
##                                                                            ##
##     https://www.apache.org/licenses/LICENSE-2.0                            ##
##                                                                            ##
## Unless required by applicable law or agreed to in writing, software        ##
## distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  ##
## WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           ##
## See the License for the specific language governing permissions and        ##
## limitations under the License.                                             ##
##                                                                            ##
## ========================================================================== ##
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

cmake_policy(SET CMP0048 NEW)
PROJECT(Galaxy VERSION 0.3.0)

set(GALAXY_VERSION ${PROJECT_VERSION})
set(GALAXY_SOVERSION ${PROJECT_VERSION_MAJOR})

if (COMMAND cmake_policy)
	# use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # interpret if args as vars only if unquoted
  cmake_policy(SET CMP0054 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)

  if(POLICY CMP0074) # since 3.12 
    # use <PackageName>_ROOT variables as search paths
    # needed since OSPRay includes TBB_ROOT
    cmake_policy(SET CMP0074 NEW)
  endif()
endif(COMMAND cmake_policy)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ignore override warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
endif()

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

SET(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Specifies the build type." FORCE)
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
ENDIF(NOT CMAKE_BUILD_TYPE)

set(GXY_EVENT_TRACKING OFF CACHE BOOL "Generate event log during render?")
set(GXY_LOGGING OFF CACHE BOOL "Generate log messages in the Galaxy Renderer")
set(GXY_PRODUCE_STATUS_MESSAGES OFF CACHE BOOL "produce periodic status massages?")
set(GXY_REVERSE_LIGHTING ON CACHE BOOL "Use subtractive lighting model?")
set(GXY_WRITE_IMAGES OFF CACHE BOOL "write image files rather than interactive display (\"batch mode\")?")
set(GXY_TIMING OFF CACHE BOOL "Generate timing statistics for the Galaxy Renderer")
set(GXY_GUI ON CACHE BOOL "Build Galaxy GUI")
set(GXY_UNIT_TESTING OFF CACHE BOOL "Generate Galaxy unit testing framework")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(GALAXY_INCLUDES ${PROJECT_SOURCE_DIR}/src/data ${PROJECT_SOURCE_DIR}/src/framework ${PROJECT_SOURCE_DIR}/src/renderer)

set(GALAXY_INCLUDES ${PROJECT_SOURCE_DIR}/src/data ${PROJECT_SOURCE_DIR}/src/framework ${PROJECT_SOURCE_DIR}/src/renderer)

find_package(TBB REQUIRED COMPONENTS tbb)
include_directories(${TBB_INCLUDE_DIRS})

# find system-installed dependencies
find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# find system-installed dependencies

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(VTK REQUIRED NO_MODULE)
find_package(PNG REQUIRED)
# find_package(Python3 COMPONENTS Interpreter NumPy REQUIRED)
set(Python_ADDITIONAL_VERSIONS 3.6 3.7 3.8)
find_package(PythonInterp)

# find third-party local builds
# Assumption that these have been installed to single 'third-party/install' directory
# cmake -DCMAKE_INSTALL_PREFIX=${GXY_ROOT}/third-party/install/ ..

find_package(RapidJSON PATHS
    ${PROJECT_SOURCE_DIR}/third-party/install/lib64/cmake
    ${PROJECT_SOURCE_DIR}/third-party/install/lib/cmake REQUIRED)

# Cmake definitions to handle building ISPC files
include(ispc)

include_directories(${RapidJSON_INCLUDE_DIRS})

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH
  "${OSPRAY_ROOT}/${CMAKE_INSTALL_LIBDIR}"
  "${EMBREE_ROOT_DIR}/${CMAKE_INSTALL_LIBDIR}"
  "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
  "${VTK_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")
set(GALAXY_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
set(GALAXY_INCLUDE_DIRS "${CMAKE_INSTALL_INCLUDEDIR}")
set(GALAXY_CONFIG "${PROJECT_NAME}Config.cmake")
set(GALAXY_CONFIG_VERSION "${PROJECT_NAME}ConfigVersion.cmake")
set(GALAXY_FRAMEWORK_LIB "${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}gxy_framework${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(GALAXY_DATA_LIB "${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}gxy_data${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(GALAXY_RENDERER_LIB "${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}gxy_renderer${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(GALAXY_SAMPLER_LIB "${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}gxy_sampler${CMAKE_SHARED_LIBRARY_SUFFIX}")

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


write_basic_package_version_file("${GALAXY_CONFIG_VERSION}" COMPATIBILITY AnyNewerVersion)
configure_package_config_file( "CMake/${PROJECT_NAME}Config.cmake.in" 
  "${GALAXY_CONFIG}"
  PATH_VARS GALAXY_INCLUDE_DIR GALAXY_INCLUDE_DIRS GALAXY_FRAMEWORK_LIB GALAXY_DATA_LIB GALAXY_RENDERER_LIB GALAXY_SAMPLER_LIB
  INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}")

install(FILES "${CMAKE_BINARY_DIR}/${GALAXY_CONFIG}" "${CMAKE_BINARY_DIR}/${GALAXY_CONFIG_VERSION}"
  DESTINATION "${CONFIG_INSTALL_DIR}")

add_subdirectory(scripts)
add_subdirectory(src)
if (GXY_UNIT_TESTING)
  add_subdirectory(tests/src)
endif(GXY_UNIT_TESTING)

add_subdirectory(colormaps)

