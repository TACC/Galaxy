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

/*! \file ParticlesVis.h 
 * \brief a visualization element operating on a particle dataset within Galaxy
 * \ingroup data
 */

#include "dtypes.h"
#include "Application.h"
#include "Vis.h"
#include "Particles.h"
#include "Datasets.h"

#include "KeyedObject.h"

#include "rapidjson/document.h"

namespace gxy
{

OBJECT_POINTER_TYPES(ParticlesVis)

//!  a visualization element operating on a particle dataset within Galaxy
/* \ingroup data 
 * \sa Vis, KeyedObject, ISPCObject, OSPRayObject
 */
class ParticlesVis : public Vis
{
  KEYED_OBJECT_SUBCLASS(ParticlesVis, Vis) 

public:
	~ParticlesVis(); //!< destructor
  
  virtual void initialize(); //!< initialize this ParticlesVis object
  //! commit this object to the local registry
  /*! This action is performed in response to a CommitMsg */
  virtual bool local_commit(MPI_Comm);

protected:

	virtual void initialize_ispc();
	virtual void allocate_ispc();
	virtual void destroy_ispc();

  virtual bool LoadFromJSON(rapidjson::Value&);
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);
};

} // namespace gxy
