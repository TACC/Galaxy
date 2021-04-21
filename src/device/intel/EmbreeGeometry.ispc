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

#include "EmbreeGeometry.ih"

#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"

// force export of EmbreeGeometry_ispc in EmbreeGeometry_ispc.h

export void EmbreeGeometry_dummy(uniform EmbreeGeometry_ispc * uniform egi) {}

export void EmbreeGeometry_postIntersect(uniform int, void *uniform _self, void *uniform rayhit, void *uniform sample)
{
  print("EmbreeGeometry_postIntersect\n");
}
