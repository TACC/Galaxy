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

/*! \file Geometry.h 
 * \brief representation for 3D renderable geometry in Galaxy
 * \ingroup data
 */

#include <string>
#include <string.h>
#include <pthread.h>
#include <memory.h>
#include <vector>

#include <vtkPointSet.h>

#include "KeyedDataObject.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Geometry)

//! representation for 3D renderable geometry in Galaxy
/*! \ingroup data 
 * \sa KeyedDataObject
 */
class Geometry : public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(Geometry, KeyedDataObject)

  //! Load JSON partition file and send it to servers so they can import their bits
  virtual bool Import(std::string);

  //! Allocate space for vertices(data) and connectivity
  virtual void allocate_vertices(int nv);
  virtual void allocate_connectivity(int nc);
  void allocate(int nv, int nc) { allocate_vertices(nv); allocate_connectivity(nc); }

  /*! This action is performed in response to a ImportMsg */
  virtual bool local_import(char *, MPI_Comm);

  //! learn global and local minmax of data
  virtual bool local_commit(MPI_Comm);


  //! load geometry from a Galaxy JSON specification
  virtual bool LoadFromJSON(rapidjson::Value&);

  //! set the default color to use when rendering these Particles
  void SetDefaultColor(vec4f dc) { default_color = dc; }
  void SetDefaultColor(float r, float g, float b, float a)
  {
    default_color.x = r;
    default_color.y = g;
    default_color.z = b;
    default_color.w = a;
  }

  //! get the default color to use when rendering these Particles
  void GetDefaultColor(vec4f& dc) { dc = default_color; }
  void GetDefaultColor(float& r, float& g, float& b, float& a)
  {
    r = default_color.x;
    g = default_color.y;
    b = default_color.z;
    a = default_color.w;
  }

  int GetNumberOfVertices() { return vertices->size(); }
  int GetConnectivitySize() { return connectivity->size(); }

  vec3f* GetVertices() { return (vec3f *)vertices->data(); }
  float* GetData() { return (float *)data->data(); }
  int*   GetConnectivity() { return (int *)connectivity->data(); }

  void clear()
  {
    vertices->clear();
    data->clear();
    connectivity->clear();
  }

  void Lock() { pthread_mutex_lock(&lock); }
  void Unlock() { pthread_mutex_unlock(&lock); }

  void GetCounts(int& nv, int& ne) { nv = global_vertex_count; ne = global_element_count; }
  int GetVertexCount() { return global_vertex_count; }
  int GetElementCount() { return global_element_count; }

protected:
  pthread_mutex_t lock;

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  vec4f default_color;

  virtual bool load_from_vtkPointSet(vtkPointSet *) { return false; }

  void initialize(); //!< initialize this Geometry objec

  virtual KeyedDataObjectP Copy();

  //! Get partitioning info from JSON object
  bool get_partitioning(rapidjson::Value&);

  //! Load partitioning info from JSON file
  bool get_partitioning_from_file(char *);

  //! vertex, data and connectivity storage
  std::shared_ptr<std::vector<vec3f>> vertices;
  std::shared_ptr<std::vector<float>> data;
  std::shared_ptr<std::vector<int>>   connectivity;

  int global_vertex_count;
  int global_element_count;


  vtkDataSet *vtkobj; // If there is a retained VTK dataset
};

} // namespace gxy
