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

/*! \file Box.h 
 * \brief an axis-aligned bounding box
 * \ingroup data
 */

#include <iostream>
#include <math.h>
#include "string.h"
#include "dtypes.h"
#include "float.h"

namespace gxy
{
	
//! an axis-aligned bounding box 
/*! \ingroup data */
class Box
{
	friend std::ostream& operator<<(std::ostream& o, const Box& b)
	{
		o << "X " << b.xyz_min.x << " -- " << b.xyz_max.x << std::endl;
		o << "Y " << b.xyz_min.y << " -- " << b.xyz_max.y << std::endl;
		o << "Z " << b.xyz_min.z << " -- " << b.xyz_max.z << std::endl;
		return o;
	}

public:
	Box() : initialized(false) {} //!< default constructor
	//! construct a box with given origin and axes
	/*! initialization uses `min = o` and `max = o + (n-1)*d` 
	 * \param o box origin, maps to `min` corner
	 * \param n `xyz` array of number of points per axis
	 * \param d `xyz` array of step size between points
	 */
	Box(float *o, int *n, float *d);
	//! construct a box with the given min and max points
	/*! \param m box min point in `xyz` form
	 * \param M box max point in `xyz` form
	 */
	Box(vec3f m, vec3f M);
	//! construct a box with the given min and max point components
	Box(float minx, float miny, float minz, float maxx, float maxy, float maxz);
	//! construct a box with the given min and max points
	/*! \param p point array with form `minx miny minz maxx maxy maxz`	 */
	Box(float *p);
	
	~Box(); //!< default destructor

	//! copy a Box extent to this Box
	void copy(Box &o) { xyz_min = o.xyz_min; xyz_max = o.xyz_max; initialized = true; }

	//! equality operator, evaluates exact floating-point match
	bool operator==(Box& b)
	{
		return ((xyz_min == b.xyz_min) && (xyz_max == b.xyz_max));
	};

	//! inequality operator, evaluates floating-point delta ` > 0.0001`
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

	//! add a Box's extent to this Box
	/*! If this Box is not yet initialized, copies the Box to this Box.
	 * Otherwise, this Box extent is increased if necessary to include the Box's extent
	 */
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

	//! set the extent of this Box
	/*! initialization uses `min = o` and `max = o + (n-1)*d` 
	 * \param o box origin, maps to `min` corner
	 * \param n `xyz` array of number of points per axis
	 * \param d `xyz` array of step size between points
	 */
	void set(float *o, int *n, float *d);
	//! set the extent of this Box
	/*! \param m box min point in `xyz` form
	 * \param M box max point in `xyz` form
	 */
	void set(vec3f m, vec3f M);

	//! computes the exit face for a given vector
	/*! for a vector origin at `x,y,z` and direction of `dx,dy,dz`
	 * \returns the face of this Box where the vector will exit, where:
	 *          - exits yz-face - `0` if `dx < 0` or `1` if `dx >= 0`
	 * 					- exits xz-face - `2` if `dy < 0` or `3` if `dy >= 0`
	 *          - exits xy-face - `4` if `dz < 0` or `5` if `dz >= 0` 
	 * \warning assumes vector origin is in box. If vector origin is outside box, 
	 *          this will return `4` if `dz < 0` or `5` if `dz >= 0`. This returns
	 *          `5` if `dx = dy = dz = 0`.
	 */
	int exit_face(float x, float y, float z, float dx, float dy, float dz);

	//! does the given vector intersects this Box?
	/*! \returns true if the vector intersects the box, false otherwise.
	 *           If true, `tmin` and `tmax` contain the distance along the vector for the
	 *           min and max intersection. If the vector origin is within the Box, `tmin = 0`.
	 */
	bool intersect(vec3f& o, vec3f& d, float& tmin, float& tmax);
	//! does the given vector intersects this Box?
	/*! \returns true if the vector intersects the box, false otherwise.
	 *           If true, `tmin` and `tmax` contain the distance along the vector for the
	 *           min and max intersection. If the vector origin is within the Box, `tmin = 0`.
	 */
	bool intersect(float x, float y, float z, float dx, float dy, float dz, float& tmin, float& tmax);

	//! get the min point for this Box
	/* \returns copies the min point values into the supplied floats */
	void get_min(float &x, float &y, float &z) 
	{
		x = xyz_min.x;
		y = xyz_min.y;
		z = xyz_min.z;
	}

	//! get the max point for this Box
	/* \returns copies the max point values into the supplied floats */
	void get_max(float &x, float &y, float &z)
	{
		x = xyz_max.x;
		y = xyz_max.y;
		z = xyz_max.z;
	}

	//! has this Box been initialized?
	bool is_initialized() { return initialized; }

	//! get the center point for this Box
	/* \returns copies the center point values into the supplied `vec3f` */
	void center(vec3f& c)
	{
		c.x = 0.5 * (xyz_min.x + xyz_max.x);
		c.y = 0.5 * (xyz_min.y + xyz_max.y);
		c.z = 0.5 * (xyz_min.z + xyz_max.z);
	}

	//! get the min point for this Box
	/* \returns a pointer to the min value array for this Box */
	float *get_min()
	{
		return (float *)&xyz_min;
	};

	//! get the max point for this Box
	/* \returns a pointer to the max value array for this Box */
	float *get_max()
	{
		return (float *)&xyz_max;
	};

	//! get the center point for this Box
	/* \returns a `vec3f` containing the center point values */	
  vec3f center()
  {
    vec3f c;
    center(c);
    return c;
  }

  //! return the diagonal distance of this Box
	float diag()
	{
		float dx = xyz_max.x - xyz_min.x;
		float dy = xyz_max.y - xyz_min.y;
		float dz = xyz_max.z - xyz_min.z;
		return sqrt(dx*dx + dy*dy + dz*dz);
	}

	//! return a `vec3f` containing the requested corner point for this Box
	/*! \param i a bitmask indicating which corner to return, where:
	 *         - `0x1` requests `min.x`, otherwise return `max.x`
	 *         - `0x2` requests `min.y`, otherwise return `max.y`
	 *         - `0x4` requests `min.z`, otherwise return `max.z`
	 * \returns the requested corner point. 
	 *          For example, passing `0x0` returns the max corner point,
	 *          and passing `0x7` returns the mix corner point.
	 */
	vec3f corner(int i)
	{
		vec3f c;
		c.x = (i & 0x1) ? xyz_min.x : xyz_max.x;
		c.y = (i & 0x2) ? xyz_min.y : xyz_max.y;
		c.z = (i & 0x4) ? xyz_min.z : xyz_max.z;
		return c;
	}

	//! is the given point inside this Box?
  bool isIn(vec3f p)
  {
    return (p.x >= xyz_min.x) && (p.x <= xyz_max.x) &&
           (p.y >= xyz_min.y) && (p.y <= xyz_max.y) &&
           (p.z >= xyz_min.z) && (p.z <= xyz_max.z);
  }

	vec3f xyz_min; //!< the min point for this Box
	vec3f xyz_max; //!< the max point for this Box

private:
	bool initialized;
};

} // namespace gxy

