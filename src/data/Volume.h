#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <memory>

using namespace std;

#include "OSPRayObject.h"
#include "dtypes.h"
#include "Box.h"

#include <vtkSmartPointer.h>
#include <vtkClientSocket.h>
#include <vtkImageData.h>

namespace pvol
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

	void get_deltas(float &x, float &y, float &z) { x = deltas.x; y = deltas.y; z = deltas.z; }

	void get_global_origin(float &x, float &y, float &z) 
	{
		x = global_origin.x;
		y = global_origin.y;
		z = global_origin.z;
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

	void get_local_counts(int& nx, int& ny, int& nz) 
	{
		nx = local_counts.x;
	 ny = local_counts.y;
	 nz = local_counts.z;
	}

	void get_ghosted_local_counts(int& nx, int& ny, int& nz) 
	{
		nx = ghosted_local_counts.x;
		ny = ghosted_local_counts.y;
		nz = ghosted_local_counts.z;
	}


	void gather_global_info();

	DataType get_type() { return type; }

	virtual bool local_commit(MPI_Comm);
	virtual void local_import(char *fname, MPI_Comm c);
	virtual bool local_load_timestep(MPI_Comm c);

	void get_global_minmax(float &min, float &max) { min = global_min; max = global_max; }
	void get_local_minmax(float &min, float &max) { min = local_min; max = local_max; }

  virtual void LoadFromJSON(Value&);
  virtual void SaveToJSON(Value&, Document&);

protected:
	bool initialize_grid; 	// If time step data, need to grab grid info from first timestep

	// vtkClientSocket *skt;
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

}
