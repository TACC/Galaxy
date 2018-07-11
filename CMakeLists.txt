CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(Galaxy)

if (COMMAND cmake_policy)
	# use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)
endif(COMMAND cmake_policy)

# use C++11 foo explicitly
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
IF (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF (APPLE)

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



option(WITH_CINEMA  			"Include cinema generator?" OFF)
option(WITH_STATE        	"Include simple renderer?" ON)
option(WITH_QT_VIEWER  		"Include QT-based viewer?" OFF)

find_package(MPI)

find_package(VTK REQUIRED NO_MODULE)
include(${VTK_USE_FILE})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(EVENT_TRACKING FALSE CACHE BOOL "show events")

find_path(THREADPOOL11_LIB_DIR libthreadpool11.a PATHS ${PROJECT_SOURCE_DIR}/third-party/threadpool11/install/lib64
													   ${PROJECT_SOURCE_DIR}/third-party/threadpool11/install/lib
											     DOC "library install directory for threadpool11 (recommend third-party/threadpool11/install/lib)"
											     )
find_path(THREADPOOL11_INCLUDE_DIR threadpool11/threadpool11.hpp PATHS ${PROJECT_SOURCE_DIR}/third-party/threadpool11/install/include
	 															 DOC "include install directory for threadpool11 (recommend third-party/threadpool11/install/include)")
link_directories(${THREADPOOL11_LIB_DIR})
include_directories(${THREADPOOL11_INCLUDE_DIR})

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

  include_directories(${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
  include_directories_ispc(${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
else(ospray_DIR)
  message(FATAL_ERROR "OSPRay directory not found. Please make sure you have done a 'git submodule init' and built threadpool11, Embree and OSPRay.")
endif(ospray_DIR)

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH 
  "${CMAKE_INSTALL_PREFIX}/lib"
  "${OSPRAY_ROOT}/lib"
  "${EMBREE_ROOT_DIR}/lib"
  "${CMAKE_INSTALL_PREFIX}/ospray/lib"
  "${CMAKE_INSTALL_PREFIX}/threadpool11/lib")


add_subdirectory(src)

