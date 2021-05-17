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

#include "TraceRays.h"
#include "TraceRays_ispc.h"

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

TraceRays::TraceRays(float e)
{
  epsilon = e;
  allocate_ispc();
  initialize_ispc();
}

TraceRays::~TraceRays()
{
}

void
TraceRays::allocate_ispc()
{
  ispc = ispc::TraceRays_allocate();
}

void
TraceRays::initialize_ispc()
{
  ispc::TraceRays_initialize(GetIspc());
}

void    
TraceRays::destroy_ispc()
{            
  ispc::TraceRays_destroy(GetIspc());
}

RayList *
TraceRays::Trace(Lighting* lights, VisualizationP visualization, RayList *raysIn)
{
std::cerr << "TRACE on " << GetTheApplication()->GetRank() << "\n";

  ispc::TraceRays_TraceRays(GetIspc(), visualization->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc(), epsilon);
	RayList *raysOut = NULL;

	int nl, *t; float *l;
	lights->GetLights(nl, l, t);

	int *ao_offsets = new int[raysIn->GetRayCount()];
	int *shadow_offsets = new int[raysIn->GetRayCount()];

  int nAO; float aoRadius;
  lights->GetAO(nAO, aoRadius);
          
  bool do_shadows;
  lights->GetShadowFlag(do_shadows);
  
  int nLights; float *lightVectors; int *types;
  lights->GetLights(nLights, lightVectors, types);

	int nOutputRays = 0; 
	for (int i = 0; i < raysIn->GetRayCount(); i++)
	{
		ao_offsets[i] = nOutputRays;

		if (raysIn->get_type(i) == RAY_PRIMARY)
		{
			if (raysIn->get_term(i) & RAY_SURFACE)
				nOutputRays += nAO;
		}
	}

	int ao_ray_knt = nOutputRays;

	if (do_shadows)
	{
		for (int i = 0; i < raysIn->GetRayCount(); i++)
		{
			if ((raysIn->get_type(i) == RAY_PRIMARY) && (raysIn->get_term(i) & RAY_SURFACE))
			{
				shadow_offsets[i] = nOutputRays;
				nOutputRays += (do_shadows ? nLights : 0);
			}
			else
				shadow_offsets[i] = -1;
		}
	}

	int shadow_ray_knt = nOutputRays - ao_ray_knt;

  if (nOutputRays)
  {
    raysOut = new RayList(raysIn->GetTheRenderer(), raysIn->GetTheRenderingSet(), raysIn->GetTheRendering(), nOutputRays, raysIn->GetFrame(), RayList::SECONDARY);
  }
  
#ifdef GXY_REVERSE_LIGHTING
	ispc::TraceRays_ambientLighting(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(),  raysIn->GetIspc());
	if (ao_ray_knt)
		ispc::TraceRays_generateAORays(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc(), ao_offsets, raysOut->GetIspc(), epsilon);
	
	ispc::TraceRays_diffuseLighting(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc());
	if (shadow_ray_knt)
		ispc::TraceRays_generateShadowRays(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc(), shadow_offsets, raysOut->GetIspc(), epsilon);
#else
	if (ao_ray_knt)
		ispc::TraceRays_generateAORays(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc(), ao_offsets, raysOut->GetIspc(), epsilon);
	else
		ispc::TraceRays_ambientLighting(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(),  raysIn->GetIspc());
	
	if (shadow_ray_knt)
		ispc::TraceRays_generateShadowRays(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc(), shadow_offsets, raysOut->GetIspc(), epsilon);
	else
		ispc::TraceRays_diffuseLighting(GetIspc(), lights->GetIspc(), raysIn->GetRayCount(), raysIn->GetIspc());
#endif

	return raysOut;

}

} // namespace gxy
