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

cmake_minimum_required(VERSION 2.8.7)
project(Util)
install(PROGRAMS vtp2tri.py async-wrapper csv2xyz vti2vol pv-cam-to-json.py DESTINATION bin)
install(PROGRAMS galaxy.env DESTINATION .)
if(APPLE)
	install(PROGRAMS dbg_script.macos DESTINATION scripts RENAME dbg_script)
	install(PROGRAMS galaxy.env.macos DESTINATION . RENAME galaxy.env)
elseif(UNIX)
	install(PROGRAMS dbg_script.linux DESTINATION scripts RENAME dbg_script)
	install(PROGRAMS galaxy.env.linux DESTINATION . RENAME galaxy.env)
endif()
