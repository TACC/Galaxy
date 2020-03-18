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

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(NodeEditor PATHS
    ${PROJECT_SOURCE_DIR}/third-party/install/lib64/cmake
    ${PROJECT_SOURCE_DIR}/third-party/install/lib/cmake REQUIRED)

find_package(Qt5 5.3 COMPONENTS Core Widgets Gui OpenGL)

SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} ${NodeEditor_DIR}/../..)

file(GLOB_RECURSE CPPS  ./*.cpp )
file(GLOB_RECURSE HEADERS_TO_MOC ./*.hpp)

include_directories(/opt/x11/include
                    ${gxy_data_SOURCE_DIR}
                    ${gxy_multiserver_SOURCE_DIR}
                    ${gxy_renderer_SOURCE_DIR}
                    ${gxy_framework_SOURCE_DIR}
                    ${gxy_ospray_SOURCE_DIR}
                    ${Galaxy_BINARY_DIR}/src
                    ${OSPRAY_INCLUDE_DIRS}
                    ${EMBREE_INCLUDE_DIRS})

qt5_wrap_cpp(mocs ${HEADERS_TO_MOC} TARGET galaxygui OPTIONS --no-notes)

add_executable(galaxygui ${CPPS} ${mocs})
target_link_libraries(galaxygui NodeEditor::nodes gxy_multiserver_client ${OPENGL_LIBRARIES})
install(TARGETS galaxygui DESTINATION bin)
