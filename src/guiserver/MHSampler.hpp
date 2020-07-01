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
#include <sstream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

#include <dtypes.h>

#include "rapidjson/document.h"

#include "Volume.h"
#include "Particles.h"
#include "Datasets.h"
#include "Filter.h"

#include <time.h>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>

using namespace boost;
using namespace gxy;
using namespace std;

namespace gxy
{

enum tfer_type
{
  TF_NONE=0,
  TF_LINEAR=1,
  TF_GAUSSIAN=2
};

struct mhArgs : public filterArgs
{
  tfer_type tf_type;   // none, linear, gaussian
  int iterations;      // iteration limit - after the initial skipped iterations
  int startup;         // initial iterations to ignore
  int skip;            // only retain every skip'th successful sample
  int miss;            // max number of successive misses allowed before termination

  float radius;        // Radius for particles
  float sigma;         // type of transfer function TF_LINEAR or TF_GAUSSIAN
  float linear_tf_min; // min value of linear transfer function
  float linear_tf_max; // max value of linear transfer function
  float gaussian_mean; // gaussian transfer function mean
  float gaussian_std;  // gaussian transfer function standard deviation

  float istep;          // steps along i, j, and k axes
  float jstep;
  float kstep;
};

class MHSamplerMsg : public Work
{
public:
  MHSamplerMsg(mhArgs* a) : MHSamplerMsg(sizeof(mhArgs))
  {
    *(mhArgs *)contents->get() = *a;
  }

  WORK_CLASS(MHSamplerMsg, true);

  bool Action(int s);
};

class MHSampler : public Filter
{
  static int mhfilter_index;

// Two types of functions map volume samples to a probability distribution: a 
// linear transformation that maps linearly with 0 at some data value and 1 at some
// other data value, with 0 everywhere else, and a gaussian transformation with a
// given mean and standard deviation.   Default is linear from the data value min
// to the data value max

public:

  static void
  init()
  {
    srand(GetTheApplication()->GetRank());
    MHSamplerMsg::Register();
  }

  MHSampler(KeyedDataObjectP source)
  {
    std::stringstream ss;
    ss << "MHFilter_" << mhfilter_index++;
    name = ss.str();

    result = KeyedDataObject::Cast(Particles::NewP());
    result->CopyPartitioning(source);

  }

  ~MHSampler() { std::cerr << "MHSampler dtor\n"; }

  void
  Sample(rapidjson::Document& doc, KeyedDataObjectP scalarVolume)
  {
    mhArgs args;
    
    args.sourceKey         = scalarVolume->getkey();
    args.destinationKey    = result->getkey();

    args.radius         = doc["mh_radius"].GetDouble();
    args.sigma          = doc["mh_sigma"].GetDouble();
    args.iterations     = doc["mh_iterations"].GetInt();
    args.startup        = doc["mh_startup"].GetInt();
    args.skip           = doc["mh_skip"].GetInt();
    args.miss           = doc["mh_miss"].GetInt();
    args.linear_tf_min  = doc["mh_linear_tf_min"].GetDouble();
    args.linear_tf_max  = doc["mh_linear_tf_max"].GetDouble();
    args.gaussian_mean  = doc["gaussian_mean"].GetDouble();
    args.gaussian_std   = doc["gaussian_std"].GetDouble();

    MHSamplerMsg msg(&args);
    msg.Broadcast(false, true);

    result->Commit();
  }

  static void normally_distributed_vector(double* v, variate_generator<mt19937, normal_distribution<> >* g)
  {
    for (size_t i = 0; i < 3; ++i)
      v[i]=(*g)();
  }

  static float gaussian(float x, float m, float s)
  {
    return ( 1 / ( s * sqrt(2*M_PI) ) ) * exp( -0.5 * pow( (x-m)/s, 2.0 ) );
  }

#define RNDM ((float)rand() / RAND_MAX) 

  static vec3f get_starting_point(VolumeP v)
  {
    Box *box = v->get_local_box();
    return vec3f(box->xyz_min.x + RNDM*(box->xyz_max.x - box->xyz_min.x),
               box->xyz_min.y + RNDM*(box->xyz_max.y - box->xyz_min.y),
               box->xyz_min.z + RNDM*(box->xyz_max.z - box->xyz_min.z));
  }

  static float Q(VolumeP v, float s, mhArgs *a)
  {
    float q;

    if (a->tf_type == TF_LINEAR || a->tf_type == TF_NONE)
    {
      if (a->linear_tf_min < a->linear_tf_max)
      {
        if (s < a->linear_tf_min) q = 0.0;
        else if (s > a->linear_tf_max) q = 1.0;
        else q = (s - a->linear_tf_min) / (a->linear_tf_max - a->linear_tf_min);
      }
      else
      {
        if (s < a->linear_tf_max) q = 1.0;
        else if (s > a->linear_tf_min) q = 0.0;
        else q = (s - a->linear_tf_min) / (a->linear_tf_max - a->linear_tf_min);
      }
    }
    else
    {
      q = gaussian(s, a->gaussian_mean, a->gaussian_std);
    }
    return q;
  }

  static float sample(mhArgs *args, VolumeP v, float x, float y, float z)
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

  static float sample(mhArgs *args, VolumeP v, Particle& p) { return sample(args, v, p.xyz.x, p.xyz.y, p.xyz.z); }
  static float sample(mhArgs *args, VolumeP v, vec3f xyz) { return sample(args, v, xyz.x, xyz.y, xyz.z); }

  static void
  Metropolis_Hastings(mhArgs *a)
  {
    variate_generator<mt19937, normal_distribution<> > generator(mt19937(time(0)), normal_distribution<>(0.0, a->sigma));
  
    VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(a->sourceKey));
    ParticlesP p = Particles::Cast(KeyedDataObject::GetByKey(a->destinationKey));

    p->setModified(true);
    p->clear();
    p->CopyPartitioning(v);
  
    p->SetDefaultColor(1.0, 1.0, 1.0, 1.0);
  
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
    for (int iteration = 0; iteration < (a->startup + a->iterations); )
    {
      double rv[3];
      normally_distributed_vector(rv, &generator);
  
      Particle cp(tp.xyz.x + rv[0], tp.xyz.y + rv[1], tp.xyz.z + rv[2], float(0.0));
      if (! partition_box->isIn(cp.xyz)) continue;
  
      iteration ++;
  
      cp.u.value = sample(a, v, cp);
      float cq = Q(v, cp.u.value, a);
  
      if ((cq > tq) || (RNDM < (cq/tq)))
      {
        tp = cp;
        tq = cq;
        if ((iteration > a->startup) && ((iteration % a->skip) == 0))
          p->push_back(tp);
        miss_count = 0;
      }
      else if (++miss_count > a->miss)
      {
        tp.xyz = get_starting_point(v);
        tp.u.value = sample(a, v, tp.xyz);
        miss_count = 0;
      }
    }
  
    std::cerr << "created " << p->GetNumberOfVertices() << " samples\n";
  }

private:
  variate_generator<mt19937, normal_distribution<> > *generator;
};

}
