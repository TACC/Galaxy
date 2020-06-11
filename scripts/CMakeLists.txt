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

cmake_minimum_required(VERSION 2.8.7)
cmake_policy(SET CMP0048 NEW)
project(Util VERSION ${GALAXY_VERSION})
install(PROGRAMS partitionVTUs.vpy createPartitionDoc.py msdata mssample vtp2tri.py async-wrapper cm2gxy.py csv2xyz vti2vol vti2json pv-cam-to-json.py xml2json.py mk_simsim_layout DESTINATION bin)
if(APPLE)
	install(PROGRAMS dbg_script.macos DESTINATION scripts RENAME dbg_script)
	configure_file(galaxy.env.macos galaxy.env @ONLY)
elseif(UNIX)
	install(PROGRAMS dbg_script.linux DESTINATION scripts RENAME dbg_script)
	configure_file(galaxy.env.linux galaxy.env @ONLY)
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/galaxy.env DESTINATION ${CMAKE_INSTALL_PREFIX})
