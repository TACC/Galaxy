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

#include <string>
#include <unistd.h>
#include <sstream>
#include <time.h>

#include "Application.h"
#include "KeyedDataObject.h"
#include "DataObjects.h"
#include "Datasets.h"
#include "PathLines.h"
#include "Camera.h"
#include "PathLinesVis.h"
#include "Volume.h"
#include "Rendering.h"
#include "RenderingSet.h"
#include "Renderer.h"

#include "RungeKutta.h"
#include "TraceToPathLines.h"

using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;

void
syntax(char *a)
{
  cerr << "test\n";
  cerr << "syntax: " << a << " [options] statefile" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  exit(1);
}

#include "Debug.h"

int main(int argc,  char *argv[])
{
  string statefile("tester.state");
  char *dbgarg;
  bool dbg = false;
  bool atch = false;
  int width = 512, height = 512;
  int nsteps = 100;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  for (int i = 1; i < argc; i++)
  {
    if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i],"-n")) nsteps = atoi(argv[++i]);
    else if (!strcmp(argv[i],"-s")) width = atoi(argv[++i]), height = atoi(argv[++i]);
    else if (statefile == "")   statefile = argv[i];
    else syntax(argv[0]);
  }

  if (statefile == "")
    syntax(argv[0]);

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  Renderer::Initialize();
  RegisterDataObjects();
  RungeKutta::RegisterRK();
  RegisterTraceToPathLines();
 
  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  theApplication.Run();

  if (mpiRank == 0)
  {
    RendererP theRenderer = Renderer::NewP();

    rapidjson::Document *doc = GetTheApplication()->OpenJSONFile(statefile);
    if (! doc)
    {
      std::cerr << "Bad state file: " << statefile << "\n";
      theApplication.QuitApplication();
      theApplication.Wait();
      exit(1);
    }

    DatasetsP theDatasets = Datasets::NewP();
    if (! theDatasets->LoadFromJSON(*doc))
    {
      std::cerr << "error loading theDatasets\n";
      theApplication.QuitApplication();
      theApplication.Wait();
      exit(1);
    }
    
    if (! theDatasets->Commit())
    {
      std::cerr << "error committing theDatasets\n";
      theApplication.QuitApplication();
      theApplication.Wait();
      exit(1);
    }

    RungeKuttaP rkp = RungeKutta::NewP();
    rkp->set_max_steps(nsteps);
    if (! rkp->SetVectorField(Volume::Cast(theDatasets->Find("vectors"))))
      exit(1);
    rkp->Commit();

    sleep(1);

#if 0
    bool done = false;
    int id = 0;
    while (! done)
    {
      vec3f p;
      cin >> p.x >> p.y >> p.z;

      done = cin.eof();
      if (!done)
      {
        std::cerr << "me,X,Y,Z,VX,VY,VZ,UX,UY,UZ,t,twist\n";
        rkp->Trace(id++, p);
      }
    }
#elif 0
    // This will wait for each

    for (int i = 0; i < 10; i++)
    {
      float d = -0.5 + (i / 9.0);
      vec3f p(d, d, -0.9);
      rkp->Trace(p, i);
    }
#else
    // this will wait for them all
    vec3f pts[10];

    for (int i = 0; i < 10; i++)
    {
      float d = -0.5 + (i / 9.0);
      pts[i] = vec3f(d, d, -0.9);
    }

    rkp->Trace(10, pts);
#endif

  sleep(1);

  PathLinesP plp = PathLines::NewP();

  TraceToPathLines(rkp, plp);
  plp->Commit();

  theDatasets->Insert("pathlines", plp);
    theDatasets->Commit();

    CameraP camera = Camera::NewP();
    camera->set_viewup(0.0, 1.0, 0.0);
    camera->set_angle_of_view(45.0);
    camera->set_viewpoint(1.0, 2.0, 3.0);
    camera->set_viewdirection(-1.0, -2.0, -3.0);
    camera->Commit();

    vec4f cmap[2];
    cmap[0] = {0.0, 1.0, 0.0, 0.0};
    cmap[1] = {10.5984, 1.0, 1.0, 1.0};

    vec2f omap[2];
    omap[0] = {0.0,  0.1};
    omap[1] = {10.5984, 0.2};

    PathLinesVisP plvis = PathLinesVis::NewP();
    plvis->SetName("pathlines");
    plvis->SetRadiusTransform(0.0, 0.01, 10.5984, 0.02);
    plvis->SetColorMap(2, cmap);
    plvis->SetOpacityMap(2, omap);
    plvis->Commit(theDatasets);

    VisualizationP visualization = Visualization::NewP();
    visualization->AddVis(plvis);
    visualization->Commit(theDatasets);

    RenderingP theRendering = Rendering::NewP();
    theRendering->SetTheOwner(0);
    theRendering->SetTheSize(width, height);
    theRendering->SetTheDatasets(theDatasets);
    theRendering->SetTheCamera(camera);
    theRendering->SetTheVisualization(visualization);
    theRendering->Commit();

    RenderingSetP renderingSet = RenderingSet::NewP();
    renderingSet->AddRendering(theRendering);
    renderingSet->Commit();

    theRenderer->Start(renderingSet);
    renderingSet->WaitForDone();
    renderingSet->SaveImages(string("plines"));

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
