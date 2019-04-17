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

/*! \file Triangles.h 
 * \brief a triangle (tessellated) dataset within Galaxy
 * \ingroup data
 */

#include <string>
#include <string.h>
#include <memory.h>

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

  /*! This action is performed in response to a ImportMsg */
  virtual bool local_import(char *, MPI_Comm);

  //! construct a Triangles from a Galaxy JSON specification
  virtual bool LoadFromJSON(rapidjson::Value&);

  //! Get the number of vertices
  int GetNumberOfVertices() { return n_vertices; }

  //! Get the number of triangles
  int GetNumberOfTriangles() { return n_triangles; }

  //! Get the vertices
  float *GetVertices() { return vertices; }

  //! Get the normals
  float *GetNormals() { return normals; }

  //! Get the triangles
  int *GetTriangles() { return triangles; }

private:
	int n_triangles;
	int n_vertices;
	float *vertices;
	float *normals;
	int *triangles;
};

} // namespace gxy
