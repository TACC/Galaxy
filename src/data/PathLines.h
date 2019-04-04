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

  //! broadcast an ImportMsg to all Galaxy processes to import the given data file
  virtual bool Import(std::string);

  /*! This action is performed in response to a ImportMsg */
  virtual bool local_import(char *, MPI_Comm);

  //! construct a PathLines from a Galaxy JSON specification
  virtual bool LoadFromJSON(rapidjson::Value&);
  //! save this PathLines to a Galaxy JSON specification 
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  //! Install vertices and data in 4-tuples, which are [x, y, z, data]
  void InstallVertexData(bool shared, int nv, float *v);

  //! Install segment indices.  These may be shared with an application; the 'shared' boolean parameter indicates this.
  void InstallSegments(bool shared, int ns, int *s);

  //! Get the number of vertices
  int GetNumberOfVertices() { return n_vertices; }

  //! Get the number of segments
  int GetNumberOfSegments() { return n_segments; }

  //! Get the vertices
  float *GetVertices() { return vertices; }

  //! Get the pathline segments
  int *GetSegments() { return segments; }

private:
  bool vertices_shared;
  bool segments_shared;

	int n_segments;
	int n_vertices;

	float *vertices;
	int   *segments;

  vtkUnstructuredGrid *vtkug;
};

} // namespace gxy
