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

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include "dtypes.h"
#include "Application.h"
#include "Box.h"
#include "Geometry.h"
#include "Datasets.h"
#include "KeyedDataObject.h"

#include <vtkSmartPointer.h>
#include <vtkClientSocket.h>
#include <vtkPolyData.h>

#include "rapidjson/document.h"

#include "ospray/ospray.h"

namespace gxy
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

	virtual void Import(std::string);

  virtual bool local_commit(MPI_Comm);
  virtual void local_import(char *, MPI_Comm);
  virtual bool local_load_timestep(MPI_Comm);

  Particle *get_samples() { return samples; }
  int get_n_samples() { return n_samples; }

	void allocate(int);

  void gather_global_info();

  bool has_neighbor(unsigned int face) { return neighbors[face] >= 0; }

	void LoadPartitioning(std::string partitioning);

	void GetPolyData(vtkPolyData*& v);
	void GetSamples(Particle*& s, int& n);

	void set_filename(std::string s)     { filename = s; }
	void set_layoutname(std::string s)   { layoutname = s; }
	void set_partfilename(std::string s) { partfilename = s; }

	virtual void LoadFromJSON(rapidjson::Value&);
	virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

	void SetRadius(float r) { radius = r; }
	void SetRadiusScale(float s) { radius_scale = s; }

protected:
  vtkClientSocket *skt;
  std::string filename;
  std::string layoutname;
  std::string partfilename;

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  float radius;
  float radius_scale;

	vtkPolyData *vtkobj;

  void get_partitioning(rapidjson::Value&);
  void get_partitioning_from_file(char *);

  Particle *samples;
  int n_samples;

  std::vector<Particle> ghosts;

	class LoadPartitioningMsg : public Work
	{
  public:
    LoadPartitioningMsg(Key k, std::string pname) 
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

} // namespace gxy
