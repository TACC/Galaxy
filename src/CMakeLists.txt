## ========================================================================== ##
## Copyright (c) 2014-2019 The University of Texas at Austin.                 ##
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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/galaxy.h.in ${CMAKE_CURRENT_BINARY_DIR}/galaxy.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/galaxy.h DESTINATION include/gxy)

add_subdirectory(ospray)
add_subdirectory(framework)
add_subdirectory(data)
add_subdirectory(renderer)
add_subdirectory(sampler)
add_subdirectory(multiserver)
add_subdirectory(apps)
add_subdirectory(cinema)
add_subdirectory(tests)




