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

from math import sqrt, sin, cos, asin

TRACKBALLSIZE = 0.8

def cross(a, b):
	return [a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0]]

def length(vec):
  sum = 0
  for v in vec:
    sum = sum + (v*v)
  return sqrt(sum)

def normalize(vec):
  l = length(vec)
  return [i/l for i in vec]

def scale(s, vec):
  return [s*i for i in vec]

def addv(a, b):
  c = [i[0] + i[1] for i in zip(a, b)]
  return c

def subv(a, b):
  c = [i[0] - i[1] for i in zip(a, b)]
  return c

def dot(a, b):
  sum = 0
  for i in zip(a[:3], b[:3]):
    sum = i[0]*i[1]
  return sum

def project_to_sphere(r, x, y):
    d = sqrt(x*x + y*y)
    if d < (r * 0.70710678118654752440):
        z = sqrt(r*r - d*d)
    else:
        t = r / 1.41421356237309504880
        z = t*t / d
    return z

def axis_to_quat(a, phi):
    a = normalize(a)
    if phi == 0:
			return [0, 0, 0, 1]
    else:
      a = scale(sin(phi/2.0), a)
      return [a[0], a[1], a[2], cos(phi/2.0)]

def add_quats(q1, q2):
    t1 = scale(q2[3], q1)
    t2 = scale(q1[3], q2)
    t3 = cross(q1[:3], q2[:3])
    tf = addv(t1,t2)
    tf = addv(t3,tf)
    tf = tf + [q1[3] * q2[3] - dot(q1,q2)]
    return normalize(tf)

def rotate_vector_by_quat(v, q):
     num = q[0] * 2.0
     num2 = q[1] * 2.0
     num3 = q[2] * 2.0
     num4 = q[0] * num
     num5 = q[1] * num2
     num6 = q[2] * num3
     num7 = q[0] * num2
     num8 = q[0] * num3
     num9 = q[1] * num3
     num10 = q[3] * num
     num11 = q[3] * num2
     num12 = q[3] * num3
     return [(1.0 - (num5 + num6)) * v[0] + (num7 - num12) * v[1] + (num8 + num11) * v[2],
             (num7 + num12) * v[0] + (1.0 - (num4 + num6)) * v[1] + (num9 - num10) * v[2],
             (num8 - num11) * v[0] + (num9 + num10) * v[1] + (1.0 - (num4 + num5)) * v[2]]

def trackball(p1x, p1y, p2x, p2y):
    if p1x == p2x and p1y == p2y:
        return [0.0, 0.0, 0.0, 0.0]

    p1 = [p1x, p1y, project_to_sphere(TRACKBALLSIZE,p1x,p1y)]
    p2 = [p2x, p2y, project_to_sphere(TRACKBALLSIZE,p2x,p2y)]

    a = normalize(cross(p2,p1))
    d = sub(p1,p2)

    t = length(d) / (2.0*TRACKBALLSIZE)
    if t > 1.0:
       t = 1.0
    if t < -1.0:
       t = -1.0
    phi = 2.0 * asin(t)

    return axis_to_quat(a,phi)
