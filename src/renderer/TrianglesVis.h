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

/*! \file TrianglesVis.h 
 * \brief a visualization element operating on a triangle (tessellated) dataset within Galaxy
 * \ingroup data
 */

#include "Application.h"
#include "Datasets.h"
#include "dtypes.h"
#include "KeyedObject.h"
#include "Triangles.h"
#include "Vis.h"

namespace gxy
{

OBJECT_POINTER_TYPES(TrianglesVis)

//! a visualization element operating on a triangle (tessellated) dataset within Galaxy
/* \ingroup data 
 * \sa Vis, KeyedObject, ISPCObject, OSPRayObject
 */
class TrianglesVis : public Vis
{
  KEYED_OBJECT_SUBCLASS(TrianglesVis, Vis) 

public:
	~TrianglesVis(); //!< default destructor
  
  //! initialize this TrianglesVis object
  virtual void initialize();
  //! commit this object to the local registry
  virtual bool local_commit(MPI_Comm);

protected:

	virtual void initialize_ispc();
	virtual void allocate_ispc();
	virtual void destroy_ispc();

  virtual void LoadFromJSON(rapidjson::Value&);
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);
};

} // namespace gxy
