#! /usr/bin/env python
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

# Given the specification of a volume dataset (either by specifying the origin, spacing, 
# and counts or by giving it a .vol file) and a number of processes that Galaxy will be
# useing, create a partition document that can be used (by partitionVTUs.vpy) to
# partition triangle meshes, point sets, and pathline sets appropriately.

import sys
import json

origin = -1
counts = 101
spacing = 0.01
nparts = -1
volfile = ""

def syntax():
  print "syntax:", sys.argv[0], "[-o origin] [-s spacing] [-c counts] [-v volfile] nparts"
  print "NOTE: the dataset is assumed herein to have the same origin counts and"
  print "spacing in each axis"
  sys.exit(1)

args = sys.argv[1:]
while len(args) > 0:
  if args[0] == '-o':
    origin = float(args[1])
    args = args[2:]
  elif args[0] == '-c':
    counts = int(args[1])
    args = args[2:]
  elif args[0] == '-s':
    spacing = float(args[1])
    args = args[2:]
  elif args[0] == '-v':
    volfile = args[1]
    args = args[2:]
  elif nparts == -1:
    nparts = int(args[0])
    args = args[1:]
  else:
    syntax()

if nparts == -1:
  syntax()

if len(volfile):
  f = open(volfile)
  f.readline()
  origin = float(f.readline().strip().split()[0])
  counts = int(f.readline().strip().split()[0])
  spacing = float(f.readline().strip().split()[0])
  f.close()
  

# If ijk is prime, then [1, 1, ijk] will be chosen, i+j+k == ijk+2

def factor(ijk):
  factors = [1]*3

  if ijk > 1:
    mm = ijk + 3;
    i = 1
    while i <= (ijk >> 1):
      jk = ijk / i;
      if ijk == (i * jk):
        j = 1
        while j <= (jk >> 1):
          k = jk / j;
          if jk == (j * k):
            m = i + j + k;
            if m < mm:
              mm = m;
              factors = [i, j, k]
          j = j + 1
      i = i + 1
  return factors

def partition(factors, origin, spacing, counts):
  boxes = []
  n = [(c-2) / f for c,f in zip(counts, factors)]
  counts = [i - 2 for i in counts]
  IJK = [[1 + i * n[j] for i in range(factors[j])] + [counts[j]] for j in range(3)]
  IJK = [zip(IJK[j][:-1], IJK[j][1:]) for j in range(3)]
  IJK = [[k,j,i] for i in IJK[2] for j in IJK[1] for k in IJK[0]]
  XYZ = [[[origin[i] + ijk[i][0]*spacing[i], origin[i] + ijk[i][1]*spacing[i]] for i in range(3)] for ijk in IJK]
  return XYZ

partitions = partition(factor(nparts), [origin]*3, [spacing]*3, [counts]*3)
partitions = [[p[0][0], p[0][1], p[1][0], p[1][1], p[2][0], p[2][1]] for p in partitions]

layout = {"parts": [{"extent": p} for p in partitions]}
print json.dumps(layout)


