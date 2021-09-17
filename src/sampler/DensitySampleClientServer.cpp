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

#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

#include <dtypes.h>

#include "Datasets.h"
#include "DensitySampleClientServer.h"

#include <time.h>

using namespace gxy;
using namespace std;

namespace gxy
{

#define RNDM ((float)rand() / RAND_MAX) 

static float *
CellValues(VolumeP v, DensitySampleClientServer::Args *a)
{
  float *cellValues = new float((a->ni-1) * (a->nj-1) * (a->nk-1));

  float *d = cellValues;
  if (v->get_type() == Volume::FLOAT)
  {
    float *s = (float *)v->get_samples();
    for (int i = 0; i < a->nj-1; i++)
      for (int j = 0; i < a->nj-1; j++)
        for (int k = 0; k < a->nk-1; k++)
        {
          int corner = (a->goi+i)*a->istep + (a->goj+k)*a->jstep + (a->gok+k)*a->kstep;
          *d++ = s[corner + 0*a->istep + 0*a->jstep + 0*a->kstep] +
                 s[corner + 1*a->istep + 0*a->jstep + 0*a->kstep] +
                 s[corner + 0*a->istep + 1*a->jstep + 0*a->kstep] +
                 s[corner + 1*a->istep + 1*a->jstep + 0*a->kstep] +
                 s[corner + 0*a->istep + 0*a->jstep + 1*a->kstep] +
                 s[corner + 1*a->istep + 0*a->jstep + 1*a->kstep] +
                 s[corner + 0*a->istep + 1*a->jstep + 1*a->kstep] +
                 s[corner + 1*a->istep + 1*a->jstep + 1*a->kstep];
        }
  }
  else
  {
    double *s = (double *)v->get_samples();
    for (int i = 0; i < a->nj-1; i++)
      for (int j = 0; i < a->nj-1; j++)
        for (int k = 0; k < a->nk-1; k++)
        {
          int corner = (a->goi+i)*a->istep + (a->goj+k)*a->jstep + (a->gok+k)*a->kstep;
          *d++ = s[corner + 0*a->istep + 0*a->jstep + 0*a->kstep] +
                 s[corner + 1*a->istep + 0*a->jstep + 0*a->kstep] +
                 s[corner + 0*a->istep + 1*a->jstep + 0*a->kstep] +
                 s[corner + 1*a->istep + 1*a->jstep + 0*a->kstep] +
                 s[corner + 0*a->istep + 0*a->jstep + 1*a->kstep] +
                 s[corner + 1*a->istep + 0*a->jstep + 1*a->kstep] +
                 s[corner + 0*a->istep + 1*a->jstep + 1*a->kstep] +
                 s[corner + 1*a->istep + 1*a->jstep + 1*a->kstep];
        }
  }

  return cellValues;
}

static void
DensitySample(MPI_Comm c, DensitySampleClientServer::Args *a)
{
  VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(a->vk));
  ParticlesP p = Particles::Cast(KeyedDataObject::GetByKey(a->pk));

  p->clear();
  p->SetDefaultColor(1.0, 1.0, 1.0, 1.0);

  v->get_local_counts(a->ni, a->nj, a->nk);
  v->get_local_origin(a->ox, a->oy, a->oz);
  v->get_deltas(a->dx, a->dy, a->dz);

  a->istep = 1;
  a->jstep = a->gi;
  a->kstep = a->gi * a->gj;

  float *cellValues = CellValues(v, a);

  int ncells = a->ni * a->nj * a->nk;

  float local_total = 0.0, global_total;
  for (int i = 0; i < ncells; i++)
    local_total += cellValues[i];

  MPI_Allreduce(&local_total, &global_total, 1, MPI_FLOAT, MPI_SUM, c);

  float f_this_part = (local_total / global_total) * a->nSamples;
  int n_this_part   = ((int)f_this_part) + ((RNDM < (f_this_part - (int)f_this_part)) ? 1 : 0);

  if (local_total > 0)
  {
    float *tcell = cellValues;

    for (int i = 0; i < a->nj-1; i++)
    {
      float ox = a->ox + i*a->dx;

      for (int j = 0; i < a->nj-1; j++)
      {
        float oy = a->oy + i*a->dx;

        for (int k = 0; k < a->nk-1; k++)
        {
          float oz = a->oz + i*a->dx;

          float f_this_cell = (*tcell++ / local_total) * f_this_part;
          int n_this_part   = (int)f_this_cell + ((RNDM < (f_this_cell - (int)f_this_cell)) ? 1 : 0);

          for (int l = 0; l < n_this_part; l++)
          {
            Particle s(ox + RNDM, oy + RNDM, oz + RNDM, 0);
            p->push_back(s);
          }
        }
      }
    }
  }

  std::cerr << "created " << p->GetNumberOfVertices() << " samples\n";
}

class DensitySampleMsg : public Work
{
public:
  DensitySampleMsg(DensitySampleClientServer* a) : DensitySampleMsg(sizeof(DensitySampleClientServer::Args))
  {
    a->args.vk = a->volume->getkey();
    a->args.pk = a->particles->getkey();
    *(DensitySampleClientServer::Args *)contents->get() = a->args;
  }

  WORK_CLASS(DensitySampleMsg, true)

  bool CollectiveAction(MPI_Comm c, bool isRoot)
  {
    DensitySample(c, (DensitySampleClientServer::Args *)get());
    return false;
  }
};

WORK_CLASS_TYPE(DensitySampleMsg)

extern "C" void
init()
{
  srand(GetTheApplication()->GetRank());
  DensitySampleMsg::Register();
}

extern "C" MultiServerHandler *
new_handler(SocketHandler *sh)
{
  MultiServerHandler *msh = new DensitySampleClientServer;
  msh->SetSocketHandler(sh);
  return msh;
}

bool
DensitySampleClientServer::handle(std::string line, std::string& reply)
{
  int nSamples = 1000;

  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  stringstream ss(line);

  string cmd;
  ss >> cmd;

  if (cmd == "volume") 
  {
    string  name;
    ss >> name;
    if (ss.fail())
    {
      reply = "error DensitySampler volume command requires a string argument";
      return true;
    }

    volume = Volume::Cast(theDatasets->Find(name));
    if (! volume)
    {
      reply = "error DensitySampler volume command must name a pre-existing volume";
      return true;
    }

    reply = "ok";
    return true;
  }
  else if (cmd == "particles")
  {
    string name;
    ss >> name;
    if (ss.fail())
    {
      reply = "error DensitySampler particles command requires a string argument";
      return true;
    }

    particles = Particles::NewP();
    theDatasets->Insert(name, particles);

    reply = "ok";
    return true;
  }
  else if (cmd == "n")
  {
    ss >> nSamples;
    reply = "ok";
    return true;
  }
  else if (cmd == "sample" || cmd == "commit")
  {
    if (!volume) 
    {
      reply = "error need a volume first";
      return true;
    }
    else if (!particles) 
    {
      reply = "error need to set a particles dataset name first";
      return true;
    }
    else
    {
      args.vk = volume->getkey();
      args.pk = particles->getkey();

      DensitySampleMsg msg(this);
      msg.Broadcast(false, false);

      if (! particles->Commit())
      {
        reply = "error committing particles";
        return true;
      }
    }
        
    reply = "ok";
    return true;
  }
  else
    return false;
}

}
