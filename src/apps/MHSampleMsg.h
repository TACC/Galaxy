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
#include <cstdlib>
#include <math.h>

#include <dtypes.h>
#include <Application.h>

#include <time.h>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>

using namespace boost;
using namespace gxy;
using namespace std;

static variate_generator<mt19937, normal_distribution<> > *generator = NULL;
static float RNDM() { return  (float)rand() / RAND_MAX; }

class MHSampleMsg : public Work
{
    struct args
    {
        Key vk;
        Key pk;
        float p0_value;
        float p1_value;
        float step;
        int   n_iterations;
        int   n_start;
        int   n_skip;
        int   n_miss;
    };

public:
  MHSampleMsg(VolumeP v, ParticlesP p, float p0, float p1, float step, int niterations, int nstart, int nskip, int nmiss)
            : MHSampleMsg(sizeof(args))
  {
        args *a = (args *)contents->get();
        a->vk = v->getkey();
        a->pk = p->getkey();
        a->p0_value = p0;
        a->p1_value = p1;
        a->step = step;
        a->n_iterations = niterations;
        a->n_start = nstart;
        a->n_skip = nskip;
        a->n_miss = nmiss;
    }

  ~MHSampleMsg() {}

  WORK_CLASS(MHSampleMsg, true)

	virtual void initialize()
	{
		if (! generator)
		{
			generator = new variate_generator<mt19937, normal_distribution<> >(mt19937(time(0)), normal_distribution<>(0.0, 1.0));
		}
	}

private: 
  float istep, jstep, kstep;

	void normally_distributed_vector(double* v)
	{
		for (size_t i = 0; i < 3; ++i)
			v[i]=(*generator)();
	}


	vec3f get_starting_point(VolumeP v)
	{
			Box *box = v->get_local_box();
			return vec3f(box->xyz_min.x + RNDM()*(box->xyz_max.x - box->xyz_min.x),
									 box->xyz_min.y + RNDM()*(box->xyz_max.y - box->xyz_min.y),
									 box->xyz_min.z + RNDM()*(box->xyz_max.z - box->xyz_min.z));
	}

  float Q(VolumeP v, float s, args *a)
  {
      if (a->p0_value < a->p1_value)
      {
        if (s < a->p0_value) return 0.0;
        else if (s > a->p1_value) return 1.0;
        else return (s - a->p0_value) / (a->p1_value - a->p0_value);
      }
      else
      {
        if (s < a->p1_value) return 1.0;
        else if (s > a->p0_value) return 0.0;
        else return (s - a->p0_value) / (a->p1_value - a->p0_value);
      }
  }


  float sample(VolumeP v, Particle& p) { return sample(v, p.xyz.x, p.xyz.y, p.xyz.z); }
  float sample(VolumeP v, vec3f xyz) { return sample(v, xyz.x, xyz.y, xyz.z); }
  float sample(VolumeP v, float x, float y, float z)
  {
    float dx, dy, dz;
    v->get_deltas(dx, dy, dz);

    float ox, oy, oz;
    v->get_local_origin(ox, oy, oz);

    x = (x - ox) / dx;
    y = (y - oy) / dy;
    z = (z - oz) / dz;

    int ix = (int)x;
    int iy = (int)y;
    int iz = (int)z;

    dx = x - ix;
    dy = y - iy;
    dz = z - iz;

    int v000 = (ix + 0) * istep + (iy + 0) * jstep + (iz + 0) * kstep;
    int v001 = (ix + 0) * istep + (iy + 0) * jstep + (iz + 1) * kstep;
    int v010 = (ix + 0) * istep + (iy + 1) * jstep + (iz + 0) * kstep;
    int v011 = (ix + 0) * istep + (iy + 1) * jstep + (iz + 1) * kstep;
    int v100 = (ix + 1) * istep + (iy + 0) * jstep + (iz + 0) * kstep;
    int v101 = (ix + 1) * istep + (iy + 0) * jstep + (iz + 1) * kstep;
    int v110 = (ix + 1) * istep + (iy + 1) * jstep + (iz + 0) * kstep;
    int v111 = (ix + 1) * istep + (iy + 1) * jstep + (iz + 1) * kstep;

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

public:
  bool CollectiveAction(MPI_Comm c, bool s)
  {
    args *a = (args *)contents->get();

    VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(a->vk));
    ParticlesP p = Particles::Cast(KeyedDataObject::GetByKey(a->pk));

		p->clear();

    p->CopyPartitioning(v);

    float deltaX, deltaY, deltaZ;
    v->get_deltas(deltaX, deltaY, deltaZ);

    float ox, oy, oz;
    v->get_local_origin(ox, oy, oz);

    int nli, nlj, nlk;
    v->get_local_counts(nli, nlj, nlk);

    int local_count = nli*nlj*nlk;

    float max_x = ox + (nli-1) * deltaX,
          max_y = oy + (nlj-1) * deltaY,
          max_z = oz + (nlk-1) * deltaZ;

    int ngx, ngy, ngz;
    v->get_local_counts(ngx, ngy, ngz);
    int global_count = ngx*ngy*ngz;

    istep = 1;
    jstep = nli;
    kstep = nli * nlj;

    Particle tp;
    tp.xyz = get_starting_point(v);
    tp.u.value = sample(v, tp.xyz);

    float tq = Q(v, tp.u.value, a);

    Box *partition_box = v->get_local_box();

    int miss_count = 0;
    for (int iteration = 0; iteration < (a->n_start + a->n_iterations); )
    {
      double rv[3];
      normally_distributed_vector(rv);

      Particle cp(tp.xyz.x + rv[0], tp.xyz.y + rv[1], tp.xyz.z + rv[2], float(0.0));
      if (! partition_box->isIn(cp.xyz)) continue;

      iteration ++;

      cp.u.value = sample(v, cp);
      float cq = Q(v, cp.u.value, a);

      if ((cq > tq) || (RNDM() < (cq/tq)))
      {
        tp = cp;
        tq = cq;
        if ((iteration > a->n_start) && ((iteration % a->n_skip) == 0))
          p->push_back(tp);
        miss_count = 0;
      }
      else if (++miss_count > a->n_miss)
      {
        tp.xyz = get_starting_point(v);
        tp.u.value = sample(v, tp.xyz);
        miss_count = 0;
      }
    }

    std::cerr << "created " << p->GetNumberOfVertices() << " samples\n";
    return false;
  }
};

WORK_CLASS_TYPE(MHSampleMsg)

