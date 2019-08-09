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

# Given a partition document (see createPartitionDoc.py) and a list of .vtus that
# contain triangle meshes, point sets, and pathline sets, partition them.   The
# result for, say, foo.vtu will consist of foo.part (a JSON file) and n foo_k.vtu's
# containing the data.

import sys
import json 
import numpy as np
from vtk import *
from vtk.numpy_interface import dataset_adapter as dsa

vtus = []
pdoc = ""
ghost = 0.1

args = sys.argv[1:]
while len(args) > 0:
  if args[0] == '-g': 
    ghost = float(argv[1])
    args = args[2:]
  elif pdoc == "":
    pdoc = args[0]
    args = args[1:]
  else:
    vtus.append(args[0])
    args = args[1:]

if pdoc == "" or len(vtus) == 0:
  print ('syntax: vtkpython', sys.argv[0], '[-g ghostsize] pdocfile vtu0 vtu1...')
  sys.exit(1)

import json 
import numpy as np
from vtk import *
from vtk.numpy_interface import dataset_adapter as dsa

f = open(pdoc)
j = json.load(f)
f.close()

boxes = [a['extent'] for a in j['parts']]

partitions = []
for b in boxes:
  partitions.append({'lower': [i - ghost for i in b[0:6:2]],
                     'upper': [i + ghost for i in b[1:6:2]]})

if len(boxes) == 1:

  for srcname in vtus:
    pname = srcname[:-4] + '.part'
    f = open(pname, "w")
    f.write(json.dumps({"parts": [{'extent': boxes[0], 'filename': srcname}]}))
    f.close()
  
