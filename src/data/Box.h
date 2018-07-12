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
#include <math.h>
#include "string.h"
#include "dtypes.h"
#include "float.h"


using namespace std;

namespace gxy
{
class Ray;

class Box
{
	friend ostream& operator<<(ostream& o, const Box& b)
	{
		o << "X " << b.xyz_min.x << " -- " << b.xyz_max.x << "\n";
		o << "Y " << b.xyz_min.y << " -- " << b.xyz_max.y << "\n";
		o << "Z " << b.xyz_min.z << " -- " << b.xyz_max.z << "\n";
		return o;
	}

public:
	Box() : initialized(false) {}
	Box(float *o, int *n, float *d);
	Box(vec3f m, vec3f M);
	
	~Box();

	void copy(Box &o) { xyz_min = o.xyz_min; xyz_max = o.xyz_max; initialized = true; }

	bool operator==(Box& b)
	{
		return ((xyz_min == b.xyz_min) && (xyz_max == b.xyz_max));
	};

	bool operator!=(const Box& b)
	{
		return 
			(fabs(xyz_min.x - b.xyz_min.x) > 0.0001) ||
			(fabs(xyz_min.y - b.xyz_min.y) > 0.0001) ||
			(fabs(xyz_min.z - b.xyz_min.z) > 0.0001) ||
			(fabs(xyz_max.x - b.xyz_max.x) > 0.0001) ||
			(fabs(xyz_max.y - b.xyz_max.y) > 0.0001) ||
			(fabs(xyz_max.z - b.xyz_max.z) > 0.0001);

	};

	void add(Box& b)
	{
		if (! initialized)
			this->copy(b);
		else
		{
			if (b.xyz_min.x < xyz_min.x) xyz_min.x = b.xyz_min.x;
			if (b.xyz_min.y < xyz_min.y) xyz_min.y = b.xyz_min.y;
			if (b.xyz_min.z < xyz_min.z) xyz_min.z = b.xyz_min.z;
			if (b.xyz_max.x > xyz_max.x) xyz_max.x = b.xyz_max.x;
			if (b.xyz_max.y > xyz_max.y) xyz_max.y = b.xyz_max.y;
			if (b.xyz_max.z > xyz_max.z) xyz_max.z = b.xyz_max.z;
		}
	}

	void set(float *o, int *n, float *d);
	void set(vec3f m, vec3f M);

	int exit_face(float x, float y, float z, float dx, float dy, float dz);

	bool intersect(vec3f& o, vec3f& d, float& tmin, float& tmax);
	bool intersect(float x, float y, float z, float dx, float dy, float dz, float& tmin, float& tmax);

	void get_min(float &x, float &y, float &z) 
	{
		x = xyz_min.x;
		y = xyz_min.y;
		z = xyz_min.z;
	}

	void get_max(float &x, float &y, float &z)
	{
		x = xyz_max.x;
		y = xyz_max.y;
		z = xyz_max.z;
	}

	bool is_initialized() { return initialized; }

	void center(vec3f& c)
	{
		c.x = 0.5 * (xyz_min.x + xyz_max.x);
		c.y = 0.5 * (xyz_min.y + xyz_max.y);
		c.z = 0.5 * (xyz_min.z + xyz_max.z);
	}

	float *get_min()
	{
		return (float *)&xyz_min;
	};

	float *get_max()
	{
		return (float *)&xyz_max;
	};

  vec3f center()
  {
    vec3f c;
    center(c);
    return c;
  }

	float diag()
	{
		float dx = xyz_max.x - xyz_min.x;
		float dy = xyz_max.y - xyz_min.y;
		float dz = xyz_max.z - xyz_min.z;
		return sqrt(dx*dx + dy*dy + dz*dz);
	}

	vec3f corner(int i)
	{
		vec3f c;
		c.x = (i & 0x1) ? xyz_min.x : xyz_max.x;
		c.y = (i & 0x2) ? xyz_min.y : xyz_max.y;
		c.z = (i & 0x4) ? xyz_min.z : xyz_max.z;
		return c;
	}

  bool isIn(vec3f p)
  {
    return (p.x >= xyz_min.x) && (p.x <= xyz_max.x) &&
           (p.y >= xyz_min.y) && (p.y <= xyz_max.y) &&
           (p.z >= xyz_min.z) && (p.z <= xyz_max.z);
  }

private:
	bool initialized;
	vec3f xyz_min;
	vec3f xyz_max;
};
} // namespace gxy