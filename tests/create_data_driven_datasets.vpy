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
from vtk import *
from vtk.numpy_interface import dataset_adapter as dsa
from math import sqrt
import numpy as np

def SetScalars(d, name):
  pd = d.GetPointData()
  old_scalars = None
  if pd.GetScalars() != None:
    old_scalar_name = pd.GetScalars().GetName()
    if old_scalar_name == name:
      return
    else:
      old_scalars = pd.GetScalars()
  new_scalars = pd.GetArray(name)
  pd.SetScalars(new_scalars)
  if old_scalars != None:
    pd.AddArray(old_scalars)

def SetVectors(d, name):
  pd = d.GetPointData()
  old_vectors = None
  if pd.GetVectors() != None:
    old_vector_name = pd.GetVectors().GetName()
    if old_vector_name == name:
      return
    else:
      old_vectors = pd.GetVectors()
  new_vectors = pd.GetArray(name)
  pd.SetVectors(new_vectors)
  if old_vectors != None:
    pd.AddArray(old_vectors)

def Names(d):
  pd = d.GetPointData()
  if pd.GetScalars() != None:
    print('scalars:', pd.GetScalars().GetName())
  if pd.GetVectors() != None:
    print('vectors:', pd.GetVectors().GetName())
  for i in range(pd.GetNumberOfArrays()):
    print(i, ": ", pd.GetArray(i).GetName())

def do_particles(id):
  SetScalars(id, 'eightBalls')

  ct = vtkContourFilter()
  ct.SetInputData(id)
  ct.SetValue(0, 0.3)
  af = vtkAppendFilter()
  af.SetInputConnection(ct.GetOutputPort())
  af.Update()

  contour1 = af.GetOutput()

  w = vtkXMLUnstructuredGridWriter()
  w.SetFileName('eightBalls-points-contour.vtu')
  w.SetInputData(contour1)
  w.Write()

  nPoints = 2000
  pSkip = int(float(contour1.GetNumberOfPoints()) / nPoints)
  if pSkip < 1:
     pSkip = 1
     nPoints = contour1.GetNumberOfPoints()

  vtx = vtkVertex()
  vtx.GetPointIds().SetNumberOfIds(1)

  ca = vtkCellArray()
  for i in range(nPoints):
    vtx.GetPointIds().SetId(0, i*pSkip)
    ca.InsertNextCell(vtx)

  contour1.SetCells(VTK_VERTEX, ca)
  contour1.GetCellData().Initialize()

  gf = vtkGeometryFilter()
  gf.SetInputData(contour1)
  cpd = vtkCleanPolyData()
  cpd.SetInputConnection(gf.GetOutputPort())
  af = vtkAppendFilter()
  af.SetInputConnection(cpd.GetOutputPort())
  af.Update()

  points = af.GetOutput()
  SetScalars(points, 'oneBall')

  wrtr = vtkXMLUnstructuredGridWriter()
  wrtr.SetInputData(points)
  wrtr.SetFileName('eightBalls-points.vtu')
  wrtr.Write()

def do_mesh(id):
  SetScalars(id, 'oneBall')

  ct = vtkContourFilter()
  ct.SetInputData(id)
  ct.SetValue(0, 1.4)
  af = vtkAppendFilter()
  af.SetInputConnection(ct.GetOutputPort())
  af.Update()

  contour2 = af.GetOutput()
  SetScalars(contour2, 'eightBalls')

  wrtr = vtkXMLUnstructuredGridWriter()
  wrtr.SetInputData(contour2)
  wrtr.SetFileName('oneBall-mesh.vtu')
  wrtr.Write()

def do_streamlines(id):
  ls = vtkLineSource()
  ls.SetPoint1([-0.7, -0.7, -0.90])
  ls.SetPoint2([ 0.7,  0.7, -0.90])
  ls.SetResolution(4)
  ls.Update()

  seeds = ls.GetOutput()

  SetVectors(id, 'vector')

  sl = vtkStreamTracer()
  sl.SetInputData(id)
  sl.SetSourceData(seeds)
  sl.SetMaximumNumberOfSteps(1000000)
  sl.SetMaximumPropagation(1000000)

  af = vtkAppendFilter()
  af.SetInputConnection(sl.GetOutputPort())
  af.Update()

  streamlines = af.GetOutput()
  SetScalars(streamlines, 'oneBall')

  wrtr = vtkXMLUnstructuredGridWriter()
  wrtr.SetFileName('streamlines.vtu')
  wrtr.SetInputData(streamlines)
  wrtr.Write()


rdr = vtkXMLImageDataReader()
rdr.SetFileName('radial.vti')
rdr.Update()
id = rdr.GetOutput()

do_streamlines(id)

n = 65
d = 2.0 / (n - 1)
xyz = [[-1 + d*x,-1 + d*y,-1 + d*z] for z in range(n) for y in range(n) for x in range(n)]

id = vtkImageData()
id.SetOrigin((-1, -1, -1))
id.SetDimensions([n]*3)
id.SetSpacing([d]*3)

oneBall    = np.array([sqrt(sum([i*i for i in a])) for a in xyz]).astype('f4')
eightBalls = np.array([sqrt(sum([(abs(i)-0.5) * (abs(i)-0.5) for i in a])) for a in xyz]).astype('f4')
vectors    = np.array([[-a[1], a[0], 0.1] for a in xyz]).astype('f4')

oneBall    = dsa.numpyTovtkDataArray(oneBall)
eightBalls = dsa.numpyTovtkDataArray(eightBalls)
vectors    = dsa.numpyTovtkDataArray(vectors)

oneBall.SetName('oneBall')
id.GetPointData().AddArray(oneBall)

eightBalls.SetName('eightBalls')
id.GetPointData().AddArray(eightBalls)

vectors.SetName('vectors')
id.GetPointData().AddArray(vectors)

do_mesh(id)
do_particles(id)



