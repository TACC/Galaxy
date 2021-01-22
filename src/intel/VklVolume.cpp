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

#include "VklVolume.h"

namespace gxy
{

OBJECT_CLASS_TYPE(VklVolume)

struct VklVolume_ispc
{
  VKLVolume volume;
};

int 
VklVolume::IspcSize() 
{
  return sizeof(VklVolume_ispc);
}

void
VklVolume::FinalizeData(KeyedDataObjectPtr kop)
{
  super::FinalizeData(kop);

  VolumePtr v = Volume::Cast(kop);

  vec3i counts;
  v->get_ghosted_local_counts(counts.x, counts.y, counts.z);

  vec3f origin, spacing;
  v->get_ghosted_local_origin(origin.x, origin.y, origin.z);
  v->get_deltas(spacing.x, spacing.y, spacing.z);

  size_t sz = counts.x*counts.y*counts.z;
  VKLData data = vklNewData(sz, v->isFloat() ? VKL_FLOAT : VKL_UCHAR, (void *)v->get_samples().get(), VKL_DATA_SHARED_BUFFER, 0);

  VKLData attr[] = {data};
  VKLData aData = vklNewData(1, VKL_DATA, attr, VKL_DATA_DEFAULT, 0);
  vklRelease(data);

  VklVolume_ispc *vispc = (VklVolume_ispc *)GetIspc();
  
  vispc->volume = vklNewVolume("structuredRegular");
  vklSetVec3i(vispc->volume, "dimensions", counts.x, counts.y, counts.z);
  vklSetVec3f(vispc->volume, "gridOrigin", origin.x, origin.y, origin.z);
  vklSetVec3f(vispc->volume, "gridSpacing", spacing.x, spacing.y, spacing.z);
  vklSetData(vispc->volume, "data", aData);
  vklRelease(aData);
}

VklVolume::~VklVolume()
{
  VklVolume_ispc *vispc = (VklVolume_ispc *)GetIspc();
  vklRelease(vispc->volume);
  vispc->volume = NULL;
}

}