else:

  for srcname in vtus:
    parts = []
  
    rdr = vtkXMLUnstructuredGridReader()
    rdr.SetFileName(srcname)
    rdr.Update()
    src = dsa.WrapDataObject(rdr.GetOutput())
  
    scalars = src.VTKObject.GetPointData().GetScalars()
    if scalars != None:
      scalarName = scalars.GetName()
    else:
      scalarName = ""
  
    nv = src.Cells[src.CellLocations]    # number of verts in each cell
    ns = src.CellLocations + 1           # index of first vertex in each cell
    ne = ns + nv                         # index one past the last vertex
  
    cells = [src.Cells[i:j] for i,j in zip(ns, ne)]
  
    bad_cell_types = False
    for ip,p in enumerate(partitions):
  
      upper = p['upper']
      lower = p['lower']
  
      octypes  = []
      oclocations = []
      ocells = []
      nxtp = 0
  
      # mapping of output points/cells to input point/cells
  
      cmap = [] 
      pmap = [99999999] * len(src.Points)
      for cindex, cell, ctype in zip(range(len(src.CellTypes)), cells, src.CellTypes):
  
        if ctype == VTK_POLY_LINE:      # if its a polyline...
  
          pts = src.Points[cell]    #   points comprising the poly line
  
          # Determine whether each point lies inside the partition
  
          insiders = np.apply_along_axis(np.all, 1, np.hstack((pts <= upper, pts >= lower)))
          linelen = len(insiders)
  
          # iterate along the polyline finding segments that shoule be included in partition
  
          offset = 0
          kk = insiders
          while len(kk) > 0:
            s = np.argmax(kk)                       # index of first insider in ramaining segments
            if not kk[s]: break                     # returns 0 if none; check that the addressed elt is True.  If not, segments are done
            n = np.argmax(np.logical_not(kk[s:]))   # index of first loser in remainder
            if n == 0: n = len(kk[s:]) + 1          # if its zero, went to the end of the string - last insider is last entry
            n = s + n - 1                           # otherwise, last insider
            s = offset + s                          # indices of first and last insider in original list
            n = offset + n                          # ...
            if s > 0: s = s - 1                     # add in one extra segment at either end (if there is one)
            if n < linelen-1 : n = n + 1            # ...
            segment_indices = cell[s:n+1]           # indices of segment vertices in input point-dep arrays
            offset = n + 1
            kk = insiders[offset:]
            for s in segment_indices:               # For each vertex along the segment, look at the map to see if its already
              if pmap[s] == 99999999:               # been added to the output.   If not, add it
                pmap[s] = nxtp
                nxtp = nxtp + 1
            octypes.append(ctype)                   # Add the segment 
            ocells.append([pmap[s] for s in segment_indices])
            cmap.append(cindex)
  
        elif ctype == VTK_LINE or VTK_TRIANGLE or ctype == VTK_QUAD:
  
          pts = src.Points[cell]
          # print ('PTs', pts)
          inside = np.any([(pt < upper).all() and (pt > lower).all() for pt in pts])
          # print ('inside', inside)
          if inside:
            # print ('cell', cell)
            octypes.append(ctype)
            for ic in cell:
              if pmap[ic] == 99999999:
                pmap[ic] = nxtp
                nxtp = nxtp + 1
            ocells.append([pmap[ic] for ic in cell])
            cmap.append(cindex)
            # print ('ocells', ocells)
            # print ('pmap', pmap)
  
        elif ctype == VTK_TRIANGLE_STRIP:      # if its a polyline...
          
          pts = src.Points[cell]    #   points comprising the triangle strip...
  
          # Determine whether each point lies inside the partition
  
          insiders = np.apply_along_axis(np.all, 1, np.hstack((pts <= upper, pts >= lower)))
          linelen = len(insiders)
  
          # iterate along the triangle strip finding segments that shoule be included in partition
  
          offset = 0
          kk = insiders
          while len(kk) > 0:
            s = np.argmax(kk)                       # index of first insider in ramaining segments
            if not kk[s]: break                     # returns 0 if none; check that the addressed elt is True.  If not, segments are done
            n = np.argmax(np.logical_not(kk[s:]))   # index of first loser in remainder
            if n == 0: n = len(kk[s:]) + 1          # if its zero, went to the end of the string - last insider is last entry
            n = s + n - 1                           # otherwise, last insider
            s = offset + s                          # indices of first and last insider in original list
            n = offset + n                          # ...
            if s > 0:                               # add up to two extra vertices at either end
              if s > 1: 
                s = s - 2                     
              else:
                s = s - 1                     
            if n < linelen-1:
              if n < linelen-2:
                n = n + 2
              else:
                n = n + 1
            segment_indices = cell[s:n+1]           # indices of segment vertices in input point-dep arrays
            offset = n + 1
            kk = insiders[offset:]
            for s in segment_indices:               # For each vertex along the segment, look at the map to see if its already
              if pmap[s] == 99999999:               # been added to the output.   If not, add it
                pmap[s] = nxtp
                nxtp = nxtp + 1
            octypes.append(ctype)
            ocells.append([pmap[s] for s in segment_indices])
            cmap.append(cindex)
          
        else:
          if bad_cell_types == False:
            print ('input contains cell types that are not handled')
            bad_cell_types = True
  
      # print ('pmap', pmap)
      # print ('cmap', cmap)
      # print ('ocells', ocells)
            
      # Now aggregate the output unstructured grid for this partition of this input unstructured grid...
  
      dst = dsa.WrapDataObject(vtkUnstructuredGrid())
      ct = np.array(octypes).astype('u1')
      cl = np.hstack(([0], np.cumsum([len(c) for c in ocells])[:-1]))
      ocells1 = []
      for i,j in zip([len(c) for c in ocells], ocells):
        ocells1 = ocells1 + [i] + j
  
      # print ('ct', ct)
      # print ('cl', cl)
      # print ('ocells1', ocells1)
      dst.SetCells(ct, cl, ocells1)
  
      # ipmap[k] is the index in the output array of the input vertex k.   First we invert the map so that ipmap[k]
      # is the index of the input vertex that goes into the k'th slot of the output, followed by 9999999 for every
      # input vertex that is not required.   Then we chop those off.
  
      # print ('pmap', pmap)
      ipmap = np.argsort(pmap)
      # print ('ipmap', ipmap)
      ipmap = ipmap[0:np.argmax(np.array(pmap)[ipmap])+1]
      # print ('ipmap', ipmap)
      dst.SetPoints(src.Points[ipmap])
      for n in src.PointData.keys():
        dst.PointData.append(src.PointData[n][ipmap], n)
  
      # cmap[k] is the index of the input cell that segment k was cut from
  
      for n in src.CellData.keys():
        dst.CellData.append(src.CellData[n][cmap], n)
      dst.CellData.append(dsa.VTKArray(np.array(cmap).astype('i4')), 'source cell')
  
      # And that should do it for this partition of the current source dataset
  
      ofname= '%s_%d.vtu' % (srcname[:-4], ip)
  
      vdst = dst.VTKObject
      if scalarName != "":
        vdst.GetPointData().SetScalars(vdst.GetPointData().GetArray(scalarName))
  
      wrtr = vtkXMLUnstructuredGridWriter()
      wrtr.SetFileName(ofname)
      wrtr.SetInputData(dst.VTKObject)
      wrtr.Write()
  
      parts.append({'extent': boxes[ip], 'filename': ofname})
  
      print ('%s done' % ofname)
  
    pname = srcname[:-4] + '.part'
    f = open(pname, "w")
    f.write(json.dumps({"parts": parts}, indent=2))
    f.close()
  
