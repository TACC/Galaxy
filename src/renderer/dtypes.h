#pragma once
#include <math.h>

namespace gxy
{
struct vec2f { 
	vec2f() {};
	vec2f(float X, float Y) : x(X), y(Y) {};
	vec2f(float *xy) : x(xy[0]), y(xy[1]) {};
	bool operator==(const vec2f& o) { return (x == o.x) && (y == o.y); }
	bool operator!=(const vec2f& o) { return (x != o.x) || (y != o.y); }
	float x, y;
};

struct vec2i { 
	vec2i() {};
	vec2i(int X, int Y) : x(X), y(Y) {};
	vec2i(int *xy) : x(xy[0]), y(xy[1]) {};
	bool operator==(const vec2i& o) { return (x == o.x) && (y == o.y); }
	bool operator!=(const vec2i& o) { return (x != o.x) || (y != o.y); }
	int x, y;
};

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

struct vec3i {
	vec3i() {};
	vec3i(int X, int Y, int Z) : x(X), y(Y), z(Z) {};
	vec3i(int *xyz) : x(xyz[0]), y(xyz[1]), z(xyz[2]) {};
	bool operator==(const vec3i& o) { return (x == o.x) && (y == o.y) && (z == o.z); }
	bool operator!=(const vec3i& o) { return (x != o.x) || (y != o.y) || (z != o.z); }
	int x, y, z;
};

struct vec4f {
	vec4f() {};
	vec4f(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) {};
	vec4f(float *xyzw) : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) {};
	bool operator==(const vec4f& o) { return (x == o.x) && (y == o.y) && (z == o.z) && (w == o.w); }
	bool operator!=(const vec4f& o) { return (x != o.x) || (y != o.y) || (z != o.z) || (w != o.w); }
	float x, y, z, w;
};

struct vec4i {
	vec4i() {};
	vec4i(int X, int Y, int Z, int W) : x(X), y(Y), z(Z), w(W) {};
	vec4i(int *xyzw) : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) {};
	bool operator==(const vec4i& o) { return (x == o.x) && (y == o.y) && (z == o.z) && (w == o.w); }
	bool operator!=(const vec4i& o) { return (x != o.x) || (y != o.y) || (z != o.z) || (w != o.w); }
	int x, y, z, w;
};

inline void  zero(vec4f& a) { a.x = 0; a.y = 0; a.z = 0; a.w = 0; }
inline void  set(vec4f& a, float x, float y, float z, float w) { a.x = x; a.y = y; a.z = z; a.w = w; }
inline void  copyvec(vec4f& s, vec4f& d) { d.x = s.x; d.y = s.y; d.z = s.z; d.w = s.w; }
inline void  scale(float s, vec4f& d) { d.x *= s; d.y *= s; d.z *= s; d.w *= s; }
inline void  sub(vec4f& a, vec4f& b, vec4f& r) { r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w; }
inline void  add(vec4f& a, vec4f& b, vec4f& r) { r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; r.w = a.w + b.w; }
inline float len(vec4f& a) { return sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }
inline void  normalize(vec4f& a) { float d = len(a); if (d != 0) { d = 1.0 / d; a.x *= d; a.y *= d; a.z *= d; a.w *= d; }}

inline void  zero(vec3f& a) { a.x = 0; a.y = 0; a.z = 0; }
inline void  set(vec3f& a, float x, float y, float z) { a.x = x; a.y = y; a.z = z; }
inline void  copyvec(vec3f& s, vec3f& d) { d.x = s.x; d.y = s.y; d.z = s.z; }
inline void  scale(float s, vec3f& d) { d.x *= s; d.y *= s; d.z *= s; }
inline void  sub(vec3f& a, vec3f& b, vec3f& r) { r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; }
inline void  add(vec3f& a, vec3f& b, vec3f& r) { r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; }
inline float len(vec3f& a) { return sqrt(a.x*a.x + a.y*a.y + a.z*a.z); }
inline void  normalize(vec3f& a) { float d = len(a); if (d != 0) { d = 1.0 / d; a.x *= d; a.y *= d; a.z *= d; }}

inline void  zero(vec2f& a) { a.x = 0; a.y = 0; }
inline void  set(vec2f& a, float x, float y) { a.x = x; a.y = y; }
inline void  copyvec(vec2f& s, vec2f& d) { d.x = s.x; d.y = s.y; }
inline void  scale(float s, vec2f& d) { d.x *= s; d.y *= s; }
inline void  sub(vec2f& a, vec2f& b, vec2f& r) { r.x = a.x - b.x; r.y = a.y - b.y; }
inline void  add(vec2f& a, vec2f& b, vec2f& r) { r.x = a.x + b.x; r.y = a.y + b.y; }
inline float len(vec2f& a) { return sqrt(a.x*a.x + a.y*a.y); }
inline void  normalize(vec2f& a) { float d = len(a); if (d != 0) { d = 1.0 / d; a.x *= d; a.y *= d; }}

#define CROSS(T0, T1, T2) inline void cross(T0 & a, T1 & b, T2 & r) { r.x = a.y*b.z - a.z*b.y; r.y = a.z*b.x - a.x*b.z; r.z = a.x*b.y - a.y*b.x; } 
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

#define DOT(T0, T1) inline float dot(T0 & a, T1 & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
DOT(vec3f, vec3f)
DOT(vec3f, vec4f)
DOT(vec4f, vec3f)
DOT(vec4f, vec4f)
}