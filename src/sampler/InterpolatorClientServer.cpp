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

#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <dtypes.h>

#include "Datasets.h"
#include "InterpolatorClientServer.h"
#include "SocketHandler.h"

#include <time.h>

using namespace gxy;
using namespace std;

namespace gxy
{

// Interpolate a point set in a volume 

InterpolatorClientServer::InterpolatorClientServer()
{
  volume = NULL;
  src = NULL;
  dst = NULL;
};

static float interpolate(InterpolatorClientServer::Args *a, VolumeP v, float x, float y, float z)
{
  x = (x - a->ox) / a->dx;
  y = (y - a->oy) / a->dy;
  z = (z - a->oz) / a->dz;

  int ix = (int)x;
  int iy = (int)y;
  int iz = (int)z;

  float dx = x - ix;
  float dy = y - iy;
  float dz = z - iz;

  int v000 = (ix + 0) * a->istride + (iy + 0) * a->jstride + (iz + 0) * a->kstride;
  int v001 = (ix + 0) * a->istride + (iy + 0) * a->jstride + (iz + 1) * a->kstride;
  int v010 = (ix + 0) * a->istride + (iy + 1) * a->jstride + (iz + 0) * a->kstride;
  int v011 = (ix + 0) * a->istride + (iy + 1) * a->jstride + (iz + 1) * a->kstride;
  int v100 = (ix + 1) * a->istride + (iy + 0) * a->jstride + (iz + 0) * a->kstride;
  int v101 = (ix + 1) * a->istride + (iy + 0) * a->jstride + (iz + 1) * a->kstride;
  int v110 = (ix + 1) * a->istride + (iy + 1) * a->jstride + (iz + 0) * a->kstride;
  int v111 = (ix + 1) * a->istride + (iy + 1) * a->jstride + (iz + 1) * a->kstride;

  float b000 = (1.0 - dx) * (1.0 - dy) * (1.0 - dz);
  float b001 = (1.0 - dx) * (1.0 - dy) * (dz);
  float b010 = (1.0 - dx) * (dy) * (1.0 - dz);
  float b011 = (1.0 - dx) * (dy) * (dz);
  float b100 = (dx) * (1.0 - dy) * (1.0 - dz);
  float b101 = (dx) * (1.0 - dy) * (dz);
  float b110 = (dx) * (dy) * (1.0 - dz);
  float b111 = (dx) * (dy) * (dz);

  if (v->get_type() == Volume::FLOAT)
  {
    float *p = (float *)v->get_samples();
    return p[v000]*b000 + p[v001]*b001 + p[v010]*b010 + p[v011]*b011 +
           p[v100]*b100 + p[v101]*b101 + p[v110]*b110 + p[v111]*b111;
  }
  else
  {
    unsigned char *p = (unsigned char *)v->get_samples();
    return p[v000]*b000 + p[v001]*b001 + p[v010]*b010 + p[v011]*b011 +
           p[v100]*b100 + p[v101]*b101 + p[v110]*b110 + p[v111]*b111;
  }
}

float interpolate(InterpolatorClientServer::Args *args, VolumeP v, Particle& p) { return interpolate(args, v, p.xyz.x, p.xyz.y, p.xyz.z); }
float interpolate(InterpolatorClientServer::Args *args, VolumeP v, vec3f xyz) { return interpolate(args, v, xyz.x, xyz.y, xyz.z); }
float interpolate(InterpolatorClientServer::Args *args, VolumeP v, vec3f *xyz) { return interpolate(args, v, xyz->x, xyz->y, xyz->z); }

static void
Interpolate(InterpolatorClientServer::Args *a)
{
  VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(a->vk));
  ParticlesP s = Particles::Cast(KeyedDataObject::GetByKey(a->sk));
  ParticlesP d = Particles::Cast(KeyedDataObject::GetByKey(a->dk));

  d->CopyPartitioning(s);

  v->get_local_origin(a->ox, a->oy, a->oz);
  v->get_deltas(a->dx, a->dy, a->dz);

  int ik, jk, kk;
  v->get_local_counts(ik, jk, kk);

  a->istride = 1;
  a->jstride = ik;
  a->kstride = ik * jk;

  vec3f *srcp = s->GetVertices();
  for (int i = 0; i < s->GetNumberOfVertices(); i++, srcp++)
  {
    Particle p;
    p.xyz = *srcp;
    p.u.value = interpolate(a, v, srcp);
    d->push_back(p);
  }
}

class InterpolatorMsg : public Work
{
public:
  InterpolatorMsg(InterpolatorClientServer* a) : InterpolatorMsg(sizeof(InterpolatorClientServer::Args))
  {
    a->args.vk = a->volume->getkey();
    a->args.sk = a->src->getkey();
    a->args.dk = a->dst->getkey();
    *(InterpolatorClientServer::Args *)contents->get() = a->args;
  }

  WORK_CLASS(InterpolatorMsg, true)

  bool Action(int s)
  {
    Interpolate((InterpolatorClientServer::Args *)get());
    return false;
  }
};

WORK_CLASS_TYPE(InterpolatorMsg)

extern "C" void
init()
{
  srand(GetTheApplication()->GetRank());
  InterpolatorMsg::Register();
}

extern "C" MultiServerHandler *
new_handler(SocketHandler *sh)
{
  MultiServerHandler *msh = new InterpolatorClientServer;
  msh->SetSocketHandler(sh);
  return msh;
}

bool
InterpolatorClientServer::handle(std::string line, std::string &reply)
{
  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  stringstream ss(line);

  string cmd;
  ss >> cmd;

  if (cmd == "interpolate")
  {
    string vname, sname, dname;
    ss >> vname >> sname >> dname;
    if (ss.fail())
    {
      reply = std::string("error Interpolator interpolate command requires three string naming a volume, a pre-existing particles set and a new particles set");
      return true;
    }

    volume = Volume::Cast(theDatasets->Find(vname));
    if (! volume) 
    {
      reply = "error Interpolator first arg must name a volume dataset";
      return true;
    }

    src = Particles::Cast(theDatasets->Find(sname));
    if (! src) 
    {
      reply = "error Interpolator second arg must name a pre-existing particles dataset";
      return true;
    }

    dst = Particles::NewP();
    theDatasets->Insert(dname, dst);

    InterpolatorMsg msg(this);
    msg.Broadcast(false, false);

    if (! dst->Commit())
    {
      reply = "error committing interpolated particles dataset";
      return true;
    }

    reply = "ok";
    return true;
  }
  else
    return false;
}

}
