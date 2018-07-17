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

#include "Box.h"

using namespace std;

namespace gxy
{
Box::Box(float *o, int *n, float *d)
{
	set(o, n, d);
}

Box::Box(vec3f m, vec3f M)
{
	set(m, M);
}

Box::Box(float minx, float miny, float minz, float maxx, float maxy, float maxz)
{
	xyz_min.x = minx;
	xyz_min.y = miny;
	xyz_min.z = minz;

	xyz_max.x = maxx;
	xyz_max.y = maxy;
	xyz_max.z = maxz;
}

Box::Box(float *p)
{
	xyz_min.x = *p++;
	xyz_min.y = *p++;
	xyz_min.z = *p++;

	xyz_max.x = *p++;
	xyz_max.y = *p++;
	xyz_max.z = *p++;
}

void
Box::set(vec3f m, vec3f M)
{
	initialized = true;

	xyz_min = m;
	xyz_max = M;
}

void
Box::set(float *o, int *n, float *d)
{
	initialized = true;

	xyz_min.x = o[0];
	xyz_min.y = o[1];
	xyz_min.z = o[2];

	xyz_max.x = xyz_min.x + (n[0]-1)*d[0];
	xyz_max.y = xyz_min.y + (n[1]-1)*d[1];
	xyz_max.z = xyz_min.z + (n[2]-1)*d[2];
}

Box::~Box() {};

int
Box::exit_face(float x, float y, float z, float dx, float dy, float dz)
{
	float tx = (dx > 0.0001) ? ((xyz_max.x - x) / dx) : (dx < -0.0001) ? ((xyz_min.x - x) / dx) : FLT_MAX;
	float ty = (dy > 0.0001) ? ((xyz_max.y - y) / dy) : (dy < -0.0001) ? ((xyz_min.y - y) / dy) : FLT_MAX;
	float tz = (dz > 0.0001) ? ((xyz_max.z - z) / dz) : (dz < -0.0001) ? ((xyz_min.z - z) / dz) : FLT_MAX;
	if (tx < 0) tx = FLT_MAX;
	if (ty < 0) ty = FLT_MAX;
	if (tz < 0) tz = FLT_MAX;
	if (tx < ty && tx < tz) return (dx < 0) ? 0 : 1;
	else if (ty < tz) return (dy < 0) ? 2 : 3;
	else return (dz < 0) ? 4 : 5;
}

bool
Box::intersect(float x, float y, float z, float dx, float dy, float dz, float& tmin, float& tmax)
{
  vec3f org = vec3f(x, y, z);
  vec3f dir = vec3f(dx, dy, dz);
  return intersect(org, dir, tmin, tmax);
}

bool
Box::intersect(vec3f& org, vec3f& dir, float& tmin, float& tmax)
{
	tmin = (xyz_min.x - org.x) / dir.x; 
	tmax = (xyz_max.x - org.x) / dir.x; 

	if (tmin > tmax) swap(tmax, tmin); 
  if (tmax < 0) return false;

	float tymin = (xyz_min.y - org.y) / dir.y; 
	float tymax = (xyz_max.y - org.y) / dir.y; 

	if (tymin > tymax) swap(tymax, tymin); 
  if (tymax < 0) return false;

	if ((tmin > tymax) || (tymin > tmax)) 
			return false; 

	if (tymin > tmin) 
			tmin = tymin; 

	if (tymax < tmax) 
			tmax = tymax; 

	float tzmin = (xyz_min.z - org.z) / dir.z; 
	float tzmax = (xyz_max.z - org.z) / dir.z; 

	if (tzmin > tzmax) swap(tzmin, tzmax); 
  if (tzmax < 0) return false;

	if ((tmin > tzmax) || (tzmin > tmax)) 
			return false; 

	if (tzmin > tmin) 
			tmin = tzmin; 

	if (tzmax < tmax) 
			tmax = tzmax; 

	if (tmin < 0) 
		tmin = 0;

	return true;
}

} // namespace gxy
