// ========================================================================== //
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

#include "ospray/SDK/volume/Volume.ih"
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

varying bool GradientSamplerVis_init(void *uniform _self, varying Ray& ray, varying float t)
{
    GradientSamplerVis_ispc *uniform self = (uniform GradientSamplerVis_ispc *)_self;
    vec3f coord = ray.org + t*ray.dir;
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)self)->data;
    self->sLast = vol->sample(vol, coord);
    self->tLast = t;
    return true;
}                                                                                                                            

varying bool GradientSamplerVis_check_interval(void *uniform _self, varying Ray& ray, varying float tThis)
{
    GradientSamplerVis_ispc *uniform self = (uniform GradientSamplerVis_ispc *)_self;
    vec3f coord = ray.org + tThis*ray.dir;
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)self)->data;
    varying float sThis = vol->sample(vol, coord);

    bool hit = false;

    if (((self->sLast < self->tolerance) && (sThis >= self->tolerance)) || ((self->sLast > self->tolerance) && sThis <= self->tolerance))
    {
      self->tHit = self->tLast + (((self->tolerance - self->sLast)/(sThis - self->sLast)) * (tThis - self->tLast));
      hit   = true;
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

