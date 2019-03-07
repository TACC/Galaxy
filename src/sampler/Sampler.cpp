// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 k/
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

//#define _GNU_SOURCE // XXX TODO: what needs this? remove if possible
#include <sched.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <math.h>

#include <ospray/ospray.h>

#include "galaxy.h"

#include "Sampler.h"
#include "Renderer.h"
// #include "Application.h"
// #include "Camera.h"
// #include "DataObjects.h"
// #include "Pixel.h"
// #include "RayFlags.h"
// #include "RayQManager.h"
// #include "Renderer.h"
// #include "TraceRays.h"
// #include "Work.h"
// #include "Threading.h"

// #include "Rays_ispc.h"
// #include "TraceRays_ispc.h"
// #include "Visualization_ispc.h"

// #include "OVolume.h"
// #include "OTriangles.h"
// #include "OParticles.h"

// #include "rapidjson/document.h"
// #include "rapidjson/stringbuffer.h"

#ifdef GXY_TIMING
#include "Timer.h"
static gxy::Timer timer("ray_processing");
#endif

using namespace rapidjson;
using namespace std;

namespace gxy
{
KEYED_OBJECT_CLASS_TYPE(Sampler)

Sampler::~Sampler()
{
}

void
Renderer::HandleTerminatedRays(RayList *raylist, int *classification)
{
}

} // namespace gxy
