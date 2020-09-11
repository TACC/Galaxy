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

/*! \file Triangles.h 
 * \brief a triangle (tessellated) dataset within Galaxy
 * \ingroup data
 */

#include <string>
#include <string.h>
#include <memory.h>

#include <vtkUnstructuredGrid.h>

#include "Application.h"
#include "Box.h"
#include "dtypes.h"
#include "Geometry.h"

#include "rapidjson/document.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Triangles)

//! a triangle (tessellated) dataset within Galaxy
/* \ingroup data 
 * \sa KeyedObject, KeyedDataObject
 */
class Triangles : public Geometry
{
  KEYED_OBJECT_SUBCLASS(Triangles, Geometry)

public:
	void initialize(); //!< initialize this Triangles object
	virtual ~Triangles(); //!< default destructor 

  //! Allocate space for vertices(data) and connectivity
  virtual void allocate_vertices(int nv);

  //! Copy local part
  virtual bool local_copy(KeyedDataObjectP);

  vec3f* GetNormals() { return (vec3f *)normals->data(); }

  virtual OsprayObjectP CreateTheOSPRayEquivalent(KeyedDataObjectP);

protected:
  virtual bool load_from_vtkPointSet(vtkPointSet *);
  std::shared_ptr< std::vector<vec3f> > normals;
};

} // namespace gxy
