// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#pragma once

#include <iostream>
#include "dtypes.h" 

#include <boost/math/quaternion.hpp>
#include <cmath>

typedef boost::math::quaternion<double> quaternion;

namespace gxy
{

class Q
{
public:
	Q() { zero(); }
	Q(quaternion q) : q(q) {}
	Q(vec3f axis) { q = quaternion(0.0, axis.x, axis.y, axis.z); }
	Q(vec3f u, float r) { q =  quaternion(cos(r/2), sin(r/2)*u.x, sin(r/2)*u.y, sin(r/2)*u.z); }

	~Q() {}

	Q operator*(const Q& q) const
	{
		return Q(this->q * q.q);
	}

	void print(std::ostream& o)
	{
	}

	vec3f rotate(vec3f v)
	{
		quaternion qr = this->q * quaternion(0.0, v.x, v.y, v.z) * conj(this->q);
		return vec3f( qr.R_component_2(), qr.R_component_3(),qr.R_component_4());
	}

	void zero() { q = quaternion(1.0, 0.0, 0.0, 0.0); }

	quaternion get() { return q; }

private:
	quaternion q;
};

class Trackball
{
public:
	Trackball(float s = 1) : size(s) {}
	~Trackball() {}

	void spin(float p1x, float p1y, float p2x, float p2y)
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
    normalize(axis);

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

		current = current * Q(axis, phi);

		return;
	}

	vec3f rotate_vector(vec3f v)
	{
		quaternion qr = current.get() * quaternion(0.0, v.x, v.y, v.z) * conj(current.get());
		return vec3f( qr.R_component_2(), qr.R_component_3(),qr.R_component_4());
	}

	Q get() { return current; }

	void reset() { current.zero(); }

private:
	Q current;
	float size;

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
};

}
