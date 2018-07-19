## ========================================================================== ##
## Copyright (c) 2014-2018 The University of Texas at Austin.                 ##
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(Galaxy)

set(GALAXY_MAJOR_VER 0)
set(GALAXY_MINOR_VER 9)
set(GALAXY_PATCH_VER 0)
set(GALAXY_VERSION ${GALAXY_MAJOR_VER}.${GALAXY_MINOR_VER}.${GALAXY_PATCH_VER})
set(GALAXY_SOVERSION ${GALAXY_MAJOR_VER})

if (COMMAND cmake_policy)
	# use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # interpret if args as vars only if unquoted
  cmake_policy(SET CMP0054 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)
endif(COMMAND cmake_policy)

# use C++11 foo explicitly
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ignore override warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")

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
set(GXY_SYNCHRONOUS OFF CACHE BOOL "synchronize on completion of RenderSets?")
set(GXY_TIMING OFF CACHE BOOL "Generate timing statistics for the Galaxy Renderer")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(GALAXY_INCLUDES ${PROJECT_SOURCE_DIR}/src/data ${PROJECT_SOURCE_DIR}/src/framework ${PROJECT_SOURCE_DIR}/src/renderer)

# find system-installed dependencies
find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
find_package(TBB REQUIRED)
include_directories(${TBB_INCLUDE_DIRS})
find_package(OpenGL REQUIRED)
find_package(VTK REQUIRED NO_MODULE)
include(${VTK_USE_FILE})
find_package(PNG REQUIRED)

# find third-party directory dependencies
find_package(RapidJSON PATHS
  ${PROJECT_SOURCE_DIR}/../third-party/rapidjson/install/lib64/cmake
  ${PROJECT_SOURCE_DIR}/../third-party/rapidjson/install/lib/cmake
  ${PROJECT_SOURCE_DIR}/../third-party/rapidjson/build
  ${PROJECT_SOURCE_DIR}/third-party/rapidjson/install/lib64/cmake
  ${PROJECT_SOURCE_DIR}/third-party/rapidjson/install/lib/cmake
  ${PROJECT_SOURCE_DIR}/third-party/rapidjson/build)
include_directories(${RapidJSON_INCLUDE_DIRS})

find_package(embree 3 PATHS 
  ${PROJECT_SOURCE_DIR}/../third-party/embree/install/lib64/cmake
  ${PROJECT_SOURCE_DIR}/../third-party/embree/install/lib/cmake
  ${PROJECT_SOURCE_DIR}/../third-party/embree/build
  ${PROJECT_SOURCE_DIR}/third-party/embree/install/lib64/cmake
	${PROJECT_SOURCE_DIR}/third-party/embree/install/lib/cmake
	${PROJECT_SOURCE_DIR}/third-party/embree/build)
find_package(ospray PATHS 
	${PROJECT_SOURCE_DIR}/../third-party/OSPRay/install/lib64/cmake
  ${PROJECT_SOURCE_DIR}/../third-party/OSPRay/install/lib/cmake
  ${PROJECT_SOURCE_DIR}/../third-party/OSPRay/build/cmake
  ${PROJECT_SOURCE_DIR}/third-party/ospray/install/lib64/cmake
  ${PROJECT_SOURCE_DIR}/third-party/ospray/install/lib/cmake
	${PROJECT_SOURCE_DIR}/third-party/ospray/build/cmake)

if (ospray_DIR)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPRAY_CMAKE_ROOT})
  include(macros)
  include(ospray_macros)
  include(ispc)

  include_directories(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
  include_directories_ispc(${GALAXY_INCLUDES} ${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
else(ospray_DIR)
  message(FATAL_ERROR "OSPRay not found. Please run ${PROJECT_SOURCE_DIR}/prep-third-party.sh and build the third-party dependencies before building Galaxy.")
endif(ospray_DIR)

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH 
  "${OSPRAY_ROOT}/lib"
  "${EMBREE_ROOT_DIR}/lib"
  "${CMAKE_INSTALL_PREFIX}/lib"
  "${CMAKE_INSTALL_PREFIX}/ospray/lib")

add_subdirectory(scripts)
add_subdirectory(src)


