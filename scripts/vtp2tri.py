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
      pts = ply.Points.flatten().astype('f4')
      normals = ply.PointData["Normals"].flatten().astype('f4')
      tris = ply.Polygons.reshape((-1,4))[:,1:].flatten().astype('i4')
      sizes = np.array([pts.shape[0]/3, tris.shape[0]/3]).astype('i4')
      f = open(ofile.rsplit('.')[0] + '.tri', 'wb')
      sizes.tofile(f)
      tris.tofile(f)
      pts.tofile(f)
      normals.tofile(f)
      f.close()
