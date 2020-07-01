#! /usr/bin/env python3
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

import sys
import json
import re

def recur(f, keys, info, values):
  k = keys[0]
  i = info[0]
  for v in i['values']:
    if len(keys) > 1:
      recur(f, keys[1:], info[1:], values + [v])
    else:
      f.write(','.join(values + [v]) + ',' + 'image/' + (template % tuple(values + [v])) + '\n')

for DB in sys.argv[1:]:

  f = open(DB + '/image/info.json', 'r')
  j = json.load(f)
  f.close()

  a = j['arguments']
  keys = a.keys()
  items = [a[i] for i in keys]

  template = re.sub('{[^}]*}', '%s', j['name_pattern'])

  f = open(DB + '/data.csv', 'w')
  f.write(','.join(keys + ['FILE']) + '\n')
  recur(f, keys, items, [])
  f.close()

