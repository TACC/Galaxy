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
#include "Box.h"

  /*! This method uses the Box face orientation indices for neighbor indexing
   *          - yz-face neighbors - `0` for the lower (left) `x`, `1` for the higher (right) `x`
   *          - xz-face neighbors - `2` for the lower (left) `y`, `3` for the higher (right) `y`
   *          - xy-face neighbors - `4` for the lower (left) `z`, `5` for the higher (right) `z`
   */

namespace gxy
{

class Partitioning
{
  inline int ijk2rank(int i, int j, int k)
  {
    return(i + (j * gpart.x) + (k * gpart.x * gpart.y));
  }

  static void factor(int ijk, vec3i &factors)
  {
    if (ijk == 1)
    {
      factors.x = factors.y = factors.z = 1;
      return;
    }

    int mm = ijk+3;
    for (int i = 1; i <= ijk>>1; i++)
    {
      int jk = ijk / i;
      if (ijk == (i * jk))
      {
        for (int j = 1; j <= jk>>1; j++)
        {
          int k = jk / j;
          if (jk == (j * k))
          {
            int m = i + j + k;
            if (m < mm)
            {
              mm = m;
              factors.x = i;
              factors.y = j;
              factors.z = k;
            }
          }
        }
      }
    }
  }

public:
  Partitioning(Box& global_box)
  {
    gbox = global_box;

    factor(GetTheApplication()->GetSize(), gpart);
  
    vec3f gsize = gbox.xyz_max - gbox.xyz_min;
    psize = vec3f(gsize.x / gpart.x, gsize.y / gpart.y, gsize.z / gpart.z);
  
    boxes = new Box[GetTheApplication()->GetSize()];
    owners = new int[GetTheApplication()->GetSize()];

    Box *b = boxes;
    int *o = owners;
    vec3i ijk;

    float oz = gbox.xyz_min.z;
    for (int k = 0; k < gpart.z; k++, oz += psize.z)
    {
      float oy = gbox.xyz_min.y;
      for (int j = 0; j < gpart.y; j++, oy += psize.y)
      {
        float ox = gbox.xyz_min.x;
        for (int i = 0; i < gpart.x; i++, ox += psize.x, b++, o++)
        {
          *o = ijk2rank(i,j,k);

          if (*o == GetTheApplication()->GetRank())
            ijk = vec3i(i,j,k);

          b->xyz_min.x = ox;  b->xyz_max.x = ox + psize.x;
          b->xyz_min.y = oy;  b->xyz_max.y = oy + psize.y;
          b->xyz_min.z = oz;  b->xyz_max.z = oz + psize.z;
        }
      }
    }

    neighbors[0] = (ijk.x > 0) ? ijk2rank(ijk.x - 1, ijk.y, ijk.z) : -1;
    neighbors[1] = (ijk.x < (gpart.x-1)) ? ijk2rank(ijk.x + 1, ijk.y, ijk.z) : -1;
    neighbors[2] = (ijk.y > 0) ? ijk2rank(ijk.x, ijk.y - 1, ijk.z) : -1;
    neighbors[3] = (ijk.y < (gpart.y-1)) ? ijk2rank(ijk.x, ijk.y + 1, ijk.z) : -1;
    neighbors[4] = (ijk.z > 0) ? ijk2rank(ijk.x, ijk.y, ijk.z - 1) : -1;
    neighbors[5] = (ijk.z < (gpart.z-1)) ? ijk2rank(ijk.x, ijk.y, ijk.z + 1) : -1;
  }

  bool isIn(vec3f p, float fuzz = 0) { return boxes[GetTheApplication()->GetRank()].isIn(p, fuzz); }
  bool isIn(int i, vec3f p, float fuzz = 0) { return boxes[i].isIn(p, fuzz); }

  int neighbor(vec3f p, vec3f v) { return neighbor(p.x, p.y, p.z, v.x, v.y, v.z); }
  int neighbor(float px, float py, float pz, float vx, float vy, float vz)
  {
    int face = get_local_box().exit_face(px, py, pz, vx, vy, vz);
    return neighbors[face];
  }

  void get_local_box(Box& b) { b = boxes[GetTheApplication()->GetRank()]; }
  Box get_local_box() { return boxes[GetTheApplication()->GetRank()]; }

  int PointOwner(vec3f& p) 
  {
    // Point in partition space

    p = prod(p - gbox.xyz_min, vec3f(1.0 / psize.x, 1.0 / psize.y, 1.0 / psize.z));
    if (! gbox.isIn(p))
      return -1;

    return ijk2rank(int(p.x), int(p.y), int(p.z));
  }

private:
  vec3i gpart;
  Box gbox;
  Box *boxes = NULL;
  int *owners = NULL;
  int neighbors[6];
  vec3f psize;
};

}

