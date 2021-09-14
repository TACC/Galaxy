// ========================================================================== //
//                                                                            //
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

#include "Interpolator.hpp"

namespace gxy
{

WORK_CLASS_TYPE(InterpolatorMsg)

int Interpolator::interpolator_index = 0;

void 
Interpolator::Interpolate()
{
  InterpolatorMsg msg(volume, Geometry::Cast(result));
  msg.Broadcast(true, true);
  result->Commit();
}

bool
InterpolatorMsg::CollectiveAction(MPI_Comm c, bool is_root)
{
  Key *keys = (Key *)get();
  VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(keys[0]));
  GeometryP p = Geometry::Cast(KeyedDataObject::GetByKey(keys[1]));

  float ox, oy, oz;
  v->get_local_origin(ox, oy, oz);

  float dx, dy, dz;
  v->get_deltas(dx, dy, dz);

  int ik, jk, kk;
  v->get_local_counts(ik, jk, kk);

  int istride = 1;
  int jstride = ik;
  int kstride = ik * jk;

  float *d = p->GetData();

  for (int i = 0; i < p->GetNumberOfVertices(); i++)
  {
    vec3f point = p->GetVertices()[i];

    float x = (point.x - ox) / dx;
    float y = (point.y - oy) / dy;
    float z = (point.z - oz) / dz;

    int ix = (int)x;
    int iy = (int)y;
    int iz = (int)z;

    float dx = x - ix;
    float dy = y - iy;
    float dz = z - iz;

    int v000 = (ix + 0) * istride + (iy + 0) * jstride + (iz + 0) * kstride;
    int v001 = (ix + 0) * istride + (iy + 0) * jstride + (iz + 1) * kstride;
    int v010 = (ix + 0) * istride + (iy + 1) * jstride + (iz + 0) * kstride;
    int v011 = (ix + 0) * istride + (iy + 1) * jstride + (iz + 1) * kstride;
    int v100 = (ix + 1) * istride + (iy + 0) * jstride + (iz + 0) * kstride;
    int v101 = (ix + 1) * istride + (iy + 0) * jstride + (iz + 1) * kstride;
    int v110 = (ix + 1) * istride + (iy + 1) * jstride + (iz + 0) * kstride;
    int v111 = (ix + 1) * istride + (iy + 1) * jstride + (iz + 1) * kstride;

    float b000 = (1.0 - dx) * (1.0 - dy) * (1.0 - dz);
    float b001 = (1.0 - dx) * (1.0 - dy) * (dz);
    float b010 = (1.0 - dx) * (dy) * (1.0 - dz);
    float b011 = (1.0 - dx) * (dy) * (dz);
    float b100 = (dx) * (1.0 - dy) * (1.0 - dz);
    float b101 = (dx) * (1.0 - dy) * (dz);
    float b110 = (dx) * (dy) * (1.0 - dz);
    float b111 = (dx) * (dy) * (dz);

    float interpolated_value;
    if (v->get_type() == Volume::FLOAT)
    {
      float *p = (float *)v->get_samples();
      interpolated_value = p[v000]*b000 + p[v001]*b001 + p[v010]*b010 + p[v011]*b011 +
                           p[v100]*b100 + p[v101]*b101 + p[v110]*b110 + p[v111]*b111;
    }
    else
    {
      unsigned char *p = (unsigned char *)v->get_samples();
      interpolated_value = p[v000]*b000 + p[v001]*b001 + p[v010]*b010 + p[v011]*b011 +
                           p[v100]*b100 + p[v101]*b101 + p[v110]*b110 + p[v111]*b111;
    }

    *d++ = interpolated_value;
  }

  return false;
}


}

