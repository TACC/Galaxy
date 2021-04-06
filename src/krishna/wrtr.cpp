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
  cerr << "  -f fuzz    sort-of ghost zone width (0.0)" << endl;
  cerr << "  -n nsndrs  number of external processes sending particle data (1)" << endl;
  cerr << "  -C cdb     put output in Cinema DB (no)" << endl;
  cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  cerr << "  -A         wait for attachment" << endl;
  cerr << "  -s w h     window width, height (1920 1080)" << endl;
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
  int maxConcurrentRenderings = 99999999;
  bool override_windowsize = false;
  int nsenders = 1;
  float fuzz = 0.0;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-f")) fuzz = atof(argv[++i]);
    else if (!strcmp(argv[i], "-n")) nsenders = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-C")) cinema = true, cdb = argv[++i];
    else if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-s")) 
    {
        width = atoi(argv[++i]);
        height = atoi(argv[++i]);
        override_windowsize = true;
    }
    else if (!strcmp(argv[i], "-N")) maxConcurrentRenderings = atoi(argv[++i]);
    else if (statefile == "")   statefile = argv[i];
    else syntax(argv[0]);
  }

  if (statefile == "")
    syntax(argv[0]);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  Renderer::Initialize();
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
      sleep(2);
      exit(1);
    }

    theRenderer->LoadStateFromDocument(*doc);
    theRenderer->Commit();

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

  RenderingSetP theRenderingSet = RenderingSet::NewP();

  int index = 0;
  for (auto c : theCameras)
    for (auto v : theVisualizations)
    {
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

      theRenderingSet->AddRendering(theRendering);
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
    receiver->SetFuzz(fuzz);
    receiver->Commit();

    receiver->Start();

    int tstep = 0;

    while (true)
    {
      std::cerr << "waiting for data...\n";
      receiver->Accept();
      receiver->Wait();
      std::cerr << "shuffle done\n";
    
      theRenderingSet->Reset();
      theRenderingSet->Commit();

      long t_rendering_start = my_time();
      cout << "render start" << endl;

      theRenderer->Start(theRenderingSet);
      theRenderingSet->WaitForDone();

      long t_done = my_time();
      cout << "TIMING total " << (t_done - t_rendering_start) / 1000000000.0 << " seconds" << endl;

      char namebuf[1024];

      if (cinema)
        sprintf(namebuf, "%s/image/image/image-%04d", cdb.c_str(), tstep);
      else
        sprintf(namebuf, "image-%04d", tstep);

      theRenderingSet->SaveImages(namebuf);

      theApplication.DumpLog();

      char cmd;
      cerr << "? ";
      do
      {
        cin >> cmd;
      } 
      while (cmd != 'q' && cmd != 's');

      if (cmd == 'q') 
        break;

      tstep ++;
    }

    receiver->Stop();
    receiver = nullptr;

    theDatasets = nullptr;

    theRenderer = nullptr;
    theApplication.QuitApplication();
  }

  theApplication.Wait();

  std::cerr << "done\n";
}
