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

cmake_minimum_required (VERSION 2.8.11)
project (gxy_framework)

include_directories(
			${gxy_framework_SOURCE_DIR}
			${gxy_data_SOURCE_DIR}
			${gxy_renderer_SOURCE_DIR}
			${Galaxy_BINARY_DIR}/src)


set (CPP_SOURCES 
			Application.cpp
			ClientServer.cpp
			debug.cpp
			Events.cpp
			KeyedObject.cpp
			Message.cpp
			MessageManager.cpp 
			MessageQ.cpp 
			smem.cpp
			Work.cpp)

if (ospray_DIR)
  ospray_create_library(gxy_framework ${CPP_SOURCES} LINK ${OSPRAY_LIBRARIES} ${TBB_LIBRARIES} ${MPI_LIBRARIES} COMPONENT lib) 
  set_target_properties(gxy_framework PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
  set_target_properties(gxy_framework PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
  install(TARGETS gxy_framework DESTINATION lib)
endif()

install(FILES 
	Application.h 
	ClientServer.h  
	debug.h 
	Events.h
	KeyedObject.h
	Message.h 
	MessageManager.h 
	MessageQ.h 
	smem.h 
	Timer.h 
	Work.h 
	DESTINATION include/gxy)
