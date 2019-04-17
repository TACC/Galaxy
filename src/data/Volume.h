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

/*! \file Volume.h 
 * \brief a regular-grid volumetric dataset within Galaxy
 * \ingroup data
 */

#include <memory>
#include <string>
#include <vector>

#include <vtkClientSocket.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include "Box.h"
#include "dtypes.h"
#include "KeyedDataObject.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Volume)

//! a regular-grid volumetric dataset within Galaxy
/*! \ingroup data 
 * \sa KeyedObject, KeyedDataObject
 */
class Volume : public KeyedDataObject
{
	KEYED_OBJECT_SUBCLASS(Volume, KeyedDataObject)

public:
	virtual void initialize(); //!< initialize this Volume object
	virtual ~Volume(); //!< default destructor

	//! supported volumetric datatypes
	enum DataType
	{
		FLOAT, UCHAR
	};

	//! get the samples (i.e. data value) array for this Volume
	unsigned char *get_samples() { return samples; }
	//! set the samples (i.e. data value) array for this Volume
	void set_samples(void * s) 
	{ 
		if (samples != NULL) 
	  { std::cerr << "WARNING: overwriting (and leaking) Galaxy samples array!" << std::endl;} 
	  samples = (unsigned char*)s; 
	}

	//! get the deltas (grid step size) for this Volume
	void get_deltas(float &x, float &y, float &z) { x = deltas.x; y = deltas.y; z = deltas.z; }
	//! set the deltas (grid step size) for this Volume
	void set_deltas(float x, float y, float z) { deltas.x = x; deltas.y = y; deltas.z = z; }

	//! get the global origin for the data in this Volume
	void get_global_origin(float &x, float &y, float &z) 
	{
		x = global_origin.x;
		y = global_origin.y;
		z = global_origin.z;
	}
	//! set the global origin for the data in this Volume
	void set_global_origin(float x, float y, float z) 
	{
		global_origin.x = x;
		global_origin.y = y;
		global_origin.z = z;
	}
	//! get the local origin for the data at this process in this Volume
	/*! These values are computed from the global origin and local offsets */
	void get_local_origin(float &x, float &y, float &z)
	{
		x = global_origin.x + local_offset.x * deltas.x;
		y = global_origin.y + local_offset.y * deltas.y;
		z = global_origin.z + local_offset.z * deltas.z;
	}
        //! set the local bounding box 
        /*! The input are the extreme corners of the bounding box. The coordinates of the corner nearest the origin and those of the corner farthest from the origin. */
        void set_local_box(vec3f low, vec3f high) 
        {
            local_box = Box(low,high);
        }
        //! set the ghosted local offset values. 
        /* The values of the ghosted_local_offset are directly set with this method */
        void set_ghosted_local_offset(int x, int y, int z)
        {
            ghosted_local_offset.x = x;
            ghosted_local_offset.y = y;
            ghosted_local_offset.z = z;
        }
        //! set the local offset values.
        void set_local_offset(int x, int y, int z)
        {
            local_offset.x = x;
            local_offset.y = y;
            local_offset.z = z;
        }
	//! get the local origin, including ghost data, for the data at this process in this Volume
	/*! These values are computed from the global origin and ghosted local offsets */
	void get_ghosted_local_origin(float &x, float &y, float &z)
	{
		x = global_origin.x + ghosted_local_offset.x * deltas.x;
		y = global_origin.y + ghosted_local_offset.y * deltas.y;
		z = global_origin.z + ghosted_local_offset.z * deltas.z;
	}
	//! get the global number of sample points (i.e. data values) per axis
	void get_global_counts(int& nx, int& ny, int& nz) 
	{
		nx = global_counts.x;
		ny = global_counts.y;
		nz = global_counts.z;
	}
	//! set the global number of sample points (i.e. data values) per axis
	void set_global_counts(int nx, int ny, int nz) 
	{
		global_counts.x = nx;
		global_counts.y = ny;
		global_counts.z = nz;
	}
	//! get the local number of sample points (i.e. data values) per axis for data at this process
	void get_local_counts(int& nx, int& ny, int& nz) 
	{
		nx = local_counts.x;
	 	ny = local_counts.y;
	  nz = local_counts.z;
	}
	//! set the local number of sample points (i.e. data values) per axis for data at this process
	void set_local_counts(int nx, int ny, int nz) 
	{
		local_counts.x = nx;
	 	local_counts.y = ny;
    local_counts.z = nz;
	}
	//! get the local offset for ghost data at this process
	void get_ghosted_local_offsets(int& ni, int& nj, int& nk) 
	{
		ni = ghosted_local_offset.x;
		nj = ghosted_local_offset.y;
		nk = ghosted_local_offset.z;
	}
	//! get the local number of sample points (i.e. data values) per axis, including ghost data, at this process
	void get_ghosted_local_counts(int& nx, int& ny, int& nz) 
	{
		nx = ghosted_local_counts.x;
		ny = ghosted_local_counts.y;
		nz = ghosted_local_counts.z;
	}

	//! set the local number of sample points (i.e. data values) per axis, including ghost data, at this process
	void set_ghosted_local_counts(int nx, int ny, int nz) 
	{
		ghosted_local_counts.x = nx;
		ghosted_local_counts.y = ny;
		ghosted_local_counts.z = nz;
	}

	//! get the type of data used in this Volume
	DataType get_type() { return type; }
	//! set the type of data used in this Volume
	void set_type(DataType t) { type = t; }

  //! is this a float volume?
  bool isFloat() { return get_type() == FLOAT; }

  //! import the given data file into local memory
  /*! This action is performed in response to a ImportMsg */
	virtual bool local_import(char *fname, MPI_Comm c);

	//! get the global min and max data values for this Volume
	void get_global_minmax(float &min, float &max) { min = global_min; max = global_max; }

	//! get the local min and max data values for this Volume at this process
	void get_local_minmax(float &min, float &max) { min = local_min; max = local_max; }

  //! construct a Volume from a Galaxy JSON specification
  virtual bool LoadFromJSON(rapidjson::Value&);

protected:
	bool initialize_grid; 	// If time step data, need to grab grid info from first timestep

  vtkImageData *vtkobj;

	std::string filename;

	DataType type;

	vec3f deltas;

	vec3f global_origin;
	vec3i global_counts;
	vec3i local_offset;
	vec3i local_counts;
	vec3i ghosted_local_offset;
	vec3i ghosted_local_counts;

	unsigned char *samples;
};

} // namespace gxy
