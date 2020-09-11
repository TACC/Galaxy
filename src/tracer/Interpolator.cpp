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

#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <dtypes.h>



include "Interpolator.h"

using namespace gxy;
using namespace std;

namespace gxy
{

class InterpolateVolumeOntoGeometryMsg : public Work
{
public:
  InterpolateVolumeOntoGeometryMsg(GeometryP s, VolumeP v, GeometryP d) : InterpolateVolumeOntoGeometryMsg(3 * sizeof(Key))
  {
    Key *keys = (Key *)contents->get();
    keys[0] = s->getkey();
    keys[1] = v->getkey();
    keys[2] = d->getkey();
  }

  WORK_CLASS(InterpolateVolumeOntoGeometryMsg, true)

  bool CollectiveAction(MPI_Comm c, bool isRoot)
  {
    Key *keys = (Key *)get();

    GeometryP src = Geometry::GetByKey(keys[0]);
    VolumeP   vol = Volume::GetByKey(keys[1]);
    GeometryP dst = Geometry::GetByKey(keys[2]);

    if (src != dst)
    {
      dst->allocate(src->GetNumberOfVertices(), src->GetConnectivitySize());
      memcpy((void *)dst->GetVertices(), (void *)src->GetVertices(), dst->GetNumberOfVertices()*sizeof(vec3f));
      if (src->GetConnectivitySize() > 0)
        memcpy((void *)dst->GetConnectivity(), (void *)src->GetConnectivity(), dst->GetConnectivitySize()*sizeof(int));
    }

    float ox, oy, oz;
    vol->get_ghosted_local_origin(ox, oy, oz);

    float dx, dy, dz;
    vol->get_deltas(dx, dy, dz);

    int ik, jk, kk;
    vol->get_ghosted_local_counts(ik, jk, kk);

    int istride = 1;
    int jstride = ik;
    int kstride = ik * jk;

    vec3f *p = dst->GetVertices();
    float *d = dst->GetData();
    float m, M;
    for (int i = 0; i < dst->GetNumberOfVertices(); i++, p++, d++)
    {
      float x = (p->x - ox) / dx;
      float y = (p->y - oy) / dy;
      float z = (p->z - oz) / dz;
  
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
      
      if (vol->get_type() == Volume::FLOAT)
      {
        float *s = (float *)vol->get_samples();
        *d = s[v000]*b000 + s[v001]*b001 + s[v010]*b010 + s[v011]*b011 +
             s[v100]*b100 + s[v101]*b101 + s[v110]*b110 + s[v111]*b111;
      }        
      else
      {
        unsigned char *s = (unsigned char *)vol->get_samples();
        *d = s[v000]*b000 + s[v001]*b001 + s[v010]*b010 + s[v011]*b011 +
             s[v100]*b100 + s[v101]*b101 + s[v110]*b110 + s[v111]*b111;
      }        

      if (i == 0) m = M = *d;
      else
      {
        if (m > *d) m = *d;
        if (M < *d) M = *d;
      }
    }

    std::cerr << "min " << m << " max " << M << "\n";

    return false;
  }
};

WORK_CLASS_TYPE(InterpolateVolumeOntoGeometryMsg)

void
InitializeInterpolateVolumeOntoGeometry()
{
  InterpolateVolumeOntoGeometryMsg::Register();
}

void
InterpolateVolumeOntoGeometry(GeometryP s, VolumeP v)
{
  InterpolateVolumeOntoGeometryMsg msg(s, v, s);
  msg.Broadcast(true, true);
}

void
InterpolateVolumeOntoGeometry(GeometryP s, VolumeP v, GeometryP d)
{
  InterpolateVolumeOntoGeometryMsg msg(s, v, d);
  msg.Broadcast(true, true);
}

}

