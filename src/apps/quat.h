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

#if 0
#pragma once

#include "dtypes.h" 

namespace pvol
{

class Q
{
public:
	Q(vec3f v, float a)
	{
		angle = v.x*sin(a/2);
		vector.x = v.y*sin(a/2);
		vector.y = v.z*sin(a/2);
		vector.z = cos(a/2);
	}

	~Q() {}

	static Q identity() { return Q(vec3f(0, 0, 0), 0); }

	Q operator*(const Q&) const
	{
		Q r;
		r.x = q1.x*q2.x - q1.y*q2.y - q1.z*q2.z - q1.w*q2.w;
		r.y = q1.x*q2.y + q1.y*q2.x + q1.z*q2.w - q1.w*q2.z;
		r.z = q1.x*q2.z - q1.y*q2.w + q1.z*q2.x + q1.w*q2.y;
		r.w = q1.x*q2.w + q1.y*q2.z - q1.z*q2.y + q1.w*q2.x;
    return r;
	}

	void print(ostream o)
	{
		o << "hello";
	}

	vec3f rotate(vec3f v)
	{
		vec3f vq;

		float dot_uv = dot(vector, v);
		float dot_uu = dot(vector, vector);
		float ss     = angle*angle;

		vec3f cross_uv;
		cross(vector, v, cross_uv);

		float a = 2.0 * dot_uv;
		float b = ss - dot_uu;
		float c = 2 * s;

    vq.x = (a * vector.x) + (b * v.x) + (c * cross_uv.x);
    vq.y = (a * vector.y) + (b * v.y) + (c * cross_uv.y);
    vq.z = (a * vector.z) + (b * v.z) + (c * cross_uv.z);

		float d = 1.0 / sqrt(vq.x*vq.x + vq.y*vq.y + vq.z*vq.z);
    vq.x *= d;
    vq.y *= d;
    vq.z *= d;

		return vq;
	}

private:
	vec3f vector;
	float angle;
}
	

class Trackball
{
public:
	Trackball(float s = 1) : size(s) {}
	~Trackball(); 

	Q rotate(float p1x, float p1y, float p2x, float p2y)
	{
    vec3f axis;      /* Axis of rotation */
    float phi;       /* how much to rotate about axis */
    vec3f p1, p2, d;
    float t;

    if (p1x == p2x && p1y == p2y)
			return;

    /*
     * First, figure out z-coordinates for projection of P1 and P2 to
     * deformed sphere
     */
    p1 = vec3f(p1x, p1y, project_to_sphere(size,p1x,p1y));
    p2 = vec3f(p2x, p2y, project_to_sphere(size,p2x,p2y));

    /*
     *  Now, we want the cross product of P1 and P2
     */
    cross(p2,p1,axis);
    normalize(a);

    /*
     *  Figure out how much to rotate around that axis.
     */
    sub(p1,p2,d);
    t = len(d) / (2.0*size);

    /*
     * Avoid problems with out-of-control values...
     */
    if (t > 1.0) t = 1.0;
    if (t < -1.0) t = -1.0;
    phi = 2.0 * asin(t);

		Q q_new = Q(axis, phi);
		current = current * q_new;
		
		return current;
	}

	Q get() { return current; }

private:
	Q current;

	float project_to_sphere(float r, float x, float y)
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
}
#endif
