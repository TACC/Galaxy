// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
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
	  samples = (unsigned char*) s; 
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

  //! set the local offset values.
  void set_local_offset(int x, int y, int z)
  {
      local_offset.x = x;
      local_offset.y = y;
      local_offset.z = z;
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

	//! get the type of data used in this Volume
	DataType get_type() { return type; }
	//! set the type of data used in this Volume
	void set_type(DataType t) { type = t; }

  //! is this a float volume?
  bool isFloat() { return get_type() == FLOAT; }

  //! import the given data file into local memory
  /*! This action is performed in response to a ImportMsg */
  /*! returns true if fails; terminates messaging loop */
	virtual bool local_import(PartitioningP, char *, MPI_Comm);

  //! complete the global properties of the distributed volume object
  /*! This action is performed in response to a CommitMsg */
  virtual bool local_commit(MPI_Comm c);

	//! get the global min and max data values for this Volume
	void get_global_minmax(float &min, float &max) { min = global_min; max = global_max; }

	//! get the local min and max data values for this Volume at this process
	void get_local_minmax(float &min, float &max) { min = local_min; max = local_max; }

	//! set the local min and max data values for this Volume at this process
	void set_local_minmax(float min, float max) { local_min = min; local_max = max; }

	//! set the global  min and max data values for this Volume at this process
	void set_global_minmax(float min, float max) { global_min = min; global_max = max; }

  //! construct a Volume from a Galaxy JSON specification
  virtual bool LoadFromJSON(rapidjson::Value&, PartitioningP);

  //! Get the number of components
  int get_number_of_components() { return number_of_components; }

  //! Set the number of components
  void set_number_of_components(int n) { number_of_components = n; }

  //! Interpolate an arbitrary point and return true if its in the local partition, otherwise false
  bool Sample(vec3f& p, float* i);
  bool Sample(vec3f& p, vec3f& v);
  bool Sample(vec3f& p, float& v);

  //! Which process owns an arbitrary point in this global volume? -1 for outside
  int PointOwner(vec3f& p);

	void set_global_partitions(int i, int j, int k) { global_partitions.x = i; global_partitions.y = j; global_partitions.z = k; }

  virtual OsprayObjectP CreateTheOSPRayEquivalent(KeyedDataObjectP);

  void set_ijk(int i, int j, int k) { ijk.x = i; ijk.y = j; ijk.z = k; }

  void Allocate()
  {
    size_t sz = global_counts.x * global_counts.y * global_counts.z * number_of_components * ((type == FLOAT) ? sizeof(float) : sizeof(unsigned char));
    set_samples(malloc(sz));
  }

protected:
	bool initialize_grid; 	// If time step data, need to grab grid info from first timestep

  vtkImageData *vtkobj;

	std::string filename;

	DataType type;

	vec3f deltas;

  int number_of_components;

  vec3i ijk;
	vec3i global_partitions;
	vec3f global_origin;
	vec3i global_counts;
	vec3i local_offset;
	vec3i local_counts;

  unsigned char *samples;
};

} // namespace gxy
