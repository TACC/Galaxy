#pragma once

#include "dtypes.h" 

void add_quats(vec4f q1, vec4f q2, vec4f& dest);
void axis_to_quat(vec3f a, float phi, vec4f& q);
void rotate_vector_by_quat(vec3f v, vec4f q, vec3f& vq);
void trackball(vec4f& q, float p1x, float p1y, float p2x, float p2y);

