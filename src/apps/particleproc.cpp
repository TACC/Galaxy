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

// This application renders a set of particles. 
//
#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <cstdlib>
#include <math.h>

#include <dtypes.h>
#include <Application.h>
#include "Renderer.h"

int mpiRank, mpiSize;

#include "Debug.h"
#include "Box.h"

using namespace gxy;
using namespace std;

int samples_per_partition = 10;

#define WIDTH  1024
#define HEIGHT 768

int width  = WIDTH;
int height = HEIGHT;

float radius = 0.02;


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

class ParticleMsg : public Work
{
public:
  ParticleMsg(ParticlesDPtr p) : ParticleMsg(sizeof(Key))
  {
    ((Key *)contents->get())[0] = p->getkey();
  }
  ~ParticleMsg() {}
  WORK_CLASS(ParticleMsg,true)
public:
  bool CollectiveAction(MPI_Comm c, bool s)
   {
    Key* keys = (Key *)contents->get();
    ParticlesDPtr p = Particles::DCast(KeyedDataObject::GetByKey(*keys));
    // this method creates some particles and sets the value to the
    // local rank. The particles are randomly placed inside a
    // unit cube centered at the origin. 
    vec3f lo = {-.5,-.5,-.5};
    vec3f hi = {.5,.5,.5};
    p->set_local_box(lo,hi);
    p->set_global_box(lo,hi);
    int neighbors[6];
    neighbors[0] = -1;
    neighbors[1] = -1;
    neighbors[2] = -1;
    neighbors[3] = -1;
    neighbors[4] = -1;
    neighbors[5] = -1;
    p->set_neighbors(neighbors);
    Particle part;
    srand(mpiRank+1);
    for(int i=0;i<samples_per_partition; i++)
    {
        part.xyz.x = lo.x + ((float)rand()/RAND_MAX);
        part.xyz.y = lo.y + ((float)rand()/RAND_MAX);
        part.xyz.z = lo.z + ((float)rand()/RAND_MAX);
        part.u.value = mpiRank;
        p->push_back(part);
    }

    return false;
   }
};
WORK_CLASS_TYPE(ParticleMsg)
int
main(int argc, char * argv[])
{
  string data = "";
  char *dbgarg;
  bool dbg = false;
  bool override_windowsize = false;

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
        case 's': width = atoi(argv[++i]); 
                  height = atoi(argv[++i]); 
                  override_windowsize = true;
                  break;
        default:
          syntax(argv[0]);
      }
    else if (data == "")   data = argv[i];
    else syntax(argv[0]);
  }

  Renderer::Initialize();
  theApplication.Run();

  RendererDPtr theRenderer = Renderer::NewDistributed();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  ParticleMsg::Register();

  if (mpiRank == 0)
  {
    theRenderer->Commit();

    // create empty distributed container for particles
    // particle partitioning will match volume partition
    ParticlesDPtr samples = Particles::NewDistributed();

    // define action to perform on volume (see SampleMsg above)
    // not sampleing so comment this bit out
    ParticleMsg *pmsg = new ParticleMsg(samples);
    pmsg->Broadcast(true, true);
    //
    samples->Commit();

    DatasetsDPtr theDatasets = Datasets::NewDistributed();
    theDatasets->Insert("samples", samples);
    theDatasets->Commit();

    vector<CameraDPtr> theCameras;

#if 0
    for (int i = 0; i < 20; i++)
    {
      CameraDPtr cam = Camera::NewDistributed();

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
    CameraDPtr cam = Camera::NewDistributed();
    cam->set_viewup(0.0, 1.0, 0.0);
    cam->set_angle_of_view(45.0);
    cam->set_viewpoint(4.0, 0.0, 0.0);
    cam->set_viewdirection(-2.0, 0.0, 0.0);
    cam->Commit();
    theCameras.push_back(cam);
#endif

    ParticlesVisDPtr pvis = ParticlesVis::NewDistributed();
    pvis->SetName("samples");
    pvis->Commit(theDatasets);

    VisualizationDPtr v = Visualization::NewDistributed();
    v->AddVis(pvis);
    float light[] = {1.0, 2.0, 3.0}; int t = 1;
    v->get_the_lights()->SetLights(1, light, &t);
    v->get_the_lights()->SetK(0.4, 0.6);
    v->get_the_lights()->SetShadowFlag(false);
    v->get_the_lights()->SetAO(0, 0.0);
    v->Commit(theDatasets);

    RenderingSetDPtr theRenderingSet = RenderingSet::NewDistributed();

    int indx = 0;
    for (auto c : theCameras)
    {
      RenderingDPtr theRendering = Rendering::NewDistributed();
      theRendering->SetTheOwner((indx++) % mpiSize);
      if (override_windowsize)
      {
          c->set_width(width);
          c->set_height(height);
      }
      theRendering->SetTheCamera(c);
      theRendering->SetTheDatasets(theDatasets);
      theRendering->SetTheVisualization(v);
      theRendering->Commit();
      theRenderingSet->AddRendering(theRendering);
    }

    theRenderingSet->Commit();

std::cerr << "RENDER\n";
		theRenderer->Start(theRenderingSet);
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
