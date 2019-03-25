// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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
#include <dlfcn.h>

#include <dtypes.h>
#include <Application.h>
#include "Renderer.h"
#include "RenderingSet.h"

#include <ospray/ospray.h>

#include <time.h>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>

using namespace boost;

int mpiRank, mpiSize;

#include "Debug.h"

using namespace gxy;
using namespace std;

int total_samples = 100;

#define WIDTH   1920
#define HEIGHT 1080

int width  = WIDTH;
int height = HEIGHT;

float radius = 0.02;
float target = 0.0;
float power = 10.0;
float sigma = 1.0;
int   max_consecutive_misses = 10;
int   iteration_limit = 20000;
int   hot_start = 1000;
float scaling = 1.0;
int   skip = 10;

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

class SampleMsg : public Work
{
    struct args
    {
        Key   vk;
        Key   pk;
        int   tf_type;
        float tf0;
        float tf1;
        float step;
        int   n_iterations;
        int   n_start;
        int   n_skip;
        int   n_miss;
    };

public:
  SampleMsg(VolumeP v, ParticlesP p, int tf_type, float tf0, float tf1, float step, int niterations, int nstart, int nskip, int nmiss)
            : SampleMsg(sizeof(args))
  {
        args *a = (args *)contents->get();
        a->vk = v->getkey();
        a->pk = p->getkey();
        a->tf_type = tf_type;
        a->tf0 = tf0;
        a->tf1 = tf1;
        a->step = step;
        a->n_iterations = niterations;
        a->n_start = nstart;
        a->n_skip = nskip;
        a->n_miss = nmiss;
    }

  ~SampleMsg() {}

  WORK_CLASS(SampleMsg, true)

private: 
  float istep, jstep, kstep;

    vec3f get_starting_point(VolumeP v)
    {
        Box *box = v->get_local_box();
        return vec3f(box->xyz_min.x + RNDM*(box->xyz_max.x - box->xyz_min.x),
                     box->xyz_min.y + RNDM*(box->xyz_max.y - box->xyz_min.y),
                     box->xyz_min.z + RNDM*(box->xyz_max.z - box->xyz_min.z));
    }

  float Q(VolumeP v, float s, args *a)
  {
    float q;

    if (a->tf_type == TF_LINEAR)
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

  float sample(VolumeP v, Particle& p) { return sample(v, p.xyz.x, p.xyz.y, p.xyz.z); }
  float sample(VolumeP v, vec3f xyz) { return sample(v, xyz.x, xyz.y, xyz.z); }
  float sample(VolumeP v, float x, float y, float z)
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

    p->CopyPartitioning(v);

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

    istep = 1;
    jstep = gnli;
    kstep = gnli * gnlj;

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

      if ((cq > tq) || (RNDM < (cq/tq)))
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

    std::cerr << "created " << p->get_n_samples() << " samples\n";
    return false;
  }
};

WORK_CLASS_TYPE(SampleMsg)

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] data" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D            run debugger" << endl;
  cerr << "  -M m          max number of consecutive misses allowed (" << max_consecutive_misses << ")" << "\n";
  cerr << "  -i n          iteration limit (20000)\n";
  cerr << "  -h n          hot start count (1000)\n";
  cerr << "  -S step       scaling of candidate step (1)\n";
  cerr << "  -K skip       record every skip'th successful sample (10)\n";
  cerr << "  -R p0 p1      linear transfer function, from p0 to p1 extended (default, linear data range)\n";
  cerr << "  -G p0 p1      gaussian transfer function p0 is median, p2 is stdev (default, linear data range)\n";
  cerr << "  -s x y        window size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -t target     value of highest interest (" << target << ")" << endl;
  cerr << "  -p power      exponent of interest (" << power << ")" << endl;
  cerr << "  -r radius     radius of samples (" << radius << ")" << endl;
  cerr << "  -k sigma      standard deviation of normally distributed random step vector (" << sigma << ")" << endl;
  exit(1);
}

#if 0
namespace gxy
{
extern void DDSpheres_Hello();
};
#endif

