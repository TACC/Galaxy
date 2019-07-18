// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
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

#include "SamplerTraceRays.h"
#include "SamplerTraceRays_ispc.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string>

#include "galaxy.h"
#include "RayFlags.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

SamplerTraceRays::SamplerTraceRays()
{
  allocate_ispc();
  initialize_ispc();
}

SamplerTraceRays::~SamplerTraceRays()
{
}

RayList *
SamplerTraceRays::Trace(Lighting* lights, VisualizationP visualization, RayList *raysIn)
{
  ispc::SamplerTraceRays_SamplerTraceRays(GetIspc(), visualization->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc());
	return NULL;
}


} // namespace gxy

