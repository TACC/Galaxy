// ========================================================================== //
//                                                                            //
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

#include "galaxy.h"
#include "RaycastSampler.hpp"

#include "GradientSamplerVis.h"
#include "IsoSamplerVis.h"

namespace gxy
{

int RaycastSampler::rcsfilter_index = 0;

void
RaycastSampler::Sample(rapidjson::Document& doc, KeyedDataObjectDPtr scalarVolume)
{
  camera->LoadFromJSON(doc["camera"]);
  camera->Commit();

  int   type     = doc["type"].GetInt();

  VisDPtr vis;
  if (type == 0)
  {
    IsoSamplerVisDPtr iv = IsoSamplerVis::NewDistributed();
    iv->SetIsovalue(doc["isovalue"].GetDouble());
    vis = Vis::Cast(iv);
  }
  else
  {
    GradientSamplerVisDPtr gv = GradientSamplerVis::NewDistributed();
    gv->SetTolerance(doc["tolerance"].GetDouble());
    vis = Vis::Cast(gv);
  }

  vis->Commit(scalarVolume->getkey());

  VisualizationDPtr visualization = Visualization::NewDistributed();
  visualization->AddVis(vis);
  visualization->Commit();

  RenderingDPtr r = Rendering::NewDistributed();
  r->SetTheOwner(0);
  r->SetTheDatasets(Datasets::NewDistributed());
  r->SetTheCamera(camera);
  r->SetTheVisualization(visualization);
  r->Commit();

  RenderingSetDPtr rs = RenderingSet::NewDistributed();
  rs->AddRendering(r);
  rs->Commit();

  sampler->GetSamples()->clear();

  sampler->Commit();
  sampler->Start(rs);

#if defined(GXY_WRITE_IMAGES)
  rs->WaitForDone();
#endif

  sleep(1);
  result->Commit();

  ParticlesDPtr p = Particles::Cast(result);

  if (! p) 
    std::cerr << "samples aren't particle set?\n";
  else
    std::cerr << "Totals: " << p->GetVertexCount() << " vertices, " << p->GetElementCount() << " elements\n";
}

}

