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

#pragma once

/*! \file dtypes.h 
 * \brief useful datatypes used by Galaxy components
 * \ingroup data
 */

#include <math.h>

namespace gxy
{

//! a vector of two floats
/*! \ingroup data */
struct vec2f { 
	vec2f() {};
	vec2f(float X, float Y) : x(X), y(Y) {};
	vec2f(float *xy) : x(xy[0]), y(xy[1]) {};
	bool operator==(const vec2f& o) { return (x == o.x) && (y == o.y); }
	bool operator!=(const vec2f& o) { return (x != o.x) || (y != o.y); }
	float x, y;
};

//! a vector of two integers
/*! \ingroup data */
struct vec2i { 
	vec2i() {};
	vec2i(int X, int Y) : x(X), y(Y) {};
	vec2i(int *xy) : x(xy[0]), y(xy[1]) {};
	bool operator==(const vec2i& o) { return (x == o.x) && (y == o.y); }
	bool operator!=(const vec2i& o) { return (x != o.x) || (y != o.y); }
	int x, y;
};

//! a vector of three floats
/*! \ingroup data */
struct vec3f { 
	vec3f() {};
	vec3f(float X, float Y, float Z) : x(X), y(Y), z(Z) {};
	vec3f(float *xyz) : x(xyz[0]), y(xyz[1]), z(xyz[2]) {};
  float operator*(const vec3f& o) { return x * o.x + y * o.y + z * o.z; }
  vec3f operator*(const float& o) { return vec3f(x * o, y * o,  z * o); }
  vec3f operator+(const vec3f& o) { return vec3f(x + o.x, y + o.y, z + o.z); }
  vec3f operator-(const vec3f& o) { return vec3f(x - o.x, y - o.y, z - o.z); }
	bool operator==(const vec3f& o) { return (x == o.x) && (y == o.y) && (z == o.z); }
	bool operator!=(const vec3f& o) { return (x != o.x) || (y != o.y) || (z != o.z); }
	float x, y, z;
};

//! a vector of three integers
/*! \ingroup data */
struct vec3i {
	vec3i() {};
	vec3i(int X, int Y, int Z) : x(X), y(Y), z(Z) {};
	vec3i(int *xyz) : x(xyz[0]), y(xyz[1]), z(xyz[2]) {};
	bool operator==(const vec3i& o) { return (x == o.x) && (y == o.y) && (z == o.z); }
	bool operator!=(const vec3i& o) { return (x != o.x) || (y != o.y) || (z != o.z); }
	int x, y, z;
};

//! a vector of four floats
/*! \ingroup data */
struct vec4f {
	vec4f() {};
	vec4f(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) {};
	vec4f(float *xyzw) : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) {};
	bool operator==(const vec4f& o) { return (x == o.x) && (y == o.y) && (z == o.z) && (w == o.w); }
	bool operator!=(const vec4f& o) { return (x != o.x) || (y != o.y) || (z != o.z) || (w != o.w); }
	float x, y, z, w;
};

//! a vector of four integers
/*! \ingroup data */
struct vec4i {
	vec4i() {};
	vec4i(int X, int Y, int Z, int W) : x(X), y(Y), z(Z), w(W) {};
	vec4i(int *xyzw) : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) {};
	bool operator==(const vec4i& o) { return (x == o.x) && (y == o.y) && (z == o.z) && (w == o.w); }
	bool operator!=(const vec4i& o) { return (x != o.x) || (y != o.y) || (z != o.z) || (w != o.w); }
	int x, y, z, w;
};

/*! \ingroup data
 * @{
 */
//! set all vector elements to zero
inline void  zero(vec4f& a) { a.x = 0; a.y = 0; a.z = 0; a.w = 0; }
//! set the given vector's elements to the given values
inline void  set(vec4f& a, float x, float y, float z, float w) { a.x = x; a.y = y; a.z = z; a.w = w; }
//! copy a given vector element values into another vector
/* \param s source vector
 * \param d destination vector
 */
inline void  copyvec(vec4f& s, vec4f& d) { d.x = s.x; d.y = s.y; d.z = s.z; d.w = s.w; }
//! scale a given vector by the given factor
/* \param s scaling factor
 * \param d destination vector
 */
inline void  scale(float s, vec4f& d) { d.x *= s; d.y *= s; d.z *= s; d.w *= s; }
//! a - b = r
inline void  sub(vec4f& a, vec4f& b, vec4f& r) { r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w; }
//! a + b = r
inline void  add(vec4f& a, vec4f& b, vec4f& r) { r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; r.w = a.w + b.w; }
//! return the length of a given vector
inline float len(vec4f& a) { return sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }
//! destructively normalize the given vector, with check for divide by zero
inline void  normalize(vec4f& a) { float d = len(a); if (d != 0) { d = 1.0 / d; a.x *= d; a.y *= d; a.z *= d; a.w *= d; }}

