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

#include <memory>
#include <string>
#include <vector>

#include <vtkClientSocket.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include "Box.h"
#include "dtypes.h"
#include "KeyedObject.h"
#include "OSPRayObject.h"

namespace gxy
{

KEYED_OBJECT_POINTER(Volume)

class Volume : public OSPRayObject
{
	KEYED_OBJECT_SUBCLASS(Volume, OSPRayObject)

public:
	virtual void initialize();
	virtual ~Volume();

	enum DataType
	{
		FLOAT, UCHAR
	};

	void print_partition_boxes();

	unsigned char *get_samples() { return samples; }
	void set_samples(void * s) { if (samples != NULL) 
	{ std::cerr << "WARNING: overwriting (and leaking) PVOL samples array!" << std::endl;} samples = (unsigned char*)s; }

	void get_deltas(float &x, float &y, float &z) { x = deltas.x; y = deltas.y; z = deltas.z; }
	void set_deltas(float x, float y, float z) { deltas.x = x; deltas.y = y; deltas.z = z; }


	void get_global_origin(float &x, float &y, float &z) 
	{
		x = global_origin.x;
		y = global_origin.y;
		z = global_origin.z;
	}
	void set_global_origin(float x, float y, float z) 
	{
		global_origin.x = x;
		global_origin.y = y;
		global_origin.z = z;
	}

	void get_local_origin(float &x, float &y, float &z)
	{
		x = global_origin.x + local_offset.x * deltas.x;
		y = global_origin.y + local_offset.y * deltas.y;
		z = global_origin.z + local_offset.z * deltas.z;
	}

	void get_ghosted_local_origin(float &x, float &y, float &z)
	{
		x = global_origin.x + ghosted_local_offset.x * deltas.x;
		y = global_origin.y + ghosted_local_offset.y * deltas.y;
		z = global_origin.z + ghosted_local_offset.z * deltas.z;
	}

	void get_global_counts(int& nx, int& ny, int& nz) 
	{
		nx = global_counts.x;
		ny = global_counts.y;
		nz = global_counts.z;
	}
	void set_global_counts(int nx, int ny, int nz) 
	{
		global_counts.x = nx;
		global_counts.y = ny;
		global_counts.z = nz;
	}

	void get_local_counts(int& nx, int& ny, int& nz) 
	{
		nx = local_counts.x;
	 	ny = local_counts.y;
	  nz = local_counts.z;
	}
	void set_local_counts(int nx, int ny, int nz) 
	{
		local_counts.x = nx;
	 	local_counts.y = ny;
	  local_counts.z = nz;
	}

	void get_ghosted_local_counts(int& nx, int& ny, int& nz) 
	{
		nx = ghosted_local_counts.x;
		ny = ghosted_local_counts.y;
		nz = ghosted_local_counts.z;
	}
	void set_ghosted_local_counts(int nx, int ny, int nz) 
	{
		ghosted_local_counts.x = nx;
		ghosted_local_counts.y = ny;
		ghosted_local_counts.z = nz;
	}


	void gather_global_info();

	DataType get_type() { return type; }
	void set_type(DataType t) { type = t; }

	virtual bool local_commit(MPI_Comm);
	virtual void local_import(char *fname, MPI_Comm c);
	virtual bool local_load_timestep(MPI_Comm c);

	void get_global_minmax(float &min, float &max) { min = global_min; max = global_max; }
	void get_local_minmax(float &min, float &max) { min = local_min; max = local_max; }

  virtual void LoadFromJSON(rapidjson::Value&);
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

protected:
	bool initialize_grid; 	// If time step data, need to grab grid info from first timestep

  vtkImageData *vtkobj;

	string filename;

	DataType type;

	vec3f deltas;

	vec3f global_origin;
	vec3i global_counts;
	vec3i local_offset;
	vec3i local_counts;
	vec3i ghosted_local_offset;
	vec3i ghosted_local_counts;

	unsigned char *samples;

	float local_min, local_max;
	float global_min, global_max;

	void set_global_minmax(float min, float max)   { global_min = min; global_max = max; }
	void set_local_minmax(float min, float max)   { local_min = min; local_max = max; }
};

} // namespace gxy
