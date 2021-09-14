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

#include "Partitioning.h"
#include "Partitioning_ispc.h"
#include <unistd.h>

using namespace rapidjson;
using namespace std;

/*! Structured partitioning uses the Box face orientation indices for neighbor indexing
 *          - yz-face neighbors - `0` for the lower (left) `x`, `1` for the higher (right) `x`
 *          - xz-face neighbors - `2` for the lower (left) `y`, `3` for the higher (right) `y`
 *          - xy-face neighbors - `4` for the lower (left) `z`, `5` for the higher (right) `z`
 */

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(Partitioning)

void
Partitioning::allocate_ispc()
{ 
  ispc = ispc::Partitioning_allocate();
} 
  
void
Partitioning::initialize_ispc()
{ 
  ispc::box3f b;
  Box lbox;
  get_local_box(lbox);
  b.lower.x = lbox.xyz_min.x;
  b.lower.y = lbox.xyz_min.y;
  b.lower.z = lbox.xyz_min.z;
  b.upper.x = lbox.xyz_max.x;
  b.upper.y = lbox.xyz_max.y;
  b.upper.z = lbox.xyz_max.z;
  ispc::Partitioning_initialize(GetIspc(), b);
} 
  
void
Partitioning::destroy_ispc()
{
  if (ispc)
  {
    ispc::Partitioning_destroy(ispc);
    ispc = NULL;
  }
}

void
Partitioning::initialize()
{
  allocate_ispc();
}

void
Partitioning::Register()
{
  RegisterClass();
}

Partitioning::~Partitioning()
{
  if (rectilinear_partitions) delete[] rectilinear_partitions;
  if (ispc) destroy_ispc();
}

void
Partitioning::SetBox(Box& global_box)
{
  gbox = global_box;
  number_of_partitions = GetTheApplication()->GetSize();
  setup();
}

void 
Partitioning::SetBox(float x, float y, float z, float X, float Y, float Z)
{
  gbox.xyz_min = vec3f(x, y, z);
  gbox.xyz_max = vec3f(X, Y, Z);
}

bool
Partitioning::local_commit(MPI_Comm c)
{
  if (super::local_commit(c))
    return true;

  if (GetTheApplication()->GetRank() >= number_of_partitions)
    for (int i = 0; i < 6; i++)
      neighbors[i] = -1;
  else
    for (int i = 0; i < 6; i++)
      neighbors[i] = rectilinear_partitions[GetTheApplication()->GetRank()].neighbors[i];

  initialize_ispc();

  return false;
}

void
Partitioning::setup()
{
  factor(number_of_partitions, gpart);

  if (GetTheApplication()->GetRank() == 0)
    std::cerr << "partitioning: " << gpart.x << " " << gpart.y << " " << gpart.z << "\n";

  vec3f gsize = gbox.xyz_max - gbox.xyz_min;
  psize = vec3f(gsize.x / gpart.x, gsize.y / gpart.y, gsize.z / gpart.z);

  rectilinear_partitions = new rectilinear_partition[number_of_partitions];
  rectilinear_partition *p = rectilinear_partitions;

  float oz = gbox.xyz_min.z;
  for (int k = 0; k < gpart.z; k++, oz += psize.z)
  {
    float oy = gbox.xyz_min.y;
    for (int j = 0; j < gpart.y; j++, oy += psize.y)
    {
      float ox = gbox.xyz_min.x;
      for (int i = 0; i < gpart.x; i++, ox += psize.x, p++)
      {
        p->box.xyz_min.x = ox;  p->box.xyz_max.x = (i == (gpart.x-1)) ? gbox.xyz_max.x : ox + psize.x;
        p->box.xyz_min.y = oy;  p->box.xyz_max.y = (j == (gpart.y-1)) ? gbox.xyz_max.y : oy + psize.y;
        p->box.xyz_min.z = oz;  p->box.xyz_max.z = (k == (gpart.z-1)) ? gbox.xyz_max.z : oz + psize.z;

        p->neighbors[0] = (i > 0) ? ijk2rank(i - 1, j, k) : -1;
        p->neighbors[1] = (i < (gpart.x-1)) ? ijk2rank(i + 1, j, k) : -1;
        p->neighbors[2] = (j > 0) ? ijk2rank(i, j - 1, k) : -1;
        p->neighbors[3] = (j < (gpart.y-1)) ? ijk2rank(i, j + 1, k) : -1;
        p->neighbors[4] = (k > 0) ? ijk2rank(i, j, k - 1) : -1;
        p->neighbors[5] = (k < (gpart.z-1)) ? ijk2rank(i, j, k + 1) : -1;
      }
    }
  }
}

