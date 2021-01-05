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

#define _GNU_SOURCE // XXX TODO: what needs this? remove if possible

#include <stdlib.h>
#include "Sampler.h"
#include "SamplerVis.h"
#include "Particles.h"
#include "Rays.h"
#include "RayFlags.h"

#include "SamplerTraceRays.h"

namespace gxy 
{
KEYED_OBJECT_CLASS_TYPE(Sampler)

void
Sampler::Initialize()
{ 
  RegisterClass();
  SamplerVis::Register();
}

#if 0
void
Sampler::initialize()
{
  pthread_mutex_init(&lock, NULL);
}
#endif

void
Sampler::HandleTerminatedRays(RayList *raylist)
{
  int hit_count = 0;

  for (int i = 0; i < raylist->GetRayCount(); i++)
    if (raylist->get_classification(i) & RAY_SURFACE) hit_count++;

  // std::cerr << std::dec;
  // std::cerr << "Sampler::HandleTerminatedRays entry - " << raylist->GetRayCount() << " in, " << hit_count << " terminated\n";

  RenderingSetDPtr  renderingSet  = raylist->GetTheRenderingSet();
  RenderingDPtr rendering = raylist->GetTheRendering();

  if (hit_count == 0) return;

  ParticlesDPtr samples = this->GetSamples();

  samples->Lock();

  for (int i = 0; i < raylist->GetRayCount(); i++)
  {
    if (raylist->get_term(i) & RAY_SURFACE)
    {
      // add a particle, setting position from ray
      Particle newsample;
      newsample.xyz.x = raylist->get_ox(i) + raylist->get_t(i)*raylist->get_dx(i);
      newsample.xyz.y = raylist->get_oy(i) + raylist->get_t(i)*raylist->get_dy(i);
      newsample.xyz.z = raylist->get_oz(i) + raylist->get_t(i)*raylist->get_dz(i);
      newsample.u.value = 0.0;

      samples->push_back(newsample);
    }
  }

  samples->Unlock();

  // std::cerr << "Sampler::HandleTerminatedRays: " << samples->GetNumberOfVertices() << " samples stashed\n";
}

int
Sampler::SerialSize()
{
  return super::SerialSize() + sizeof(Key);
}

unsigned char *
Sampler::Serialize(unsigned char *p)
{
  p = super::Serialize(p);
  *(Key *)p = mSamples->getkey();
  p = p + sizeof(Key);
  return p;
}

unsigned char *
Sampler::Deserialize(unsigned char *p)
{
  p = super::Deserialize(p);
  mSamples = Particles::GetByKey(*(Key *)p);
  p = p + sizeof(Key);
  return p;
}


void
Sampler::Trace(RayList *raylist)
{
  RendererDPtr      renderer      = raylist->GetTheRenderer();
  RenderingDPtr     rendering     = raylist->GetTheRendering();
  VisualizationDPtr visualization = rendering->GetTheVisualization();

  // This is called when a list of rays is pulled off the
  // RayQ.  When we are done with it we decrement the 
  // ray list count (rather than when it was pulled off the
  // RayQ) so we don't send a message upstream saying we are idle
  // until we actually are.

  SamplerTraceRays tracer;
  RayList *out = tracer.Trace(rendering->GetLighting(), visualization, raylist);
}

} // namespace gxy
