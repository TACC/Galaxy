#
# make a two level amr dataset with spherical based data
# this makes level 0
#
import numpy as np
level0dims = np.array([32,32,32])
level0lower = np.array([-0.5,-0.5,-0.5])
level0upper = np.array([0.5,0.5,0.5])
spherecenter = np.array([0.25,0.25,0.25])
level0spacing = (level0upper - level0lower)/(level0dims-1)
outfilename = "level0sphere.vtk"
dimstring = level0dims[0].astype(str) +" "+ level0dims[1].astype(str) + " " + level0dims[2].astype(str) 
originstring = level0lower[0].astype(str) + " " + level0lower[1].astype(str) + " " + level0lower[2].astype(str)
spacingstring = level0spacing[0].astype(str) + " " + level0spacing[1].astype(str) + " " + level0spacing[2].astype(str)
numpoints = level0dims[0]*level0dims[1]*level0dims[2]
#
# write a vtk structured points dataset file with this level in it
#
with open(outfilename,'w') as f:
    f.write("# vtkDataFileVersion 3.0\n")
    f.write("level 0 sphere data\n")
    f.write("ASCII\n")
    f.write("DATASET STRUCTURED_POINTS\n")
    f.write("DIMENSIONS " + dimstring + "\n")
    f.write("ORIGIN " + originstring + "\n")
    f.write("SPACING " + spacingstring + "\n")
    f.write("POINT_DATA " + str(numpoints) + "\n")
    f.write("SCALARS volume_data float 1 " + "\n")
    f.write("LOOKUP_TABLE default \n")
    for i in range(level0dims[0]):
        for j in range(level0dims[1]):
            for k in range(level0dims[2]):
                pointindex = np.array([i,j,k])
                pointcoordinate = level0lower + pointindex*level0spacing
                spheretopoint = pointcoordinate - spherecenter
                r = np.dot(spheretopoint,spheretopoint)
                scalar = 1.0/np.sqrt(r)
                f.write(str(scalar) + " " )
            f.write("\n")
