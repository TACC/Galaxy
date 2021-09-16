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

#include "Schlieren2TraceRays.h"
#include "Schlieren2TraceRays_ispc.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string>

#include "RayFlags.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

Schlieren2TraceRays::Schlieren2TraceRays(PartitioningP p)
{
  partitioning = p;
  allocate_ispc();
  initialize_ispc();
}

Schlieren2TraceRays::~Schlieren2TraceRays()
{
}

RayList *
Schlieren2TraceRays::Trace(Lighting* lights, VisualizationP visualization, RayList *raysIn)
{
  ispc::Schlieren2TraceRays_Schlieren2TraceRays(GetIspc(), visualization->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc(), partitioning->GetIspc());
	return NULL;

}

} // namespace gxy
