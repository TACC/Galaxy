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
#include "SamplerVis.ih"

export void *uniform SamplerVis_allocate()
{
    SamplerVis_ispc *uniform v = uniform new uniform SamplerVis_ispc;
    return (void *)v;
}

export void SamplerVis_set_tolerance(void *uniform _self, uniform float t)
{
    SamplerVis_ispc *uniform self = (uniform SamplerVis_ispc *)_self;
    self->tolerance = t;
}                                                                                                                            

varying bool SamplerVis_init(void *uniform _self, varying Ray& ray, varying float t)
{
    SamplerVis_ispc *uniform self = (uniform SamplerVis_ispc *)_self;
    vec3f coord = ray.org + t*ray.dir;
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)self)->data;
    self->sLast = vol->sample(vol, coord);
    self->tLast = t;
    return true;
}                                                                                                                            
varying bool SamplerVis_check_interval(void *uniform _self, varying Ray& ray, varying float tThis)
{
    SamplerVis_ispc *uniform self = (uniform SamplerVis_ispc *)_self;
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

float SamplerVis_get_sample_t(void *uniform _self)
{
    SamplerVis_ispc *uniform self = (uniform SamplerVis_ispc *)_self;
    return self->tHit;
}                                                                                                                            

export void SamplerVis_initialize(void *uniform _self)
{
    SamplerVis_ispc *uniform self = (uniform SamplerVis_ispc *)_self;
    self->init = SamplerVis_init;
    self->check_interval = SamplerVis_check_interval;
    self->get_sample_t = SamplerVis_get_sample_t;
    self->tolerance = 0;
}

