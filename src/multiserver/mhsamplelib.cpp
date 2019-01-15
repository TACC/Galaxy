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

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <cstdlib>
#include <math.h>

#include <dtypes.h>

#include "Application.h"
#include "Datasets.h"
#include "Volume.h"
#include "Particles.h"
#include "MultiServer.h"
#include "MultiServerHandler.h"

#include <time.h>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>

using namespace boost;
using namespace gxy;
using namespace std;

// Two types of functions map volume samples to a probability distribution: a 
// linear transformation that maps linearly with 0 at some data value and 1 at some
// other data value, with 0 everywhere else, and a gaussian transformation with a
// given mean and standard deviation.   Default is linear from the data value min
// to the data value max

#define TF_NONE     0
#define TF_LINEAR   1
#define TF_GAUSSIAN 2

int tf_type = TF_NONE;
float tf0, tf1;

variate_generator<mt19937, normal_distribution<> > *generator;

void normally_distributed_vector(double* v)
{
  for (size_t i = 0; i < 3; ++i)
    v[i]=(*generator)();
}

float gaussian(float x, float m, float s)
{
  return ( 1 / ( s * sqrt(2*M_PI) ) ) * exp( -0.5 * pow( (x-m)/s, 2.0 ) );
}

#define RNDM ((float)rand() / RAND_MAX) 

struct SamplerArgs
{
  Key   vk;             // Key for volume to be sampled
  Key   pk;             // Key for particles object to stash samples
  float radius;         // Radius for particles
  int   tf_type;        // type of transfer function TF_LINEAR or TF_GAUSSIAN  (
  float tf0;            // first parameter of transfer function
  float tf1;            // second parameter of transfer function
  float sigma;          // standard deviation of gaussian step 
  int   n_iterations;   // iteration limit - after the initial skipped iterations
  int   n_startup;      // initial iterations to ignore
  int   n_skip;         // only retain every n_skip'th successful sample
  int   n_miss;         // max number of successive misses allowed before termination
  float r, g, b, a;     // color for spheres
  float istep;          // steps along i, j, and k axes 
  float jstep;
  float kstep;
};

vec3f get_starting_point(VolumeP v)
{
  Box *box = v->get_local_box();
  return vec3f(box->xyz_min.x + RNDM*(box->xyz_max.x - box->xyz_min.x),
               box->xyz_min.y + RNDM*(box->xyz_max.y - box->xyz_min.y),
               box->xyz_min.z + RNDM*(box->xyz_max.z - box->xyz_min.z));
}

float Q(VolumeP v, float s, SamplerArgs *a)
{
  float q;

  if (a->tf_type == TF_LINEAR || a->tf_type == TF_NONE)
  {
    if (a->tf0 < a->tf1)
    {
      if (s < a->tf0) q = 0.0;
      else if (s > a->tf1) q = 1.0;
      else q = (s - a->tf0) / (a->tf1 - a->tf0);
    }
    else
    {
      if (s < a->tf1) q = 1.0;
      else if (s > a->tf0) q = 0.0;
      else q = (s - a->tf0) / (a->tf1 - a->tf0);
    }
  }
  else
  {
    q = gaussian(s, a->tf0, a->tf1);
  }

  return q;
}

float sample(SamplerArgs *args, VolumeP v, float x, float y, float z)
{
  float dx, dy, dz;
  v->get_deltas(dx, dy, dz);

  float ox, oy, oz;
  v->get_ghosted_local_origin(ox, oy, oz);

  x = (x - ox) / dx;
  y = (y - oy) / dy;
  z = (z - oz) / dz;

  int ix = (int)x;
  int iy = (int)y;
  int iz = (int)z;

  dx = x - ix;
  dy = y - iy;
  dz = z - iz;

  int v000 = (ix + 0) * args->istep + (iy + 0) * args->jstep + (iz + 0) * args->kstep;
  int v001 = (ix + 0) * args->istep + (iy + 0) * args->jstep + (iz + 1) * args->kstep;
  int v010 = (ix + 0) * args->istep + (iy + 1) * args->jstep + (iz + 0) * args->kstep;
  int v011 = (ix + 0) * args->istep + (iy + 1) * args->jstep + (iz + 1) * args->kstep;
  int v100 = (ix + 1) * args->istep + (iy + 0) * args->jstep + (iz + 0) * args->kstep;
  int v101 = (ix + 1) * args->istep + (iy + 0) * args->jstep + (iz + 1) * args->kstep;
  int v110 = (ix + 1) * args->istep + (iy + 1) * args->jstep + (iz + 0) * args->kstep;
  int v111 = (ix + 1) * args->istep + (iy + 1) * args->jstep + (iz + 1) * args->kstep;

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

float sample(SamplerArgs *args, VolumeP v, Particle& p) { return sample(args, v, p.xyz.x, p.xyz.y, p.xyz.z); }
float sample(SamplerArgs *args, VolumeP v, vec3f xyz) { return sample(args, v, xyz.x, xyz.y, xyz.z); }

void
Metropolis_Hastings(SamplerArgs *a)
{
  generator = new variate_generator<mt19937, normal_distribution<> >(mt19937(time(0)), normal_distribution<>(0.0, a->sigma));

  VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(a->vk));
  ParticlesP p = Particles::Cast(KeyedDataObject::GetByKey(a->pk));

  p->clear();

  p->CopyPartitioning(v);

  p->SetDefaultColor(a->r, a->g, a->b, a->a);

  float deltaX, deltaY, deltaZ;
  v->get_deltas(deltaX, deltaY, deltaZ);

  float ox, oy, oz;
  v->get_local_origin(ox, oy, oz);

  int lioff, ljoff, lkoff;
  v->get_ghosted_local_offsets(lioff, ljoff, lkoff);

  int nli, nlj, nlk;
  v->get_local_counts(nli, nlj, nlk);

  int gnli, gnlj, gnlk;
  v->get_ghosted_local_counts(gnli, gnlj, gnlk);

  int local_count = nli*nlj*nlk;

  float max_x = ox + (nli-1) * deltaX,
        max_y = oy + (nlj-1) * deltaY,
        max_z = oz + (nlk-1) * deltaZ;

  int ngx, ngy, ngz;
  v->get_local_counts(ngx, ngy, ngz);
  int global_count = ngx*ngy*ngz;

  a->istep = 1;
  a->jstep = gnli;
  a->kstep = gnli * gnlj;

  Particle tp;
  tp.xyz = get_starting_point(v);
  tp.u.value = sample(a, v, tp.xyz);

  float tq = Q(v, tp.u.value, a);

  Box *partition_box = v->get_local_box();

  int miss_count = 0;
  for (int iteration = 0; iteration < (a->n_startup + a->n_iterations); )
  {
    double rv[3];
    normally_distributed_vector(rv);

    Particle cp(tp.xyz.x + rv[0], tp.xyz.y + rv[1], tp.xyz.z + rv[2], float(0.0));
    if (! partition_box->isIn(cp.xyz)) continue;

    iteration ++;

    cp.u.value = sample(a, v, cp);
    float cq = Q(v, cp.u.value, a);

    if ((cq > tq) || (RNDM < (cq/tq)))
    {
      tp = cp;
      tq = cq;
      if ((iteration > a->n_startup) && ((iteration % a->n_skip) == 0))
        p->push_back(tp);
      miss_count = 0;
    }
    else if (++miss_count > a->n_miss)
    {
      tp.xyz = get_starting_point(v);
      tp.u.value = sample(a, v, tp.xyz);
      miss_count = 0;
    }
  }

  std::cerr << "created " << p->get_n_samples() << " samples\n";
}

