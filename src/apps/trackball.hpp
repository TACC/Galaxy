#pragma once

#include <iostream>
#include "dtypes.h" 
using namespace pvol;

#include <boost/math/quaternion.hpp>
#include <cmath>

typedef boost::math::quaternion<double> quaternion;

namespace pvol
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
