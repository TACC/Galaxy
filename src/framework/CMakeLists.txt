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

cmake_minimum_required (VERSION 3.9)
cmake_policy(SET CMP0048 NEW)
project (gxy_framework VERSION ${GALAXY_VERSION})

if (COMMAND cmake_policy)
	# use rpath on MacOS
  cmake_policy(SET CMP0042 NEW)

  # interpret if args as vars only if unquoted
  cmake_policy(SET CMP0054 NEW)

  # ignore rpath setting effects for certain vars under MacOS
  cmake_policy(SET CMP0068 NEW)
endif(COMMAND cmake_policy)

include_directories(
			${gxy_framework_SOURCE_DIR}
			${gxy_data_SOURCE_DIR}
			${gxy_renderer_SOURCE_DIR}
			${Galaxy_BINARY_DIR}/src)


set (CPP_SOURCES 
			Application.cpp
			Threading.cpp
			ClientServer.cpp
			Events.cpp
			GalaxyObject.cpp
			Message.cpp
			MessageManager.cpp 
			MessageQ.cpp 
			smem.cpp
			Work.cpp)

add_library(gxy_framework SHARED ${CPP_SOURCES})
target_link_libraries(gxy_framework  ${MPI_LIBRARIES} ${TBB_LIBRARIES})
set_target_properties(gxy_framework PROPERTIES VERSION ${GALAXY_VERSION} SOVERSION ${GALAXY_SOVERSION})
install(TARGETS gxy_framework DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES 
	Application.h 
	ClientServer.h  
	Threading.h
	Debug.h 
	Events.h
	framework.h
    GalaxyObject.h
	Message.h 
	MessageManager.h 
	MessageQ.h 
	smem.h 
	Timer.h 
	Work.h 
	DESTINATION include/gxy)