class SampleMsg : public Work
{
public:
  SampleMsg(SamplerArgs& a) : SampleMsg(sizeof(SamplerArgs))
  {
    *(SamplerArgs *)contents->get() = a;
  }

  WORK_CLASS(SampleMsg, true)

  bool Action(int s)
  {
    Metropolis_Hastings((SamplerArgs *)get());
    return false;
  }
};

WORK_CLASS_TYPE(SampleMsg)

extern "C" void
init()
{
  std::cerr << GetTheApplication()->GetRank() << ": libgxy_module_mhsample.so\n";
  srand(GetTheApplication()->GetRank());
  SampleMsg::Register();
}

extern "C" bool
server(MultiServerHandler *handler)
{
  SamplerArgs args;

  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  args.radius       = 0.02;
  args.tf_type      = TF_NONE;
  args.sigma        = 1.0;
  args.n_iterations = 20000;
  args.n_startup    = 1000;
  args.n_skip       = 10;
  args.n_miss       = 10;

  args.r = args.g = args.b = 0.8; args.a = 1.0;

  VolumeP    volume;
  ParticlesP particles;
  
  bool client_done = false;
  while (! client_done)
  {
    std::string line;
    if (! handler->CRecv(line))
    {
      cerr << "receive failed\n";
      break;
    }

    cerr << "received " << line << "\n";

    stringstream ss(line);

    string cmd;
    ss >> cmd;

    if (cmd == "sigma")       ss >> args.sigma;
    else if (cmd == "radius") ss >> args.radius;
    else if (cmd == "volume") 
    {
      string  name;
      ss >> name;

      volume = Volume::Cast(theDatasets->Find(name));
    }
    else if (cmd == "particles")
    {
      string name;
      ss >> name;

      particles = Particles::NewP();

      theDatasets->Insert(name, particles);
    }
    else if (cmd == "iterations") ss >> args.n_iterations;
    else if (cmd == "startup") ss >> args.n_startup;
    else if (cmd == "skip") ss >> args.n_skip;
    else if (cmd == "miss") ss >> args.n_miss;
    else if (cmd == "color") ss >> args.r >> args.g >> args.b >> args.a;
    else if (cmd == "tf-linear") 
    {
      args.tf_type = TF_LINEAR;
      ss >> args.tf0 >> args.tf1;
    }
    else if (cmd == "tf-gaussian")
    {
      args.tf_type = TF_GAUSSIAN;
      ss >> args.tf0 >> args.tf1;
    }
    else if (cmd == "tf-none")
    {
      args.tf_type = TF_NONE;
    }
    else if (cmd == "sample")
    {
      if (!volume) std::cerr << "need a volume first\n";
      else if (!particles) std::cerr << "need someplace to put samples\n";
      else
      {
        args.vk = volume->getkey();
        args.pk = particles->getkey();

        particles->SetRadius(args.radius);

        if (args.tf_type == TF_NONE)
          volume->get_global_minmax(args.tf0, args.tf1);

        SampleMsg msg(args);
        msg.Broadcast(false, false);

        particles->Commit();
      }
    }
    else
    {
      std::string args;
      std::getline(ss, args);
      handler->handle(cmd, args);
    }

    std::string ok("ok");
    handler->CSend(ok.c_str(), ok.length()+1);
  }

  return true;
}
