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

#pragma once 

/*! \file Sampler.h 
 * \brief a class that takes a ray-based approach to sampling 
 * \ingroup render
 */

#include <vector>

#include "KeyedObject.h"
#include "Renderer.h"
// #include "Datasets.h"
// #include "pthread.h"
// #include "Rays.h"
// #include "Visualization.h"
// #include "Rendering.h"
// #include "RenderingEvents.h"
// #include "RenderingSet.h"
// #include "TraceRays.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Sampler)

OBJECT_POINTER_TYPES(Sampler)

class Sampler : public Renderer
{
  KEYED_OBJECT(Sampler)
    
public:
  virtual ~Sampler(); //!< default destructor
  virtual void HandleTerminatedRays(RayList *raylist, int *classification);

};

} // namespace gxy
