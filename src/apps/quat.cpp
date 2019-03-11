// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#if 1
void ksdflksaf() {}
#else
#include <iostream>
#include "quat.h"
#include <math.h>

namespace gxy
{
#define TRACKBALLSIZE 2.0

static float
project_to_sphere(float r, float x, float y)
{
    float d, t, z;

    d = sqrt(x*x + y*y);
    if (d < r * 0.70710678118654752440)
        z = sqrt(r*r - d*d);
    else
		{
        t = r / 1.41421356237309504880;
        z = t*t / d;
    }
    return z;
}


void axis_to_quat(vec3f a, float theta, vec4f& q)
{
		std::cerr << "axis " << a.x << " " << a.y << " " << a.z << " theta " << theta << "\n";

		// set(q, cos(theta/2), a.x*sin(theta/2), a.y*sin(theta/2), a.z*sin(theta/2));

		float t = a.x*sin(theta/2);
		float x = a.y*sin(theta/2);
		float y = a.z*sin(theta/2);
		float z = cos(theta/2);

#if 0
		float d = sqrt(x*x + y*y + z*z);
		if (d > 0)
		{
			d = 1.0 / d;
			x *= d;
			y *= d;
			z *= d;
		}
#endif
			
		set(q, t, x, y, z);
		
		std::cerr << "q " << q.x << " " << q.y << " " << q.z << " " << q.w << "\n";
}

void multiply_quats(vec4f q1, vec4f q2, vec4f& dest)
{
	dest.x = q1.x*q2.x - q1.y*q2.y - q1.z*q2.z - q1.w*q2.w;
	dest.y = q1.x*q2.y + q1.y*q2.x + q1.z*q2.w - q1.w*q2.z;
	dest.z = q1.x*q2.z - q1.y*q2.w + q1.z*q2.x + q1.w*q2.y;
	dest.w = q1.x*q2.w + q1.y*q2.z - q1.z*q2.y + q1.w*q2.x;
}

void add_quats(vec4f q1, vec4f q2, vec4f& dest)
{
    vec4f t1, t2, t3;
    vec4f tf;

    t1 = q1;
    scale(q2.w, t1);

    t2 = q2;
    scale(q1.w, t2);

    cross(q2,q1,t3);
    add(t1,t2,tf);
    add(t3,tf,tf);
    tf.w = q1.w * q2.w - dot(q1,q2);

    dest = tf;
		normalize(dest);
}

void
rotate_vector_by_quat(vec3f v, vec4f q, vec3f& vq)
{
	vec3f u = {q.y, q.z, q.w};
	float s = q.x;

	float dot_uv = dot(u, v);
	float dot_uu = dot(u, u);
	float ss     = s*s;

	vec3f cross_uv;
	cross(u, v, cross_uv);

	float a = 2.0 * dot(u, v);
	float b = ss - dot(u, u);
	float c = 2 * s;

    vq.x = (a * u.x) + (b * v.x) + (c * cross_uv.x);
    vq.y = (a * u.y) + (b * v.y) + (c * cross_uv.y);
    vq.z = (a * u.z) + (b * v.z) + (c * cross_uv.z);

		float d = 1.0 / sqrt(vq.x*vq.x + vq.y*vq.y + vq.z*vq.z);
    vq.x *= d;
    vq.y *= d;
    vq.z *= d;

}

void
trackball(vec4f& q, float p1x, float p1y, float p2x, float p2y)
{
    vec3f a; /* Axis of rotation */
    float phi;  /* how much to rotate about axis */
    vec3f p1, p2, d;
    float t;

    if (p1x == p2x && p1y == p2y)
		{
        zero(q);
        return;
    }

    /*
     * First, figure out z-coordinates for projection of P1 and P2 to
     * deformed sphere
     */
    p1 = vec3f(p1x, p1y, project_to_sphere(TRACKBALLSIZE,p1x,p1y));
    p2 = vec3f(p2x, p2y, project_to_sphere(TRACKBALLSIZE,p2x,p2y));

std::cerr << "===============================\n";
std::cerr << "p1 " << p1.x << " " << p1.y << " " << p1.z << "\n";
std::cerr << "p2 " << p2.x << " " << p2.y << " " << p2.z << "\n";

    /*
     *  Now, we want the cross product of P1 and P2
     */
    cross(p2,p1,a);
    normalize(a);

std::cerr << "A: " << a.x << " " << a.y << " " << a.z << "\n";

    /*
     *  Figure out how much to rotate around that axis.
     */
    sub(p1,p2,d);
    t = len(d) / (2.0*TRACKBALLSIZE);

    /*
     * Avoid problems with out-of-control values...
     */
    if (t > 1.0) t = 1.0;
    if (t < -1.0) t = -1.0;
    phi = 2.0 * asin(t);

    axis_to_quat(a,phi,q);
}

}
#endif
