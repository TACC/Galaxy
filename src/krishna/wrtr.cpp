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
#include <time.h>

#include "Application.h"
#include "Renderer.h"
#include "ClientServer.h"

#include "Receiver.hpp"


using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;

void
syntax(char *a)
{
  cerr << "use Galaxy to write an image library for the given state file" << endl;
  cerr << "syntax: " << a << " [options] statefile" << endl;
  cerr << "options:" << endl;
  cerr << "  -n nsndrs  number of external processes sending particle data (1)" << endl;
  cerr << "  -C cdb     put output in Cinema DB (no)" << endl;
  cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  cerr << "  -A         wait for attachment" << endl;
  cerr << "  -s w h     window width, height (1920 1080)" << endl;
  cerr << "  -S k       render only every k'th rendering" << endl;
  cerr << "  -N         max number of simultaneous renderings (VERY large)" << endl;
  exit(1);
}

long 
my_time()
{
  timespec s;
  clock_gettime(CLOCK_REALTIME, &s);
  return 1000000000*s.tv_sec + s.tv_nsec;
}

#include "Debug.h"

int main(int argc,  char *argv[])
{
  string statefile("");
  string  cdb("");
  char *dbgarg;
  bool dbg = false;
  bool atch = false;
  bool cinema = false;
  int width = 1920, height = 1080;
  int skip = 0;
  int maxConcurrentRenderings = 99999999;
  bool override_windowsize = false;
  int nsenders = 1;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-n")) nsenders = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-C")) cinema = true, cdb = argv[++i];
    else if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-s")) 
    {
        width = atoi(argv[++i]);
        height = atoi(argv[++i]);
        override_windowsize = true;
    }
    else if (!strcmp(argv[i], "-S")) skip = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-N")) maxConcurrentRenderings = atoi(argv[++i]);
    else if (statefile == "")   statefile = argv[i];
    else syntax(argv[0]);
  }

  if (statefile == "")
    syntax(argv[0]);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  Renderer::Initialize();
  Partitioning::Register();
  Receiver::Register();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  theApplication.Run();

  long t_run_start = my_time();

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

    theRenderer->LoadStateFromDocument(*doc);

    vector<CameraP> theCameras;
    if (! Camera::LoadCamerasFromJSON(*doc, theCameras))
    {
      std::cerr << "error loading cameras\n";
      theApplication.QuitApplication();
      theApplication.Wait();
      exit(1);
    }

    for (auto c : theCameras)
        if (! c->Commit())
        {
          std::cerr << "error committing camera\n";
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

    ParticlesP particles = Particles::Cast(theDatasets->Find("particles"));
    if (! particles)
    {
      std::cerr << "no particles object?\n";
      exit(1);
    }

    ReceiverP receiver  = Receiver::NewP();
    receiver->SetGeometry(particles);
    receiver->SetNSenders(nsenders);
    receiver->SetBasePort(1900);
    receiver->Commit();

    std::cerr << "waiting for data...\n";
    receiver->Start();
    receiver->Accept();
    receiver->Wait();
    receiver->Stop();
    receiver = nullptr;
    std::cerr << "got it\n";
   
    if (! theDatasets->Commit())
    {
      std::cerr << "error committing theDatasets\n";
      theApplication.QuitApplication();
      theApplication.Wait();
      exit(1);
    }

    vector<VisualizationP> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
    for (auto v : theVisualizations)
      if (! v->Commit(theDatasets))
      {
        std::cerr << "error committing a visualization\n";
        theApplication.QuitApplication();
        theApplication.Wait();
        exit(1);
      }

    vector<RenderingSetP> theRenderingSets;
    theRenderingSets.push_back(RenderingSet::NewP());

		int k = 0, index = 0;
    for (auto c : theCameras)
      for (auto v : theVisualizations)
      {
        if (skip && (k % skip) != 0)
        {
          std::cerr << "S";
          continue;
        }

        if (theRenderingSets.back()->GetNumberOfRenderings() >= maxConcurrentRenderings)
          theRenderingSets.push_back(RenderingSet::NewP());

        RenderingP theRendering = Rendering::NewP();

        theRendering->SetTheOwner(index++ % mpiSize );
        if (override_windowsize)
        {
            c->set_width(width);
            c->set_height(height);
        }
        theRendering->SetTheCamera(c);
        theRendering->SetTheDatasets(theDatasets);
        theRendering->SetTheVisualization(v);
        if (! theRendering->Commit())
        {
          std::cerr << "error committing theRendering\n";
          theApplication.QuitApplication();
          theApplication.Wait();
          exit(1);
        }

        theRenderingSets.back()->AddRendering(theRendering);

        k ++;
      }

    cout << "index = " << index << endl;

    theApplication.SyncApplication();

    long t_rendering_start = my_time();

    for (auto& rs : theRenderingSets)
    {
      long t0 = my_time();
      cout << "render start" << endl;

      if (! rs->Commit())
      {
        std::cerr << "error committing a RenderingSet\n";
        theApplication.QuitApplication();
        theApplication.Wait();
        exit(1);
      }

      theRenderer->Start(rs);

      rs->WaitForDone();
      rs->SaveImages(cinema ? (cdb + "/image/image").c_str() : "image");

      long t1 = my_time();
      cout << rs->GetNumberOfRenderings() << ": " << ((t1 - t0) / 1000000000.0) << " seconds" << endl;
    }

    long t_done = my_time();
    cout << "TIMING total " << (t_done - t_rendering_start) / 1000000000.0 << " seconds" << endl;


    theDatasets = nullptr;

    theRenderingSets.clear();
    theVisualizations.clear();
    theCameras.clear();

    theRenderer = nullptr;
    theApplication.QuitApplication();
  }

  theApplication.Wait();

  std::cerr << "done\n";
}