bool
Partitioning::isIn(vec3f p, float fuzz)
{
  return rectilinear_partitions[GetTheApplication()->GetRank()].box.isIn(p, fuzz);
}

bool
Partitioning::isIn(int i, vec3f p, float fuzz)
{
  return rectilinear_partitions[i].box.isIn(p, fuzz);
}

int
Partitioning::neighbor(float px, float py, float pz, float vx, float vy, float vz)
{
  int face = get_local_box().exit_face(px, py, pz, vx, vy, vz);
  return neighbors[face];
}

int
Partitioning::PointOwner(vec3f& p) 
{
  p = prod(p - gbox.xyz_min, vec3f(1.0 / psize.x, 1.0 / psize.y, 1.0 / psize.z));
  if (! gbox.isIn(p))
    return -1;

  return ijk2rank(int(p.x), int(p.y), int(p.z));
}

int
Partitioning::serialSize()
{
  return super::serialSize() + gbox.serialSize() + sizeof(int) + number_of_partitions*sizeof(rectilinear_partition);
}

unsigned char*
Partitioning::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  ptr = gbox.serialize(ptr);
  
  *(int *)ptr = number_of_partitions;
  ptr += sizeof(int);

  memcpy(ptr, rectilinear_partitions, number_of_partitions*sizeof(rectilinear_partition));
  ptr += number_of_partitions*sizeof(rectilinear_partition);

  return ptr;
}

unsigned char*
Partitioning::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  ptr = gbox.deserialize(ptr);

  number_of_partitions = *(int *)ptr;
  ptr += sizeof(int);

  rectilinear_partitions = new rectilinear_partition[number_of_partitions];
  memcpy(rectilinear_partitions, ptr, number_of_partitions*sizeof(rectilinear_partition));
  ptr += number_of_partitions*sizeof(rectilinear_partition);

  return ptr;
}

int
Partitioning::ijk2rank(int i, int j, int k)
{
  return(i + (j * gpart.x) + (k * gpart.x * gpart.y));
}

vec3i
Partitioning::rank2ijk(int r)
{
  int i = r % gpart.x;
  int j = int(r / gpart.x) % gpart.y;
  int k = int(r / (gpart.x * gpart.y));
  return vec3i(i,j,k);
}

void
Partitioning::factor(int ijk, vec3i &factors)
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

bool
Partitioning::LoadFromJSON(Value& v)
{
  if (! v.HasMember("Partitioning"))
    return false;

  Value& vp = v["Partitioning"];

  if (vp.HasMember("default rectilinear box"))
  {
    gbox.LoadFromJSON(vp["default rectilinear box"]["global box"]);
    if (vp["default rectilinear box"].HasMember("number of partitions"))
      number_of_partitions = vp["default rectilinear box"]["number of partitions"].GetInt();
    else
      number_of_partitions = GetTheApplication()->GetSize();
    setup();
  }
  else if (vp.HasMember("rectilinear partitions"))
  {
    Value& partitions_json = vp["rectilinear partitions"];
    rectilinear_partitions = new rectilinear_partition[partitions_json.Size()];

    for (int i = 0; i < partitions_json.Size(); i++)
    {
      rectilinear_partitions[i].box.LoadFromJSON(partitions_json[i]["box"]);
      for (int j = 0; j < 6; j++)
        rectilinear_partitions[i].neighbors[j] = partitions_json[i]["nbrs"][j].GetInt();

      if (i == 0)
        gbox = rectilinear_partitions[0].box;
      else
        gbox.expand(rectilinear_partitions[i].box);
    }
  }
  else if (vp.HasMember("filename"))
  {
    rapidjson::Document *pdoc = GetTheApplication()->OpenJSONFile(vp["filename"].GetString());
    LoadFromJSON(*pdoc);
  }
  else
  {
    std::cerr << "unrecognized Partitioning json\n";
    return false;
  }

  return true;
}

}
