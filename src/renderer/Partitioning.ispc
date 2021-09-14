// Copyright (c) 2016-2018 The University of Texas at Austin.                 //
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

#include "Partitioning.ih"

void Partitioning_ZRange(void *uniform _self, const Ray& ray, float &near, float &far)
{
  uniform Partitioning_ispc *uniform self = (uniform Partitioning_ispc *)_self;

  //print("BOX\nLX %\nLY %\nLZ %\n", self->box.lower.x, self->box.lower.y, self->box.lower.z);
  //print("UX %\nUY %\nUZ %\n", self->box.upper.x, self->box.upper.y, self->box.upper.z);
  
  const vec3f mins = (self->box.lower - ray.org) * rcp(ray.dir);
  const vec3f maxs = (self->box.upper - ray.org) * rcp(ray.dir);

  //print("MINS\nX %\nY %\nZ %\n", mins.x, mins.y, mins.z);
  //print("MAXS\nX %\nY %\nZ %\n", maxs.x, maxs.y, maxs.z);

  near = max(min(mins.x,maxs.x),
           max(min(mins.y,maxs.y),
               min(mins.z,maxs.z)));

  far = min(max(mins.x,maxs.x),
           min(max(mins.y,maxs.y),
               max(mins.z,maxs.z)));
}


export void *uniform Partitioning_allocate()
{
  Partitioning_ispc *uniform v = uniform new uniform Partitioning_ispc;
  return (void *)v;
}   

export void Partitioning_initialize(void *uniform _self, uniform box3f& b)
{
  Partitioning_ispc *uniform self = (uniform Partitioning_ispc *)_self;
  self->box = b;
  self->ZRange = Partitioning_ZRange;
} 
  
export void Partitioning_destroy(void *uniform _self)
{ 
  Partitioning_ispc *uniform self = (uniform Partitioning_ispc *)_self;
}