int
main(int argc, char * argv[])
{
  string data = "";
  char *dbgarg;
  bool dbg = false;

#if 0
  dbg = true;
  std::cerr << getpid() << "\n";
  while (dbg)
    sleep(1);
#endif

  ospInit(&argc, (const char **)argv);

#if 0
  DDSpheres_Hello();

  {
    void *foo = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    void *s = dlsym(foo, "ospray_init_module_ispc");
    if (s) std::cerr << "ospray_init_module_ispc OK\n"; else std::cerr << "ospray_init_module_ispc NOT FOUND\n";

    s = dlsym(foo, "ospray_create_geometry__dspheres");
    if (s) std::cerr << "ospray_create_geometry__dspheres OK\n"; else std::cerr << "ospray_create_geometry__dspheres NOT FOUND\n";
  }
#endif

  Application theApplication(&argc, &argv);
  theApplication.Start();

  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
    {
      switch (argv[i][1])
      {
        case 'D': dbg = true, dbgarg = argv[i] + 2; break;
        case 'M': max_consecutive_misses = atoi(argv[++i]); break;
        case 'i': iteration_limit = atoi(argv[++i]); break;
        case 'h': hot_start = atoi(argv[++i]); break;
        case 'S': scaling = atof(argv[++i]); break;
        case 'K': skip = atoi(argv[++i]); break;
        case 'R': tf0 = atof(argv[++i]), tf1 = atof(argv[++i]), tf_type = TF_LINEAR; break;
        case 'G': tf0 = atof(argv[++i]), tf1 = atof(argv[++i]), tf_type = TF_GAUSSIAN; break;
        case 'r': radius = atof(argv[++i]); break;
        case 't': target = atof(argv[++i]); break;
        case 'p': power  = atof(argv[++i]); break;
        case 'k': sigma = atof(argv[++i]); break;
        case 's': width  = atoi(argv[++i]); height = atoi(argv[++i]); break;
        default:
          syntax(argv[0]);
      }
    }
    else if (data == "") data = argv[i];
    else syntax(argv[0]);
  }

  generator = new variate_generator<mt19937, normal_distribution<> >(mt19937(time(0)), normal_distribution<>(0.0, sigma));

  Renderer::Initialize();
  theApplication.Run();

  RendererP theRenderer = Renderer::NewP();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  srand(mpiRank);

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  SampleMsg::Register();

  if (mpiRank == 0)
  {
    theRenderer->Commit();

    VolumeP volume = Volume::NewP();
    volume->Import(data);

    if (tf_type == TF_NONE)
      volume->get_global_minmax(tf0, tf1), tf_type = TF_LINEAR;

    ParticlesP samples = Particles::NewP();
    samples->SetRadius(radius);

    SampleMsg *smsg = new SampleMsg(volume, samples, tf_type, tf0, tf1, scaling, iteration_limit, hot_start, skip, max_consecutive_misses);
    smsg->Broadcast(true, true);

    samples->Commit();

    DatasetsP theDatasets = Datasets::NewP();
    theDatasets->Insert("samples", samples);
    theDatasets->Insert("volume", volume);
    theDatasets->Commit();

    vector<CameraP> theCameras;

#if 1
    for (int i = 0; i < 20; i++)
    {
      CameraP cam = Camera::NewP();

      cam->set_viewup(0.0, 1.0, 0.0);
      cam->set_angle_of_view(30.0);

      float angle = 2*3.1415926*(i / 20.0);

      float vpx = 8.0 * cos(angle);
      float vpy = 8.0 * sin(angle);

      cam->set_viewpoint(vpx, vpy, 0.0);
      cam->set_viewdirection(-vpx, -vpy, 0.0);

      cam->Commit();
      theCameras.push_back(cam);
    }
#else
    CameraP cam = Camera::NewP();
    cam->set_viewup(0.0, 1.0, 0.0);
    cam->set_angle_of_view(45.0);
    cam->set_viewpoint(4.0, 0.0, 0.0);
    cam->set_viewdirection(-2.0, 0.0, 0.0);
    cam->Commit();
    theCameras.push_back(cam);
#endif

    ParticlesVisP pvis = ParticlesVis::NewP();
    pvis->SetName("samples");
    pvis->Commit(theDatasets);

    VolumeVisP vvis = VolumeVis::NewP();
    vvis->SetName("volume");

    vec4f cmap[] = {
        {0.00,1.0,0.5,0.5},
        {0.25,0.5,1.0,0.5},
        {0.50,0.5,0.5,1.0},
        {0.75,1.0,1.0,0.5},
        {1.00,1.0,0.5,1.0}
    };

    vec2f omap[] = {
                { 0.00, 0.00},
                { 1.00, 0.10},
                { 8.00, 0.10}
               };

    vvis->SetColorMap(5, cmap);
    vvis->SetOpacityMap(3, omap);
    vvis->SetVolumeRendering(true);
    vvis->Commit(theDatasets);

    VisualizationP v = Visualization::NewP();
    float light[] = {1.0, 2.0, 3.0}; int t = 1;
    v->get_the_lights()->SetLights(1, light, &t);
    v->get_the_lights()->SetK(0.4, 0.6);
    v->get_the_lights()->SetShadowFlag(false);
    v->get_the_lights()->SetAO(0, 0.0);
    v->AddVis(pvis);
    v->AddVis(vvis);
    v->Commit(theDatasets);

    RenderingSetP theRenderingSet = RenderingSet::NewP();

    int indx = 0;
    for (auto c : theCameras)
    {
      RenderingP theRendering = Rendering::NewP();
      theRendering->SetTheOwner((indx++) % mpiSize);
      theRendering->SetTheSize(width, height);
      theRendering->SetTheCamera(c);
      theRendering->SetTheDatasets(theDatasets);
      theRendering->SetTheVisualization(v);
      theRendering->Commit();
      theRenderingSet->AddRendering(theRendering);
    }

    theRenderingSet->Commit();

    theRenderer->Render(theRenderingSet);

#ifdef GXY_WRITE_IMAGES
    theRenderingSet->WaitForDone();
#endif
    
    theRenderingSet->SaveImages(string("samples"));

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
