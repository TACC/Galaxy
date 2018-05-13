#include "quat.h"
#include <math.h>

namespace pvol
{

#define TRACKBALLSIZE 0.8


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


void axis_to_quat(vec3f a, float phi, vec4f& q)
{
    normalize(a);
    scale(sin(phi/2.0), a);
    set(q, a.x, a.y, a.z, cos(phi/2.0));
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
    float dot_uv = dot(q, v);
    float dot_uu = dot(q, q);
    float s      = q.w;
    float ss     = s*s;

    vec3f cross_uv;
    cross(q, v, cross_uv);

    float a = 2.0 * dot(q, v);
    float b = ss - dot(q, q);
    float c = 2 * s;

    vq.x = (a * q.x) + (b * v.x) + (c * cross_uv.x);
    vq.y = (a * q.y) + (b * v.y) + (c * cross_uv.y);
    vq.z = (a * q.z) + (b * v.z) + (c * cross_uv.z);

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

    /*
     *  Now, we want the cross product of P1 and P2
     */
    cross(p2,p1,a);
    normalize(a);

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
