#pragma once

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include "dtypes.h"
#include "Application.h"
#include "Box.h"
#include "Geometry.h"
#include "Datasets.h"

#include <vtkSmartPointer.h>
#include <vtkClientSocket.h>
#include <vtkPolyData.h>

#include "rapidjson/document.h"

#include "ospray/ospray.h"
#include "KeyedDataObject.h"

namespace pvol
{

KEYED_OBJECT_POINTER(Particles)

class Box;

struct Particle
{
  vec3f xyz;
	union {
		int type;
		float value;
	} u;
};

class Particles : public Geometry
{
  KEYED_OBJECT_SUBCLASS(Particles, Geometry)

	friend class LoadPartitioningMsg;

public:
	void initialize();
	virtual ~Particles();

  static void RegisterMessages()
  {
		LoadPartitioningMsg::Register();
  }

	virtual void Import(string);

  virtual bool local_commit(MPI_Comm);
  virtual void local_import(char *, MPI_Comm);
  virtual bool local_load_timestep(MPI_Comm);

  Particle *get_samples() { return samples; }
  int get_n_samples() { return n_samples; }

	void allocate(int);

  void gather_global_info();

  bool has_neighbor(unsigned int face) { return neighbors[face] >= 0; }

	void LoadPartitioning(string partitioning);

	void GetPolyData(vtkPolyData*& v);
	void GetSamples(Particle*& s, int& n);

	void set_filename(string s)     { filename = s; }
	void set_layoutname(string s)   { layoutname = s; }
	void set_partfilename(string s) { partfilename = s; }

	virtual void LoadFromJSON(Value&);
	virtual void SaveToJSON(Value&, Document&);

protected:
  vtkClientSocket *skt;
  string filename;
  string layoutname;
  string partfilename;

  float radius;
  float radius_scale;

	vtkPolyData *vtkobj;

  void get_partitioning(Value&);
  void get_partitioning_from_file(char *);

  Particle *samples;
  int n_samples;

  vector<Particle> ghosts;

	class LoadPartitioningMsg : public Work
	{
  public:
    LoadPartitioningMsg(Key k, string pname) 
				: LoadPartitioningMsg(sizeof(Key) + pname.length() + 1)
    {
      *(Key *)contents->get() = k;
      memcpy(((char *)contents->get()) + sizeof(Key), pname.c_str(), pname.length()+1);
    }

    WORK_CLASS(LoadPartitioningMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
      char *p = (char *)contents->get();
      Key k = *(Key *)p;
      p += sizeof(Key);
			Particles::GetByKey(k)->get_partitioning_from_file(p);
			return false;
    }
  };
};
}
