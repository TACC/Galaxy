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

/*! \file PointSurfaceVis.h 
 * \brief a visualization element operating on a particle dataset within Galaxy
 * \ingroup render
 */

#include "dtypes.h"
#include "Application.h"
#include "GeometryVis.h"
#include "PointSurface.h"
#include "Datasets.h"

#include "KeyedObject.h"

#include "rapidjson/document.h"

namespace gxy
{

OBJECT_POINTER_TYPES(PointSurfaceVis)

//!  a visualization element operating on a particle dataset within Galaxy
/* \ingroup render 
 * \sa Vis, KeyedObject, IspcObject, OsprayObject
 */
class PointSurfaceVis : public GeometryVis
{
  KEYED_OBJECT_SUBCLASS(PointSurfaceVis, GeometryVis) 

public:
	~PointSurfaceVis(); //!< destructor
  
  virtual void initialize(); //!< initialize this PointSurfaceVis object
  //! commit this object to the local registry
  /*! This action is performed in response to a CommitMsg */
  virtual bool local_commit(MPI_Comm);

  virtual void SetTheOsprayDataObject(OsprayObjectP o);

  //! Set a R range
  void SetRRange(float _r0, float _r1, float _dr) { r0 = _r0; r1 = _r1; dr = _dr; }

  //! Get the R Range
  void GetRRange(float& _r0, float& _r1, float& _dr) { _r0 = r0; _r1 = r1; _dr = dr; }

  virtual OsprayObjectP CreateTheOsprayDataObject(KeyedDataObjectP d);

protected:

	virtual void initialize_ispc();
	virtual void allocate_ispc();
	virtual void destroy_ispc();

  virtual bool LoadFromJSON(rapidjson::Value&);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  float r0, r1, dr;
};

} // namespace gxy
