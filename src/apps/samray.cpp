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

#define SAMPLE 1

#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <cstdlib>
#include <math.h>

#include <dtypes.h>
#include <Application.h>
#include "Renderer.h"
#include "Sampler.h"

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

float radius = 0.001;

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

    Particle particle;

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

#ifdef SAMPLE 
void
execute_sampler(SamplerP sampler)
{
    sampler->Commit();

    ParticlesP samples = sampler->GetSamples();

    // create and add some samples, just to test
    Particle particle;

    particle.xyz.x = 0.0;
    particle.xyz.y = 0.0;
    particle.xyz.z = 0.0;

    samples->push_back(particle);

    samples->Commit();
}
#endif // SAMPLE

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

#ifdef SAMPLE
  Sampler::Initialize();
#endif // SAMPLE

  Renderer::Initialize();
  theApplication.Run();

#ifdef SAMPLE
  // order matters here
  SamplerP   theSampler  = Sampler::NewP();
#endif // SAMPLE

  RendererP theRenderer = Renderer::NewP();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  srand(mpiRank);

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  SampleMsg::Register();

  if (mpiRank == 0)
  {
    DatasetsP theDatasets = Datasets::NewP();

    // create empty distributed container for volume data
    VolumeP volume = Volume::NewP();

    // import data to all processes, smartly distributes volume across processses
    // this import defines the partitioning of the data across processses
    // if subsequent Import commands have a different partition, an error will be thrown
    volume->Import(data);
    volume->Commit();

    // add it to Datasets
    theDatasets->Insert("volume", volume);
    theDatasets->Commit();
    
    // Create a Visualization that specifies how the volume is to be sampled...
    // No need to futz with lights, we aren't lighting

    VisualizationP vis0 = Visualization::NewP();
  
    // Create a VolumeVis with an isovalue to sample the volume at 
    // that isolevel and add it to the sampling 'Visualization'

    VolumeVisP vvis = VolumeVis::NewP();
    vvis->SetName("volume");
    vvis->AddIsovalue(0.5);
    vvis->Commit(theDatasets);
    vis0->AddVis(vvis);

    vis0->Commit(theDatasets);
    


    // Create a rendering set for the sampling pass...

    // one rendering set
    RenderingSetP theRenderingSet0 = RenderingSet::NewP();

    // Create a Rendering combining the sampling 'visualization' and the camera..

    // multi-sample loop
    //   create camera
    //   create rendering
    //   add rendering to the rendering set
    //     everything gets the same visualization
    CameraP cam0;
    RenderingP theRendering0;
    float angle[2]   = {45.0, 10.0};
    float vPoint[2]  = {-4.0, 4.0};
    float vDir[2]    = {1.0, -1.0};
    int   sWidth[2]  = {width/4, width/4};
    int   sHeight[2] = {height/4, height/4};
    static int numCameras = 3;
    for (int i=0;i<numCameras;i++) 
    {
        cam0 = Camera::NewP();
        cam0->set_viewup(0.0, 1.0, 0.0);
        cam0->set_angle_of_view(angle[i]);
        cam0->set_viewpoint(vPoint[i], 0.0, 0.0);
        cam0->set_viewdirection(vDir[i], 0.0, 0.0);
        cam0->Commit();

        theRendering0 = Rendering::NewP();

        theRendering0->SetTheOwner(0);
        theRendering0->SetTheSize(sWidth[i], sHeight[i]);
        theRendering0->SetTheDatasets(theDatasets);
        theRendering0->SetTheCamera(cam0);
        theRendering0->SetTheVisualization(vis0);
        theRendering0->Commit();

        theRenderingSet0->AddRendering(theRendering0);
        theRenderingSet0->Commit();
    }

    // Creates a Particles dataset to sample into and attach it to the 
    // 'Sampler' renderer.   

    ParticlesP samrays = Particles::NewP();
    samrays->CopyPartitioning(volume);
    samrays->SetDefaultColor(0.5, 0.5, 0.5, 1.0);
    theSampler->SetSamples(samrays);

    // Commit the Sampler, initiate sampling, and wait for it to be done
    
    theSampler->Commit();
    theSampler->Sample(theRenderingSet0);
    theRenderingSet0->WaitForDone();

    // Now the 'samrays' particle set contains the samples.  Commit it and
    // add it to the known dataset

    samrays->Commit();
    theDatasets->Insert("samrays", samrays);
    theDatasets->Commit();

    // Now we set up a Visualization to visualize the samples.  
    // This time we'll be lighting...

    VisualizationP vis1 = Visualization::NewP();

    float light[] = {1.0, 2.0, 3.0}; int t = 1;
    Lighting *l = vis1->get_the_lights();
    l->SetLights(1, light, &t);
    l->SetK(0.8, 0.2);
    l->SetShadowFlag(false);
    l->SetAO(0, 0.0);
  
    // A ParticlesVis to render the samples

    ParticlesVisP pvis = ParticlesVis::NewP();
    pvis->SetName("samrays");
    pvis->Commit(theDatasets);
    pvis->SetRadius(radius);
    vis1->AddVis(pvis);

    vis1->Commit(theDatasets);

    // Now we set up a RenderingSet for the visualization of the particles.
    // We'll use two cameras - one the same as in the first pass, and one
    // off-angle

    CameraP cam1 = Camera::NewP();
    cam1->set_viewup(0.0, 1.0, 0.0);
    cam1->set_angle_of_view(45.0);
    cam1->set_viewpoint(1.0, 1.0, 1.0);
    cam1->set_viewdirection(-1.0, -1.0, -1.0);
    cam1->Commit();

    RenderingSetP theRenderingSet1 = RenderingSet::NewP();

    // RenderingP theRendering1 = Rendering::NewP();
    // theRendering1->SetTheOwner(0);
    // theRendering1->SetTheSize(width, height);
    // theRendering1->SetTheDatasets(theDatasets);
    // theRendering1->SetTheCamera(cam0);          // the original camera
    // theRendering1->SetTheVisualization(vis1);   // the Particles-based vis
    // theRendering1->Commit();

    // theRenderingSet1->AddRendering(theRendering1);

    RenderingP theRendering2 = Rendering::NewP();
    theRendering2->SetTheOwner(0);
    theRendering2->SetTheSize(width, height);
    theRendering2->SetTheDatasets(theDatasets);
    theRendering2->SetTheCamera(cam1);          
    theRendering2->SetTheVisualization(vis1);
    theRendering2->Commit();

    theRenderingSet1->AddRendering(theRendering2);
    theRenderingSet1->Commit();

    // Render, wait, and write results

    theRenderer->Render(theRenderingSet1);
    theRenderingSet1->WaitForDone();
    theRenderingSet1->SaveImages(string("samples"));

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
