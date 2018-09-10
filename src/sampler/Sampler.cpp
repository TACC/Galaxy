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

#include "Sampler.h"
#include "Particles.h"
#include "Rays.h"

namespace gxy 
{
WORK_CLASS_TYPE(Sampler::SampleMsg);

KEYED_OBJECT_TYPE(Sampler)

void
Sampler::Initialize()
{ 
  RegisterClass();
  SampleMsg::Register();
}

void
Sampler::HandleTerminatedRays(RayList *raylist, int *classification)
{
  int terminated_count = 0;

  for (int i = 0; i < raylist->GetRayCount(); i++)
    if (classification[i] == Renderer::TERMINATED) terminated_count++;

  RenderingSetP  renderingSet  = raylist->GetTheRenderingSet();
  RenderingP rendering = raylist->GetTheRendering();

  if (terminated_count == 0) return;

  Renderer::SendPixelsMsg *spmsg = (!rendering->IsLocal()) ? 
    new Renderer::SendPixelsMsg(rendering, renderingSet,
    raylist->GetFrame(), terminated_count) : NULL;

  Particle newsample;
  for (int i = 0; i < raylist->GetRayCount(); i++)
  {
    if (classification[i] == Renderer::TERMINATED)
    {
      if (rendering->IsLocal())
      {
        // add a particle, setting position from ray
        newsample.xyz.x = raylist->get_ox(i);
        newsample.xyz.y = raylist->get_oy(i);
        newsample.xyz.z = raylist->get_oz(i);
        this->GetSamples()->push_back(newsample);
      }
      else
      {
        spmsg->StashPixel(raylist, i);
      }
    }
  }

  if (spmsg)
  {
    if (renderingSet->IsActive(raylist->GetFrame()))
    {
      spmsg->Send(rendering->GetTheOwner());
    }
    else
    {
      delete spmsg;
    }
  }
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
Sampler::Sample(RenderingSetP rs)
{   
  SampleMsg msg(this, rs);
  msg.Broadcast(true, true);
}

Sampler::SampleMsg::SampleMsg(Sampler* r, RenderingSetP rs) :
  Sampler::SampleMsg(sizeof(Key) + r->SerialSize() + sizeof(Key))
{
  unsigned char *p = contents->get();
  *(Key *)p = r->getkey();
  p = p + sizeof(Key);
  p = r->Serialize(p);
  *(Key *)p = rs->getkey();
}

bool
Sampler::SampleMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  unsigned char *p = (unsigned char *)get();
  Key sampler_key = *(Key *)p;
  p += sizeof(Key);

  SamplerP sampler = Sampler::GetByKey(sampler_key);
  p = sampler->Deserialize(p);

  RenderingSetP rs = RenderingSet::GetByKey(*(Key *)p);

  sampler->localRendering(sampler, rs, c);

  return false;
}


} // namespace gxy
