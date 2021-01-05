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

public:
	void initialize(); //!< initialize this Particles object
	virtual ~Particles(); //!< default destructor

  //! add a Particle to this Particles dataset
	void push_back(Particle& p)
  {
    vertices->push_back(p.xyz);
    data->push_back(p.u.value);
  }
  void set_local_box(vec3f lo, vec3f hi) {local_box = Box(lo,hi);}
  void set_global_box(vec3f lo, vec3f hi) {global_box = Box(lo,hi);}
  void set_neighbors(int *nei) { 
    for (int i=0; i<6; i++) {
      neighbors[i] = nei[i];
    }
  }

  //! Allocates and stuffs a buffer full of particle structures.   Hope to remove the necessity for this by further extending OSPRay's spheres to take separate data array
  
  void GetParticles(Particle*& p, int& n);

  virtual GalaxyObjectDPtr CreateTheDeviceEquivalent(KeyedDataObjectDPtr);

protected:
  virtual bool load_from_vtkPointSet(vtkPointSet *);
};

} // namespace gxy
