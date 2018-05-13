CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(PVol)

option(WITH_CINEMA  			"Include cinema generator?" OFF)
option(WITH_STATE        	"Include simple renderer?" ON)
option(WITH_QT_VIEWER  		"Include QT-based viewer?" OFF)

find_package(MPI)

find_package(OpenGL REQUIRED)
find_package(VTK REQUIRED NO_MODULE)
include(${VTK_USE_FILE})

find_package(PNG REQUIRED)

option(PVOL_SYNCHRONOUS "synchronize on completion of RenderSets?" ON)
option(PRODUCE_STATUS_MESSAGES "produce periodic status massages" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(EVENT_TRACKING FALSE CACHE BOOL "show events")

set(THREADPOOL11_INSTALLDIR "" CACHE FILEPATH "install directory of THREADPOOL11")
link_directories(${THREADPOOL11_INSTALLDIR}/lib ${THREADPOOL11_INSTALLDIR}/lib64)
include_directories(${THREADPOOL11_INSTALLDIR}/include)

# set(OSPRAY_SRCDIR "" CACHE FILEPATH "source directory of OSPRAY")
# if (OSPRAY_SRCDIR STREQUAL "")
  # message(SEND_ERROR "OSPRAY_SRCDIR is required")
# endif()

find_package(ospray)
find_package(embree 2.17.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPRAY_CMAKE_ROOT})

include(macros)
include(ospray_macros)
include(ispc)

include_directories(${OSPRAY_INCLUDE_DIR} ${OSPRAY_INCLUDE_DIR}/ospray ${OSPRAY_INCLUDE_DIR}/ospray/SDK ${embree_DIR}/include)
include_directories_ispc(${OSPRAY_INCLUDE_DIR} ${OSPRAY_INCLUDE_DIR}/ospray ${OSPRAY_INCLUDE_DIR}/ospray/SDK ${embree_DIR}/include)

# include_directories(${OSPRAY_SRCDIR} ${OSPRAY_INCLUDE_DIR} ${OSPRAY_INCLUDE_DIR}/ospray ${OSPRAY_SRCDIR}/ospray ${embree_DIR}/include)
# include_directories_ispc(${OSPRAY_SRCDIR} ${OSPRAY_INCLUDE_DIR} ${OSPRAY_INCLUDE_DIR}/ospray ${OSPRAY_SRCDIR}/ospray ${embree_DIR}/include)

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH 
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
