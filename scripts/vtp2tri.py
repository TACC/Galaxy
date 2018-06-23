import sys
import numpy
import vtk 
import numpy as np
from vtk.numpy_interface import dataset_adapter as dsa 

inpt = sys.argv[1]

r = vtk.vtkXMLPolyDataReader()
r.SetFileName(inpt);
r.Update()
ply = dsa.WrapDataObject(r.GetOutput())
pts = ply.Points.flatten().astype('f4')
normals = ply.PointData["Normals"].flatten().astype('f4')
tris = ply.Polygons.reshape((-1,4))[:,1:].flatten().astype('i4')
sizes = np.array([pts.shape[0]/3, tris.shape[0]/3]).astype('i4')

f = open(inpt.rsplit('.')[0] + '.tri', 'wb')
sizes.tofile(f)
tris.tofile(f)
pts.tofile(f)
normals.tofile(f)
f.close()

print "pts", pts
print "tris", tris
print "normals", normals

