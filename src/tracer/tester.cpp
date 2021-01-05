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

#include <string>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <vector>
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
  cerr << "syntax: " << a << " [options] statefile" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D[which]     run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  cerr << "  -m n          max number of steps (2000)" << endl;
  cerr << "  -h h          portion of cell size to step (0.2)" << endl;
  cerr << "  -z z          termination magnitude of vectors (1e-12)" << endl;
  cerr << "  -s w h        image size (512x512)" << endl;
  cerr << "  -S seeds.csv  seeds (csv with header line) (none)" << endl;
  cerr << "  -P x y z      single seed to trace (0.0, 0.0, 0.0)" << endl;
  cerr << "  -T            dump local trajectories to stderr (no)" << endl;
  cerr << "  -z z          termination magnitude of vectors (1e-12)" << endl;
  cerr << "  -t t          max integration time (none)" << endl;
  cerr << "  -I max        scale the colormap to this to avoid hairballs (scale to max integration time)\n";
  exit(1);
}

#include "Debug.h"

int main(int argc,  char *argv[])
{
  string seedfile("");
  string statefile("tester.state");
  char *dbgarg;
  bool dbg = false;
  bool atch = false;
  int width = 512, height = 512;
  int nsteps = 2000;
  float h = 0.2;
  float z = 1e-12;
  float t = -1;
  float X = 0.0, Y = 0.0, Z = 0.0;
  bool  dump_trajectories = false;
  float max_i = -1;
  bool override_windowsize = false;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  for (int i = 1; i < argc; i++)
  {
    if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i],"-m")) nsteps = atoi(argv[++i]);
    else if (!strcmp(argv[i],"-h")) h = atof(argv[++i]); 
    else if (!strcmp(argv[i],"-z")) z = atof(argv[++i]); 
    else if (!strcmp(argv[i],"-s")) 
    {
        width  = atoi(argv[++i]);
        height = atoi(argv[++i]);
        override_windowsize = true;
    }
    else if (!strcmp(argv[i],"-S")) seedfile = argv[++i];
    else if (!strcmp(argv[i],"-P")) X = atof(argv[++i]), Y = atof(argv[++i]), Z = atof(argv[++i]);
    else if (!strcmp(argv[i],"-T")) dump_trajectories = true;
    else if (!strcmp(argv[i],"-I")) max_i = atof(argv[++i]); 
    else if (!strcmp(argv[i],"-z")) z = atof(argv[++i]);
    else if (!strcmp(argv[i],"-t")) t = atof(argv[++i]);
    else if (!strcmp(argv[i],"--")) syntax(argv[0]);
    else statefile = argv[i];
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
    RendererDPtr theRenderer = Renderer::NewDistributed();

    rapidjson::Document *doc = GetTheApplication()->OpenJSONFile(statefile);
    if (! doc)
    {
      std::cerr << "Bad state file: " << statefile << "\n";
      theApplication.QuitApplication();
      theApplication.Wait();
      exit(1);
    }

    DatasetsDPtr theDatasets = Datasets::NewDistributed();
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

    RungeKuttaDPtr rkp = RungeKutta::NewDistributed();
    rkp->set_max_steps(nsteps);
    rkp->set_stepsize(h);
    rkp->SetMinVelocity(z);
    rkp->SetMaxIntegrationTime(t);

    if (! rkp->SetVectorField(Volume::Cast(theDatasets->Find("vectors"))))
      exit(1);
    rkp->Commit();

    if (seedfile.size() > 0)
    {
      ifstream sf(seedfile);
      if (sf.fail())
      {
        std::cerr << "error unable to open seed file: " << seedfile << "\n";
        exit(1);
      }

      char line[1024];
      sf.getline(line, 1024);

      vector<vec3f> seeds;
      while (1 == 1)
      {
        sf.getline(line, 1024);
        if (sf.eof())
          break;

        if (line[0] != '#')
        {
          vec3f seed;
          if (sscanf(line, "%f,%f,%f", &seed.x, &seed.y, &seed.z) != 3)
          {
            std::cerr << "error unable to read seed file: " << seedfile << "\n";
            exit(1);
          }
          seeds.push_back(seed);
        }
      }

      rkp->Trace(seeds.size(), seeds.data());
    }
    else
    {
      vec3f pt(X, Y, Z);
      rkp->Trace(1, &pt);
    }

    std::cerr << "max integration time: " << rkp->get_maximum_integration_time() << "\n";

    if (max_i == -1) max_i = rkp->get_maximum_integration_time();

    if (dump_trajectories)
    {
      int nt = rkp->get_number_of_local_trajectories();
      std::cerr << "NT " << nt << "\n";
      for (auto i = 0; i < nt; i++)
      {
        trajectory t = rkp->get_trajectory(i);
        cout << "X,Y,Z,T\n";
        for (auto s : (*t))
          for (auto j = 0; j < s->points.size(); j++)
          {
            vec3f xyz = s->points[j];
            cout << xyz.x << "," << xyz.y << "," << xyz.z << "," << s->times[j] << "\n";
          }
        }
    }

    PathLinesDPtr plp = PathLines::NewDistributed();

    TraceToPathLines(rkp, plp, 1e10, 1e10);
    plp->Commit();

    theDatasets->Insert("pathlines", plp);
    theDatasets->Commit();

    vector<CameraDPtr> theCameras;
    Camera::LoadCamerasFromJSON(*doc, theCameras);
    for (auto c : theCameras)
      c->Commit();

    vector<VisualizationDPtr> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
    for (auto v : theVisualizations)
    {
      for (auto i = 0; i < v->GetNumberOfVis(); i++)
      {
        MappedVisDPtr mvp = MappedVis::Cast(v->GetVis(i));
        if (mvp)
          mvp->ScaleMaps(0.0, max_i);
      }
      v->Commit(theDatasets);
    }

    RenderingSetDPtr theRenderingSet = RenderingSet::NewDistributed();
    for (auto v : theVisualizations)
      for (auto c : theCameras)
      {
        RenderingDPtr r = Rendering::NewDistributed();
        r = Rendering::NewDistributed();
        r->SetTheOwner(0);
        if (override_windowsize)
        {
            c->set_width(width);
            c->set_height(height);
        }
        r->SetTheDatasets(theDatasets);
        r->SetTheCamera(c);
        r->SetTheVisualization(v);
        r->Commit();

        theRenderingSet->AddRendering(r);
      }

    theRenderingSet->Commit();

    theRenderer->Start(theRenderingSet);
    theRenderingSet->WaitForDone();
    theRenderingSet->SaveImages(string("plines"));

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
