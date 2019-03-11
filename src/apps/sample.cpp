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

#include <dtypes.h>
#include <Application.h>
#include "Renderer.h"

#include <ospray/ospray.h>

int mpiRank, mpiSize;

#include "Debug.h"

using namespace gxy;
using namespace std;

int samples_per_partition = 100;

#define WIDTH  1920
#define HEIGHT 1080

int width  = WIDTH;
int height = HEIGHT;

float radius = 0.02;

class SampleMsg : public Work
{
public:
  SampleMsg(VolumeP v, ParticlesP p) : SampleMsg(2*sizeof(Key))
  {
    ((Key *)contents->get())[0] = v->getkey();
    ((Key *)contents->get())[1] = p->getkey();
  }
  ~SampleMsg() {}

  WORK_CLASS(SampleMsg, true /* bcast */)

public:
  bool CollectiveAction(MPI_Comm c, bool s)
  {
    Key* keys = (Key *)contents->get();
    VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(keys[0]));
    ParticlesP p = Particles::Cast(KeyedDataObject::GetByKey(keys[1]));

    p->CopyPartitioning(v);

    float deltaX, deltaY, deltaZ;
    v->get_deltas(deltaX, deltaY, deltaZ);

    float ox, oy, oz;
    v->get_local_origin(ox, oy, oz);

    int nx, ny, nz;
    v->get_local_counts(nx, ny, nz);

    int xstep = 1;
    int ystep = nx;
    int zstep = nx * ny;

    float *fsamples = (float *)v->get_samples();

#if 0
    p->allocate(samples_per_partition);
    Particle *particle = p->get_samples();
#else
    Particle particle;
#endif

    for (int i = 0; i < samples_per_partition; i++)
    {
      float x, y, z;

      if (samples_per_partition == 1)
      {
        x = 0.5 * (nx - 1);
        y = 0.5 * (ny - 1);
        z = 0.5 * (nz - 1);
      }
      else
      {
        x = ((float)rand() / RAND_MAX) * (nx - 1);
        y = ((float)rand() / RAND_MAX) * (ny - 1);
        z = ((float)rand() / RAND_MAX) * (nz - 1);
      }

      int ix = (int)x;
      int iy = (int)y;
      int iz = (int)z;

      float dx = x - ix;
      float dy = y - iy;
      float dz = z - iz;

      int v000 = (ix + 0) * xstep + (iy + 0) * ystep + (iz + 0) * zstep;
      int v001 = (ix + 0) * xstep + (iy + 0) * ystep + (iz + 1) * zstep;
      int v010 = (ix + 0) * xstep + (iy + 1) * ystep + (iz + 0) * zstep;
      int v011 = (ix + 0) * xstep + (iy + 1) * ystep + (iz + 1) * zstep;
      int v100 = (ix + 1) * xstep + (iy + 0) * ystep + (iz + 0) * zstep;
      int v101 = (ix + 1) * xstep + (iy + 0) * ystep + (iz + 1) * zstep;
      int v110 = (ix + 1) * xstep + (iy + 1) * ystep + (iz + 0) * zstep;
      int v111 = (ix + 1) * xstep + (iy + 1) * ystep + (iz + 1) * zstep;

      float b000 = (1.0 - dx) * (1.0 - dy) * (1.0 - dz);
      float b001 = (1.0 - dx) * (1.0 - dy) * (dz);
      float b010 = (1.0 - dx) * (dy) * (1.0 - dz);
      float b011 = (1.0 - dx) * (dy) * (dz);
      float b100 = (dx) * (1.0 - dy) * (1.0 - dz);
      float b101 = (dx) * (1.0 - dy) * (dz);
      float b110 = (dx) * (dy) * (1.0 - dz);
      float b111 = (dx) * (dy) * (dz);

#if 0
      particle->u.value = fsamples[v000]*b000 +
                          fsamples[v001]*b001 +
                          fsamples[v010]*b010 +
                          fsamples[v011]*b011 +
                          fsamples[v100]*b100 +
                          fsamples[v101]*b101 +
                          fsamples[v110]*b110 +
                          fsamples[v111]*b111;

      particle->xyz.x = ox + x*deltaX;
      particle->xyz.y = oy + y*deltaY;
      particle->xyz.z = oz + z*deltaZ;

      particle ++;
#else
      particle.u.value = fsamples[v000]*b000 +
                          fsamples[v001]*b001 +
                          fsamples[v010]*b010 +
                          fsamples[v011]*b011 +
                          fsamples[v100]*b100 +
                          fsamples[v101]*b101 +
                          fsamples[v110]*b110 +
                          fsamples[v111]*b111;

      particle.xyz.x = ox + x*deltaX;
      particle.xyz.y = oy + y*deltaY;
      particle.xyz.z = oz + z*deltaZ;

      p->push_back(particle);
#endif
    }

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
  cerr << "  -n nsamples   number of samples in each partition (100)" << endl;
  cerr << "  -s x y        window size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -r radius     radius of samples (" << radius << ")" << endl;
  exit(1);
}


int
main(int argc, char * argv[])
{
  string data = "";
  char *dbgarg;
  bool dbg = false;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
      switch (argv[i][1])
      {
        case 'D': dbg = true, dbgarg = argv[i] + 2; break;
        case 'n': samples_per_partition = atoi(argv[++i]); break;
        case 'r': radius = atof(argv[++i]); break;
        case 's': width = atoi(argv[++i]); height = atoi(argv[++i]); break;
        default:
          syntax(argv[0]);
      }
    else if (data == "")   data = argv[i];
    else syntax(argv[0]);
  }

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

    // create empty distributed container for volume data
    VolumeP volume = Volume::NewP();
    // import data to all processes, smartly distributes volume across processses
    // this import defines the partitioning of the data across processses
    // if subsequent Import commands have a different partition, an error will be thrown
    volume->Import(data);

    // create empty distributed container for particles
    // particle partitioning will match volume partition
    ParticlesP samples = Particles::NewP();
    samples->SetRadius(radius);
    std::cerr << "radius is " << radius << "\n";

    // define action to perform on volume (see SampleMsg above)
    SampleMsg *smsg = new SampleMsg(volume, samples);
    smsg->Broadcast(true, true);

    samples->Commit();

    DatasetsP theDatasets = Datasets::NewP();
    theDatasets->Insert("samples", samples);
    theDatasets->Commit();

    vector<CameraP> theCameras;

#if 0
    for (int i = 0; i < 20; i++)
    {
      CameraP cam = Camera::NewP();

      cam->set_viewup(0.0, 1.0, 0.0);
      cam->set_angle_of_view(45.0);

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

    VisualizationP v = Visualization::NewP();
    v->AddVis(pvis);
    float light[] = {1.0, 2.0, 3.0}; int t = 1;
    v->get_the_lights()->SetLights(1, light, &t);
    v->get_the_lights()->SetK(0.4, 0.6);
    v->get_the_lights()->SetShadowFlag(false);
    v->get_the_lights()->SetAO(0, 0.0);
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

std::cerr << "RENDER\n";
		theRenderer->Render(theRenderingSet);
// #ifdef GXY_WRITE_IMAGES
std::cerr << "WAIT\n";
		theRenderingSet->WaitForDone();
std::cerr << "WAIT DONE\n";
    
// #endif 

    theRenderingSet->SaveImages(string("samples"));

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
