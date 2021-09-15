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

#include "Application.h"
#include "KeyedObject.h"
#include "Box.h"
#include "IspcObject.h"

// #include "Partitioning_ispc.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

  /*! This method uses the Box face orientation indices for neighbor indexing
   *          - yz-face neighbors - `0` for the lower (left) `x`, `1` for the higher (right) `x`
   *          - xz-face neighbors - `2` for the lower (left) `y`, `3` for the higher (right) `y`
   *          - xy-face neighbors - `4` for the lower (left) `z`, `5` for the higher (right) `z`
   */

namespace gxy
{

OBJECT_POINTER_TYPES(Partitioning)

class Partitioning : public gxy::KeyedObject, public IspcObject
{
  KEYED_OBJECT(Partitioning)

  struct rectilinear_partition
  {
    Box box;
    int neighbors[6];
    bool isIn(vec3f p, float fuzz = 0)
    {
      if (p.x < (box.xyz_min.x - fuzz)) return false;
      if (p.x > (box.xyz_min.x + fuzz)) return false;
      if (p.y < (box.xyz_min.y - fuzz)) return false;
      if (p.y > (box.xyz_min.y + fuzz)) return false;
      if (p.z < (box.xyz_min.z - fuzz)) return false;
      if (p.z > (box.xyz_min.z + fuzz)) return false;
      return true;
    }
  };

public:
  virtual ~Partitioning();  //!< default destructor
  virtual void initialize(); //!< initialize this object

  void SetBox(Box&);
  void SetBox(float, float, float, float, float, float); // xmin ymin zmin xmax ymax zmax

  bool isIn(vec3f, float fuzz = 0);          // Is the point inside this rank's partition (with fuzz)
  bool isIn(int i, vec3f p, float fuzz = 0); // Is the point inside rank i's partition (with fuzz)

  // What rank will a ray with origin p and direction hit next?

  int neighbor(vec3f p, vec3f v) { return neighbor(p.x, p.y, p.z, v.x, v.y, v.z); }
  int neighbor(float, float, float, float, float, float);

  Box get_box(int i) { return rectilinear_partitions[i].box; }

  void get_local_box(Box& b) { b = rectilinear_partitions[GetTheApplication()->GetRank()].box; }
  Box get_local_box() { return rectilinear_partitions[GetTheApplication()->GetRank()].box; }

  void get_global_box(Box& b) { b = gbox; }
  Box get_global_box() { return gbox; }

  // Which rank's partion contains a point?

  int PointOwner(vec3f&);

  virtual bool local_commit(MPI_Comm);

  bool LoadFromJSON(rapidjson::Value&);

  bool isInitialized() { return number_of_partitions != -1; }
  int get_number_of_partitions() { return number_of_partitions; }


protected:
  void setup();

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  virtual void allocate_ispc();
  virtual void initialize_ispc();
  virtual void destroy_ispc();

private:
  int ijk2rank(int, int, int);        // convert an (i,j,k) in structured partitioning to a rank
  vec3i rank2ijk(int);                // convert a rank to an (i,j,k) in structured partitioning 
  static void factor(int, vec3i &);   // given a number of ranks, come up with a spatial partitioning

  vec3i gpart;
  Box gbox = Box(-1, -1, -1, 1, 1, 1);
  int neighbors[6];
  vec3f psize;

  rectilinear_partition *rectilinear_partitions = NULL;
  int number_of_partitions = -1;
};

}

