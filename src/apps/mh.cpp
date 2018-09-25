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
#include "Renderer.h"
#include "RenderingSet.h"
#include "MHSampleMsg.h"

#include <ospray/ospray.h>

#include <time.h>

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
float sigma = 1.0;
int   max_consecutive_misses = 10;

int   iteration_limit = 200000;
int   hot_start = 1000;
float scaling = 1.0;
int   skip = 10;
float data_min = -1, data_max = -1;
bool  data_minmax_from_argument = false;

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] data" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D            run debugger" << endl;
  cerr << "  -M m          max number of consecutive misses allowed (" << max_consecutive_misses << ")" << "\n";
  cerr << "  -i n          iteration limit (" << iteration_limit << ")\n";
  cerr << "  -h n          hot start count (" << hot_start << ")\n";
  cerr << "  -S step       scaling of candidate step (" << scaling << ")\n";
  cerr << "  -K skip       record every skip'th successful sample (" << skip << ")\n";
  cerr << "  -R p0 p1      values at which probability is 0 and 1, extended (data range min to max)\n";
  cerr << "  -s x y        window size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -r radius     radius of samples (" << radius << ")" << endl;
  cerr << "  -k sigma      standard deviation of normally distributed random step vector (" << sigma << ")" << endl;
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
    {
      switch (argv[i][1])
      {
        case 'D': dbg = true, dbgarg = argv[i] + 2; break;
        case 'M': max_consecutive_misses = atoi(argv[++i]); break;
        case 'i': iteration_limit = atoi(argv[++i]); break;
        case 'h': hot_start = atoi(argv[++i]); break;
        case 'S': scaling = atof(argv[++i]); break;
        case 'K': skip = atoi(argv[++i]); break;
        case 'R': data_min = atof(argv[++i]), data_max = atof(argv[++i]), data_minmax_from_argument = true; break;
        case 'r': radius = atof(argv[++i]); break;
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

  MHSampleMsg::Register();

  if (mpiRank == 0)
  {
    VolumeP volume = Volume::NewP();
    volume->Import(data);

    if (! data_minmax_from_argument)
      volume->get_global_minmax(data_min, data_max);

    ParticlesP samples = Particles::NewP();
    samples->SetRadius(radius);

    MHSampleMsg *smsg = new MHSampleMsg(volume, samples, data_min, data_max, scaling, iteration_limit, hot_start, skip, max_consecutive_misses);
    smsg->Broadcast(true, true);

    samples->Commit();

    float light[] = {1.0, 2.0, 3.0}; int t = 1;
    theRenderer->GetTheLighting()->SetLights(1, light, &t);
    theRenderer->GetTheLighting()->SetK(0.4, 0.6);
    theRenderer->GetTheLighting()->SetShadowFlag(false);
    theRenderer->GetTheLighting()->SetAO(0, 0.0);
    theRenderer->Commit();

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
    v->AddOsprayGeometryVis(pvis);
    v->AddVolumeVis(vvis);
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
