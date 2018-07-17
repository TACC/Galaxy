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
import numpy
import vtk 
import numpy as np
from vtk.numpy_interface import dataset_adapter as dsa 
from mpi4py import MPI
from os.path import isfile

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

inpt = sys.argv[1]

reader = vtk.vtkXMLPolyDataReader()

i = -1
for fname in open(inpt):
  i = i + 1
  if i % size == rank:
    ifile = fname.strip()
    ofile = ifile.rsplit('.')[0] + '.tri'
    if isfile(ofile):
      print 'skipping ',  ifile
    else:
      print 'doing ',  ifile
      
      reader.SetFileName(ifile);
      reader.Update()
      ply = dsa.WrapDataObject(reader.GetOutput())
      tris = ply.Polygons.reshape((-1,4))[:,1:].flatten().astype('i4')
      print tris[:16]
      pts = ply.Points.flatten().astype('f4')
      print pts[:16]
      normals = ply.PointData["Normals"].flatten().astype('f4')
      print normals[:16]
      sizes = np.array([pts.shape[0]/3, tris.shape[0]/3]).astype('i4')
      f = open(ofile.rsplit('.')[0] + '.tri', 'wb')
      sizes.tofile(f)
      tris.tofile(f)
      pts.tofile(f)
      normals.tofile(f)
      f.close()
