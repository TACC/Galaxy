import sys
import json
from vtk import *
from vtk.numpy_interface import dataset_adapter as dsa
from math import sqrt
import numpy as np

rdr = vtkXMLImageDataReader()

rdr.SetFileName('radial-0.vti')
rdr.Update()
id = rdr.GetOutput()

o = id.GetOrigin()
d = id.GetDimensions()
s = id.GetSpacing()

extent = [o[0]+s[0], o[0] + (d[0]-2)*s[0], o[1]+s[1], o[1] + (d[1]-2)*s[1], o[2]+s[2], o[2] + (d[2]-2)*s[2]]

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
    print 'scalars:', pd.GetScalars().GetName()
  if pd.GetVectors() != None:
    print 'vectors:', pd.GetVectors().GetName()
  for i in range(pd.GetNumberOfArrays()):
    print i, ": ", pd.GetArray(i).GetName()

SetScalars(id, 'eightBalls')

ct = vtkContourFilter()
ct.SetInputData(id)
ct.SetValue(0, 0.3)
af = vtkAppendFilter()
af.SetInputConnection(ct.GetOutputPort())
af.Update()

contour1 = af.GetOutput()

nPoints = 2000
pSkip = int(contour1.GetNumberOfPoints() / nPoints)
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

f = open('eightBalls-points.part', 'w')
f.write(json.dumps({'parts': [{'extent': extent, 'filename': 'eightBalls-points.vtu'}]}))
f.close()

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

f = open('oneBall-mesh.part', 'w')
f.write(json.dumps({'parts': [{'extent': extent, 'filename': 'oneBall-mesh.vtu'}]}))
f.close()

ls = vtkLineSource()
ls.SetPoint1([-0.7, -0.7, -0.99])
ls.SetPoint2([ 0.7,  0.7, -0.99])
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

f = open('streamlines.part', 'w')
f.write(json.dumps({'parts': [{'extent': extent, 'filename': 'streamlines.vtu'}]}))
f.close()
