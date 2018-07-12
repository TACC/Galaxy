// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>

#include "TraceRays.h"
#include "Lighting.h"
#include "RayFlags.h"

#include "TraceRays_ispc.h"

using namespace std;

namespace gxy
{
TraceRays::TraceRays()
{
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
   ispc::TraceRays_initialize(GetISPC());
}

RayList *
TraceRays::Trace(Lighting& lights, VisualizationP visualization, RayList *raysIn)
{
  ispc::TraceRays_TraceRays(GetISPC(), visualization->GetISPC(), raysIn->GetRayCount(), raysIn->GetISPC());
	RayList *raysOut = NULL;

	int *ao_offsets = new int[raysIn->GetRayCount()];
	int *shadow_offsets = new int[raysIn->GetRayCount()];

  int nAO; float aoRadius;
  lights.GetAO(nAO, aoRadius);
          
  bool do_shadows;
  lights.GetShadowFlag(do_shadows);
  
  int nLights; float *lightVectors;
  lights.GetLights(nLights, lightVectors);

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
	//std::cerr << nOutputRays << " output rays\n";

  if (nOutputRays)
  {
    raysOut = new RayList(raysIn->GetTheRendering(), nOutputRays);
  }
  
  
	if (ao_ray_knt)
		ispc::TraceRays_generateAORays(GetISPC(), lights.GetISPC(), raysIn->GetRayCount(), raysIn->GetISPC(), ao_offsets, raysOut->GetISPC());
	else
		ispc::TraceRays_ambientLighting(GetISPC(), lights.GetISPC(), raysIn->GetRayCount(),  raysIn->GetISPC());

	if (shadow_ray_knt)
		ispc::TraceRays_generateShadowRays(GetISPC(), lights.GetISPC(), raysIn->GetRayCount(), raysIn->GetISPC(), shadow_offsets, raysOut->GetISPC());
	else
		ispc::TraceRays_diffuseLighting(GetISPC(), lights.GetISPC(), raysIn->GetRayCount(), raysIn->GetISPC());

#if 0
  if (raysOut)
    std::cerr << "raysOut->count = " << raysOut->GetRayCount() << "\n";
  else
    std::cerr << "no rays out\n";
#endif
  
	return raysOut;

}

void TraceRays::LoadStateFromValue(Value& v)
{
  float e = v["epsilon"].GetDouble();
  SetEpsilon(e);
}

void TraceRays::SaveStateToValue(Value& v, Document& doc)
{
  Value tv;
  tv.AddMember("epsilon", Value().SetBool(GetEpsilon()), doc.GetAllocator());
  v.AddMember("Tracer", tv, doc.GetAllocator());
}

void TraceRays::SetEpsilon(float e)
{
  ispc::TraceRays_SetEpsilon(GetISPC(), e);
}

float TraceRays::GetEpsilon()
{
  return ispc::TraceRays_GetEpsilon(GetISPC());
}

int TraceRays::SerialSize() { return sizeof(float); }

unsigned char *TraceRays::Serialize(unsigned char *p)
{
  *(float *)p = GetEpsilon();
  return p + sizeof(float);
  
}
unsigned char *TraceRays::Deserialize(unsigned char *p)
{
  SetEpsilon(*(float *)p);
  return p + sizeof(float);
}
}