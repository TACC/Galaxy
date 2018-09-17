#
# make a two level amr dataset with spherical based data
# this makes level 1. Level 1 is a 2x refinement of level 0
# and it resides in (0,0,0) to (.5,.5,.5). Same sphere point
#
import numpy as np
level1dims = np.array([32,32,32])
level1lower = np.array([-0.0,-0.0,-0.0])
level1upper = np.array([0.5,0.5,0.5])
spherecenter = np.array([0.25,0.25,0.25])
level1spacing = (level1upper - level1lower)/(level1dims-1)
outfilename = "level1sphere.vtk"
dimstring = level1dims[0].astype(str) +" "+ level1dims[1].astype(str) + " " + level1dims[2].astype(str) 
originstring = level1lower[0].astype(str) + " " + level1lower[1].astype(str) + " " + level1lower[2].astype(str)
spacingstring = level1spacing[0].astype(str) + " " + level1spacing[1].astype(str) + " " + level1spacing[2].astype(str)
numpoints = level1dims[0]*level1dims[1]*level1dims[2]
#
# write a vtk structured points dataset file with this level in it
#
with open(outfilename,'w') as f:
    f.write("# vtk DataFile Version 3.0\n")
    f.write("level 1 sphere data\n")
    f.write("ASCII\n")
    f.write("DATASET STRUCTURED_POINTS\n")
    f.write("DIMENSIONS " + dimstring + "\n")
    f.write("ORIGIN " + originstring + "\n")
    f.write("SPACING " + spacingstring + "\n")
    f.write("POINT_DATA " + str(numpoints) + "\n")
    f.write("SCALARS volume_data float 1 " + "\n")
    f.write("LOOKUP_TABLE default \n")
    for i in range(level1dims[0]):
        for j in range(level1dims[1]):
            for k in range(level1dims[2]):
                pointindex = np.array([i,j,k])
                pointcoordinate = level1lower + pointindex*level1spacing
                spheretopoint = pointcoordinate - spherecenter
                r = np.dot(spheretopoint,spheretopoint)
                scalar = 1.0/np.sqrt(r)
                f.write(str(scalar) + " " )
            f.write("\n")
