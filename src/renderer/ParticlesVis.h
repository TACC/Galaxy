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

/*! \file ParticlesVis.h 
 * \brief a visualization element operating on a particle dataset within Galaxy
 * \ingroup render
 */

#include "dtypes.h"
#include "Application.h"
#include "GeometryVis.h"
#include "Particles.h"
#include "Datasets.h"

#include "GalaxyObject.h"

#include "rapidjson/document.h"

namespace gxy
{

KEYED_OBJECT_POINTER_TYPES(ParticlesVis)

//!  a visualization element operating on a particle dataset within Galaxy
/* \ingroup render 
 * \sa Vis, KeyedObject, IspcObject
 */
class ParticlesVis : public GeometryVis
{
  KEYED_OBJECT_SUBCLASS(ParticlesVis, GeometryVis) 

public:
	~ParticlesVis(); //!< destructor
  
  virtual void initialize(); //!< initialize this ParticlesVis object
  //! commit this object to the local registry
  /*! This action is performed in response to a CommitMsg */
  virtual bool local_commit(MPI_Comm);

#if 0
  virtual void SetTheOsprayDataObject(OsprayObjectDPtr o);
#endif

  //! Set a constant radius
  void SetRadius(float r0) { SetRadiusTransform(0.0, r0, 0.0, 0.0); }

  //! Set the transformation from data values to radius - linear between (v0,r0) and (v1,r1)
  void SetRadiusTransform(float _v0, float _r0, float _v1, float _r1) { v0 = _v0; r0 = _r0; v1 = _v1; r1 = _r1;}

  //! Get the transformation from data values to radius - linear between (v0,r0) and (v1,r1)
  void GetRadiusTransform(float& _v0, float& _r0, float& _v1, float& _r1) { _v0 = v0; _r0 = r0; _v1 = v1; _r1 = r1;}

  //! scale mapping to a given range
  virtual void ScaleMaps(float xmin, float xmax);

protected:

	virtual void initialize_ispc();
	virtual void allocate_ispc();
	virtual void destroy_ispc();

  virtual bool LoadFromJSON(rapidjson::Value&);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  float v0, r0, v1, r1; // Map radius linearly between (v0,r0) and (v1,r1)
};

} // namespace gxy
