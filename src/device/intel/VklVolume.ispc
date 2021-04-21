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

#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore.isph"

#include "VklVolume.ih"
#include "GxyRayHit.isph"
#include "Rays.ih"


// force export of VklVolume_ispc in VklVolume_ispc.h

export void VklVolume_dummy(uniform VklVolume_ispc * uniform vi) {}

void
VklVolume_IsoCrossingS(uniform VklVolume_ispc *uniform vkl, GxyRayHit& rayhit, VKLValueSelector selector)
{
  uniform VKLVolume volume   = vkl->volume;
  uniform VKLSampler sampler = vkl->sampler;

  uniform unsigned int8 bufferHit[VKL_MAX_HIT_ITERATOR_SIZE];
  
  vkl_vec3f o = {rayhit.ray.org_x, rayhit.ray.org_y, rayhit.ray.org_z};
  vkl_vec3f d = {rayhit.ray.dir_x, rayhit.ray.dir_y, rayhit.ray.dir_z};
  vkl_range1f t = {rayhit.ray.tnear, rayhit.ray.tfar};

  VKLHitIterator hitIterator = vklInitHitIteratorV(sampler, &o, &d, &t, selector, bufferHit);
  VKLHit hit;
  hit.t = 0;
  vklIterateHitV(hitIterator, &hit);

  if (hit.t > rayhit.ray.tnear && hit.t < rayhit.ray.tfar)
    rayhit.ray.tfar = hit.t;
}

void
VklVolume_IsoCrossing(uniform VklVolume_ispc *uniform vkl, GxyRayHit& rayhit, uniform int nv, float *uniform v)
{
  uniform VKLVolume volume   = vkl->volume;

  VKLValueSelector selector = vklNewValueSelector(volume);
  vklValueSelectorSetValues(selector, nv, v);
  vklCommit((VKLObject)selector);

  VklVolume_IsoCrossingS(vkl, rayhit, selector);
}

export void
VklVolume_TestIsoCrossing(uniform VklVolume_ispc *uniform vkl, const uniform int n, void *uniform _rays, uniform int nv, float *uniform v)
{
  uniform RayList_ispc *uniform rays = (uniform RayList_ispc *)_rays;
  GxyRayHit rayhit;
  GxySample sample;

  foreach (i = 0 ... n)
  {
    uniform RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    rayhit.ray.org_x     = rays->ox[i];
    rayhit.ray.org_y     = rays->oy[i];
    rayhit.ray.org_z     = rays->oz[i];
    rayhit.ray.dir_x     = rays->dx[i];
    rayhit.ray.dir_y     = rays->dy[i];
    rayhit.ray.dir_z     = rays->dz[i];
    rayhit.ray.tnear     = rays->t[i];
    rayhit.ray.tfar      = rays->tMax[i];
    rayhit.ray.mask      = -1;
    rayhit.ray.flags     = 0;
    rayhit.hit.geomID    = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    VklVolume_IsoCrossing(vkl, rayhit, nv, v);

    if (rays->tMax[i] > rayhit.ray.tfar)
    {
      rays->tMax[i] = rayhit.ray.tfar;
      rays->term[i] = 1;
    }
    else
      rays->term[i] = 0;
  }
}

float 
VklVolume_Sample(uniform VklVolume_ispc *uniform vkl, vkl_vec3f p)
{
  uniform VKLVolume volume   = vkl->volume;
  uniform VKLSampler sampler = vkl->sampler;
  return vklComputeSampleV((VKLSampler)sampler, &p);
}

export void
VklVolume_TestSample(uniform VklVolume_ispc *uniform vkl, const uniform int n, float *uniform x, float *uniform y, float *uniform z, float *uniform v)
{
  foreach (i = 0 ... n)
  {
    vkl_vec3f vklp = {x[i], y[i], z[i]};
    v[i] = VklVolume_Sample(vkl, vklp);
  }
}

