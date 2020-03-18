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

/*! \file PathLines.h 
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

OBJECT_POINTER_TYPES(PathLines)

//! a pathline vertex within Galaxy
/*! \ingroup data */

struct PLVertex
{
  PLVertex(float x, float y, float z, float v) { xyz.x = x, xyz.y = y, xyz.z = z, value = v; };
  PLVertex(const  PLVertex& a) {xyz.x = a.xyz.x, xyz.y = a.xyz.y, xyz.z = a.xyz.z, value = a.value; }
  PLVertex() {xyz.x = 0, xyz.y = 0, xyz.z = 0, value = 0; }
  vec3f xyz;
  float value;
};


//! a pathline dataset within Galaxy consists of three parts: a set of vertices, a set of indices and a set or pointers into that set of indices indicating where the individual pathlines begin.  Data is per-vertex.
/* \ingroup data 
 * \sa KeyedObject, KeyedDataObject
 */
class PathLines : public Geometry
{
  KEYED_OBJECT_SUBCLASS(PathLines, Geometry)

public:
	void initialize(); //!< initialize this PathLines object
	virtual ~PathLines(); //!< default destructor 

  void GetPLVertices(PLVertex*& p, int& n);

  virtual OsprayObjectP CreateTheOSPRayEquivalent(KeyedDataObjectP);

protected:
  virtual bool load_from_vtkPointSet(vtkPointSet *);

private:
};

} // namespace gxy
