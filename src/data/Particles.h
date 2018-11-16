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

/*! \file Particles.h 
 * \brief  a particle dataset within Galaxy
 * \ingroup data
 */

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include "dtypes.h"
#include "Application.h"
#include "Box.h"
#include "Geometry.h"
#include "KeyedDataObject.h"

#include <vtkSmartPointer.h>
#include <vtkClientSocket.h>
#include <vtkPolyData.h>

#include "rapidjson/document.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Particles)

//! a particle within Galaxy
/*! \ingroup data */
struct Particle
{
  Particle(float x, float y, float z, float v) { xyz.x = x, xyz.y = y, xyz.z = z, u.value = v; };
	Particle(float x, float y, float z, int   t) { xyz.x = x, xyz.y = y, xyz.z = z, u.type = t; };
  Particle(const  Particle& a) {xyz.x = a.xyz.x, xyz.y = a.xyz.y, xyz.z = a.xyz.z, u.value = a.u.value; }
  Particle() {xyz.x = 0, xyz.y = 0, xyz.z = 0, u.value = 0; }
  vec3f xyz;
	union {
		int type;
		float value;
	} u;
};

//!  a particle dataset within Galaxy
/* \ingroup data 
 * \sa Geometry, KeyedObject, KeyedDataObject
 */
class Particles : public Geometry
{
  KEYED_OBJECT_SUBCLASS(Particles, Geometry)

	friend class LoadPartitioningMsg;

public:
	void initialize(); //!< initialize this Particles object
	virtual ~Particles(); //!< default destructor

  //! register the message types used when processing Particle data
  static void RegisterMessages()
  {
		LoadPartitioningMsg::Register();
  }

  //! broadcast an ImportMsg to all Galaxy processes to import the given data file
	virtual void Import(std::string);

  //! import the given data file into local memory
  /*! This action is performed in response to a ImportMsg */
  virtual void local_import(char *, MPI_Comm);
  //! load a timestep into local memory
  /*! This action is performed in response to a LoadTimestepMsg */
  virtual bool local_load_timestep(MPI_Comm);

  //! return an array of Particle objects
  Particle *get_samples() { return samples.data(); }
  //! return the number of Particle objects held in this dataset
  int get_n_samples() { return samples.size(); }
  //! allocate a memory block to hold the given number of Particle elements
	void allocate(int);

  //! return true if the requested neighbor exists
  /*! This method uses the Box face orientation indices for neighbor indexing
   *          - yz-face neighbors - `0` for the lower (left) `x`, `1` for the higher (right) `x`
   *          - xz-face neighbors - `2` for the lower (left) `y`, `3` for the higher (right) `y`
   *          - xy-face neighbors - `4` for the lower (left) `z`, `5` for the higher (right) `z`
   */  
  bool has_neighbor(unsigned int face) { return neighbors[face] >= 0; }

  //! broadcast a LoadPartitioningMsg to all Galaxy processes to import the given data partition description file
	void LoadPartitioning(std::string partitioning);

  //! return the Particle elements of this object in the given vtkPolyData object
  /*! If the Particle elements are already stored as a vtk object, the pointer to this object is returned.
   * Otherwise, the Particle array is converted to a vtk object and the pointer to this new object is returned.
   * *Note:* if the array to vtk conversion occurs, both the vtk and array versions of the data are maintained.
   */
	void GetPolyData(vtkPolyData*& v);

  //! return the Particle elements of this object as a Particle array
  /*! If the Particle elements are already stored as an array, the pointer to this object is returned.
   * Otherwise, the vtk object is converted to an array and the pointer to this new array is returned.
   * *Note:* if the vtk to array conversion occurs, both the vtk and array versions of the data are maintained.
   */  
	void GetSamples(Particle*& s, int& n);

  //! set the file name for this Particles data set
	void set_filename(std::string s)     { filename = s; }

  //! construct a Particles from a Galaxy JSON specification
	virtual void LoadFromJSON(rapidjson::Value&);
  //! save this Particles to a Galaxy JSON specification 
	virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  //! set the sphere radius to use when rendering these Particles
	void SetRadius(float r) { radius = r; }
  void SetRadiusScale(float s) { radius_scale = s; }

  //! get the sphere radius to use when rendering these Particles
	float GetRadius() { return radius; }
  float GetRadiusScale() { return radius_scale; }

  //! add a Particle to this Particles dataset
	void push_back(Particle p) { samples.push_back(p); }

	//! clear the particles list
	void clear() { samples.clear(); }

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

  std::vector<Particle> samples;
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
