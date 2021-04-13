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

//#include "ospray/SDK/volume/Volume.ih"

#include "Rays.ih"
#include "GradientSamplerVis.ih"

export void *uniform GradientSamplerVis_allocate()
{
    GradientSamplerVis_ispc *uniform v = uniform new uniform GradientSamplerVis_ispc;
    return (void *)v;
}

export void GradientSamplerVis_set_tolerance(void *uniform _self, uniform float t)
{
    GradientSamplerVis_ispc *uniform self = (uniform GradientSamplerVis_ispc *)_self;
    self->tolerance = t;
}                                                                                                                            

varying bool GradientSamplerVis_init(void *uniform _self, varying RTCRayHit& ray, varying float t)
{
    GradientSamplerVis_ispc *uniform self = (uniform GradientSamplerVis_ispc *)_self;
    vec3f coord = make_vec3f(ray.org_x + t*ray.dir_x, 0, 0);
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)self)->data;

    self->sLast = vol->computeGradient(vol, coord);
    self->tLast = t;
    return true;
}                                                                                                                            

varying bool GradientSamplerVis_check_interval(void *uniform _self, varying Ray& ray, varying float tThis)
{
    GradientSamplerVis_ispc *uniform self = (uniform GradientSamplerVis_ispc *)_self;
    vec3f coord = ray.org + tThis*ray.dir;
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)self)->data;
    varying vec3f sThis = vol->computeGradient(vol, coord);

    bool hit = false;

    // float dotValue = dot(safe_normalize(sThis), safe_normalize(self->sLast));
    float dotValue = dot(sThis, self->sLast);
#ifdef DEBUG_GRADIENTSAMPLERVIS
    print("VECTOR\n%\n,%\n,%\n,%\n,%\n,%\n", coord.x, coord.y, coord.z,  sThis.x, sThis.y, sThis.z);
#endif
    if (dotValue < self->tolerance)
    {
      self->tHit = (self->tLast + tThis)/2.0;
      hit = true;
    }

    self->sLast = sThis;
    self->tLast = tThis;
    return hit;
}                                                                                                                            

float GradientSamplerVis_get_sample_t(void *uniform _self)
{
    GradientSamplerVis_ispc *uniform self = (uniform GradientSamplerVis_ispc *)_self;
    return self->tHit;
}                                                                                                                            

export void GradientSamplerVis_initialize(void *uniform _self)
{
    SamplerVis_ispc *uniform super = (uniform SamplerVis_ispc *)_self;
    super->init = GradientSamplerVis_init;
    super->check_interval = GradientSamplerVis_check_interval;
    super->get_sample_t = GradientSamplerVis_get_sample_t;

    GradientSamplerVis_ispc *uniform self = (uniform GradientSamplerVis_ispc *)_self;
    self->tolerance = 0;
}

