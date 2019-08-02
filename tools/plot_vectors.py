import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import sys

LARGE =  100000
SMALL = -100000 

inputfile = sys.argv[1] 
data = []
xmin = LARGE
ymin = LARGE
zmin = LARGE
xmax = SMALL
ymax = SMALL
zmax = SMALL

def record_minmax(v):
    global xmin, xmax, ymin, ymax, zmin, zmax

    if (xmin > v[0]):
        xmin = v[0]
    if (xmin > v[3] + v[0]):
        xmin = v[3] + v[0]
    if (xmax < v[0]):
        xmax = v[0]
    if (xmax < v[3] + v[0]):
        xmax = v[3] + v[0]
    if (ymin > v[1]):
        ymin = v[1]
    if (ymin > v[4] + v[1]):
        ymin = v[4] + v[1]
    if (ymax < v[1]):
        ymax = v[1]
    if (ymax < v[4] + v[1]):
        ymax = v[4] + v[1]
    if (zmin > v[2]):
        zmin = v[2]
    if (zmin > v[5] + v[2]):
        zmin = v[5] + v[2]
    if (zmax < v[2]):
        zmax = v[2]
    if (zmax < v[5] + v[2]):
        zmax = v[5] + v[2]

with open(inputfile, "r") as vectors:
    for line in vectors:
        vector = [float(i) for i in line.strip().split(",")]
        data.append(vector)
        record_minmax(vector)

# print("x {},{}".format(xmin, xmax))
# print("y {},{}".format(ymin, ymax))
# print("z {},{}".format(zmin, zmax))

soa = np.array(data)

x, y, z, u, v, w = zip(*soa)
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.quiver(x, y, z, u, v, w, arrow_length_ratio = 0.2 )
ax.scatter(x, y, z, color="black")
epsilon = 1
ax.set_xlim([xmin - epsilon, xmax + epsilon])
ax.set_ylim([ymin - epsilon, ymax + epsilon])
ax.set_zlim([zmin - epsilon, zmax + epsilon])
plt.show()
