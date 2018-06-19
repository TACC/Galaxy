CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(PVol)

#if (COMMAND cmake_policy)
  # use rpath on MacOS
  # cmake_policy(SET CMP0042 NEW)
  # cmake_policy(SET CMP0054 NEW)
  # cmake_policy(SET CMP0068 NEW)
#endif(COMMAND cmake_policy)

# use C++11 foo explicitly
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
IF (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF (APPLE)

execute_process(COMMAND ${PROJECT_SOURCE_DIR}/threadpool.sh ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})

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
find_package(Boost)

include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
find_package(VTK REQUIRED NO_MODULE)
include(${VTK_USE_FILE})

find_package(PNG REQUIRED)

option(PVOL_SYNCHRONOUS "synchronize on completion of RenderSets?" ON)
option(PRODUCE_STATUS_MESSAGES "produce periodic status massages" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(EVENT_TRACKING FALSE CACHE BOOL "show events")

find_path(THREADPOOL11_LIB_DIR libthreadpool11.a PATHS ${PROJECT_BINARY_DIR}/third-party/threadpool11/lib
											 DOC "library install directory for threadpool11 (recommend ${PROJECT_BINARY_DIR}/threadpool11/lib)")

find_path(THREADPOOL11_INCLUDE_DIR threadpool11/threadpool11.hpp PATHS 
											 ${PROJECT_BINARY_DIR}/third-party/threadpool11/include
											 DOC "include install directory for threadpool11 (recommend ${PROJECT_BINARY_DIR}/threadpool11/include)")

link_directories(${THREADPOOL11_LIB_DIR})
include_directories(${THREADPOOL11_INCLUDE_DIR})


# add third-party directory to ISPC search path
SET(ISPC_VERSION_WORKING "1.9.2" "1.9.1")
IF (APPLE)
  SET(ISPC_DIR_SUFFIX "osx")
ELSEIF(WIN32)
  SET(ISPC_DIR_SUFFIX "windows")
  IF (MSVC_VERSION LESS 1900)
    LIST(APPEND ISPC_DIR_SUFFIX "windows-vs2013")
  ELSE()
    LIST(APPEND ISPC_DIR_SUFFIX "windows-vs2015")
  ENDIF()
ELSE()
  SET(ISPC_DIR_SUFFIX "linux")
ENDIF()
FOREACH(ver ${ISPC_VERSION_WORKING})
  FOREACH(suffix ${ISPC_DIR_SUFFIX})
    LIST(APPEND ISPC_DIR_HINT ${PROJECT_SOURCE_DIR}/third-party/ispc/ispc-v${ver}-${suffix})
  ENDFOREACH()
ENDFOREACH()


find_package(embree 2.16.0 PATHS
                 ${PROJECT_SOURCE_DIR}/../third-party/embree/install/lib64/cmake
                 ${PROJECT_SOURCE_DIR}/../third-party/embree/install/lib/cmake
                 ${PROJECT_SOURCE_DIR}/../third-party/embree/build
                 ${PROJECT_SOURCE_DIR}/third-party/embree/install/lib64/cmake
                 ${PROJECT_SOURCE_DIR}/third-party/embree/install/lib/cmake
                 ${PROJECT_SOURCE_DIR}/third-party/embree/build
                 )
find_package(ospray PATHS
								${PROJECT_SOURCE_DIR}/../third-party/OSPRay/install/lib64/cmake
								${PROJECT_SOURCE_DIR}/../third-party/OSPRay/install/lib/cmake
								${PROJECT_SOURCE_DIR}/../third-party/OSPRay/build/cmake
								${PROJECT_SOURCE_DIR}/third-party/ospray/install/lib64/cmake
								${PROJECT_SOURCE_DIR}/third-party/ospray/install/lib/cmake
								${PROJECT_SOURCE_DIR}/third-party/ospray/build/cmake
								)


if (ospray_DIR)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPRAY_CMAKE_ROOT})
	include(macros)
	include(ospray_macros)
	include(ispc)
  include_directories(${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
  include_directories_ispc(${OSPRAY_INCLUDE_DIRS} ${EMBREE_INCLUDE_DIRS})
else(ospray_DIR)
  message(FATAL_ERROR "OSPRay directory not found")
endif(ospray_DIR)

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH 
  "${OSPRAY_ROOT}/lib"
  "${EMBREE_ROOT_DIR}/lib"
  "${CMAKE_INSTALL_PREFIX}/lib"
  "${CMAKE_INSTALL_PREFIX}/ospray/lib"
  "${CMAKE_INSTALL_PREFIX}/threadpool11/lib")

set(OSPRAY_DIR "" CACHE FILEPATH "install directory of OSPRAY")
link_directories(${OSPRAY_DIR}/lib ${OSPRAY_DIR}/lib64)
include_directories(${OSPRAY_DIR}/include)

add_subdirectory(scripts)
add_subdirectory(src)

configure_file(pvol.h.in pvol.h)
install(FILES ${PROJECT_BINARY_DIR}/pvol.h DESTINATION include)
