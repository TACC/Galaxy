#! /bin/env python 
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


import sys
import json
import xmltodict

if len(sys.argv) != 3:
		print "syntax: camera.xml  camera.json"
		sys.exit(0)

with open(sys.argv[1], 'r') as f:
    xmlString = f.read()
 
jsonString = json.dumps(xmltodict.parse(xmlString), indent=4)
 
with open(sys.argv[2], 'w') as f:
    f.write(jsonString)