//! set all vector elements to zero
inline void  zero(vec3f& a) { a.x = 0; a.y = 0; a.z = 0; }
//! set the given vector's elements to the given values
inline void  set(vec3f& a, float x, float y, float z) { a.x = x; a.y = y; a.z = z; }
//! copy a given vector element values into another vector
/* \param s source vector
 * \param d destination vector
 */
inline void  copyvec(vec3f& s, vec3f& d) { d.x = s.x; d.y = s.y; d.z = s.z; }
//! scale a given vector by the given factor
/* \param s scaling factor
 * \param d destination vector
 */
inline void  scale(float s, vec3f& d) { d.x *= s; d.y *= s; d.z *= s; }
//! a - b = r
inline void  sub(vec3f& a, vec3f& b, vec3f& r) { r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; }
//! a + b = r
inline void  add(vec3f& a, vec3f& b, vec3f& r) { r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; }
//! return the length of a given vector
inline float len(vec3f& a) { return sqrt(a.x*a.x + a.y*a.y + a.z*a.z); }
//! destructively normalize the given vector, with check for divide by zero
inline void  normalize(vec3f& a) { float d = len(a); if (d != 0) { d = 1.0 / d; a.x *= d; a.y *= d; a.z *= d; }}

//! set all vector elements to zero
inline void  zero(vec2f& a) { a.x = 0; a.y = 0; }
//! set the given vector's elements to the given values
inline void  set(vec2f& a, float x, float y) { a.x = x; a.y = y; }
//! copy a given vector element values into another vector
/* \param s source vector
 * \param d destination vector
 */
inline void  copyvec(vec2f& s, vec2f& d) { d.x = s.x; d.y = s.y; }
//! scale a given vector by the given factor
/* \param s scaling factor
 * \param d destination vector
 */
inline void  scale(float s, vec2f& d) { d.x *= s; d.y *= s; }
//! a - b = r
inline void  sub(vec2f& a, vec2f& b, vec2f& r) { r.x = a.x - b.x; r.y = a.y - b.y; }
//! a + b = r
inline void  add(vec2f& a, vec2f& b, vec2f& r) { r.x = a.x + b.x; r.y = a.y + b.y; }
//! return the length of a given vector
inline float len(vec2f& a) { return sqrt(a.x*a.x + a.y*a.y); }
//! destructively normalize the given vector, with check for divide by zero
inline void  normalize(vec2f& a) { float d = len(a); if (d != 0) { d = 1.0 / d; a.x *= d; a.y *= d; }}
/*! }@ */

//! calculate A x B = R, where R is a 3-element vector.
/*! Note that for 4 element vector types, only the first three elements are used.
 * \warning For vec4f types, R.w is undefined.
 * \ingroup data
 */
#define CROSS(T0, T1, T2) inline void cross(T0 & a, T1 & b, T2 & r) { r.x = a.y*b.z - a.z*b.y; r.y = a.z*b.x - a.x*b.z; r.z = a.x*b.y - a.y*b.x; } 
//! calculate A x B = R, where R is a 3-element vector.
/*! Note that for 4 element vector types, only the first three elements are used.
 * \warning For vec4f types, R.w is undefined.
 * \ingroup data
 */
#define CROSS1(T0, T1, T2) inline T2 cross(T0 & a, T1 & b) { T2 r; cross(a, b, r); return r; } 

CROSS(vec3f, vec3f, vec3f)
CROSS(vec3f, vec3f, vec4f)
CROSS(vec3f, vec4f, vec3f)
CROSS(vec3f, vec4f, vec4f)
CROSS(vec4f, vec3f, vec3f)
CROSS(vec4f, vec3f, vec4f)
CROSS(vec4f, vec4f, vec3f)
CROSS(vec4f, vec4f, vec4f)

CROSS1(vec3f, vec3f, vec3f)
CROSS1(vec4f, vec4f, vec4f)

//! calculate A * B = R, where R is a 3-element vector.
/*! Note that for 4 element vector types, only the first three elements are used.
 * \warning For vec4f types, R.w is undefined.
 * \ingroup data
 */
#define DOT(T0, T1) inline float dot(T0 & a, T1 & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
DOT(vec3f, vec3f)
DOT(vec3f, vec4f)
DOT(vec4f, vec3f)
DOT(vec4f, vec4f)

} // namespace gxy
