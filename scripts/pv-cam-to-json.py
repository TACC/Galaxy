#! /bin/env python 

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
