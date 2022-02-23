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
#include <limits>
#include <unistd.h>
#include <sstream>
#include <time.h>
#include <pthread.h>

typedef std::numeric_limits< double > dbl;

#include "Application.h"
#include "Renderer.h"
#include "ClientServer.h"

#include "Receiver.hpp"

using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;
int maxLoopCount = 99999999;

// These are going to be needed in the master socket handler

ReceiverP receiver  = NULL;
RenderingSetP theRenderingSet = NULL;
RendererP theRenderer = NULL;
PartitioningP thePartitioning = NULL;
DatasetsP theDatasets = NULL;
vector<VisualizationP> theVisualizations;

pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  main_cond = PTHREAD_COND_INITIALIZER;

bool   done = false;
bool   cinema = false;
string cdb("");

float fuzz = 5.0;

void
syntax(char *a)
{
  cerr << "use Galaxy to write an image library for the given state file" << endl;
  cerr << "syntax: " << a << " [options] statefile" << endl;
  cerr << "options:" << endl;
  cerr << "  -f fuzz                      sort-of ghost zone width (0.0)" << endl;
  cerr << "  -n nsndrs                    number of external processes sending particle data (1)" << endl;
  cerr << "  -C cdb                       put output in Cinema DB (no)" << endl;
  cerr << "  -D[which]                    run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  cerr << "  -A                           wait for attachment" << endl;
  cerr << "  -s w h                       window width, height (1920 1080)" << endl;
  cerr << "  -N n                         max loop count (VERY large)" << endl;
  cerr << "  -P                           base port; master here, workers start here + 1 (1900)" << endl;
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

bool 
master_handler(ServerSkt *skt, void *p, char *buf)
{
  static int tstep = 0;

  Receiver *rcvr = (Receiver *)p;
  if (buf)
  {
    std::stringstream ss(buf);
    std::string cmd;
    ss >> cmd;

    if (cmd == "go")
    {
      long tstart = my_time();

      receiver->Accept(receiver->GetNSenders());
      receiver->Wait();

      long trcv = my_time();

      theDatasets->Commit();
      for (auto v : theVisualizations)
        v->Commit(theDatasets);
      
      theRenderingSet->Reset();
      theRenderingSet->Commit();

      long tcom = my_time();

#if 1
      GetTheApplication()->SyncApplication();

      theRenderer->Start(theRenderingSet);
      theRenderingSet->WaitForDone();

      long tdone = my_time();

      double time_receive = (trcv - tstart) / 1000000000.0;
      double time_commit = (tcom - trcv) / 1000000000.0;
      double time_render = (tdone - tcom) / 1000000000.0;

      cout << "TIMING total " << time_receive << " :: " << time_render << " :: " << time_commit << "\n";

      char namebuf[1024];

      if (cinema)
        sprintf(namebuf, "%s/image/image/image-%04d", cdb.c_str(), tstep);
      else
        sprintf(namebuf, "image-%04d", tstep);

      theRenderingSet->SaveImages(namebuf);
#endif

      tstep ++;

      free(buf);

      if (--maxLoopCount <= 0)
      {
        pthread_mutex_lock(&main_lock);
        done = true;
        pthread_cond_signal(&main_cond);
        pthread_mutex_unlock(&main_lock);
      }

      return false;
    }
    else if (cmd == "box")
    {
      float *box = (float *)skt->Receive();
      thePartitioning->SetBox(box[0]-fuzz, box[1]-fuzz, box[2]-fuzz, box[3]+fuzz, box[4]+fuzz, box[5]+fuzz);
      std::cerr.precision(dbl::max_digits10);
      std::cerr << (box[0]-fuzz) << " " << (box[1]-fuzz) << " " << (box[2]-fuzz) << " " << (box[3]+fuzz) << " " << (box[4]+fuzz) << " " << (box[5]+fuzz) << " " << "\n";
      free(box);
      thePartitioning->Commit();
      return false;
    }
    else if (cmd == "sendhosts")
    {
      skt->Send((char *)rcvr->gethosts());
      return false;
    }
    else if (cmd == "nsenders")
    {
      int nsenders;
      ss >> nsenders;
      rcvr->SetNSenders(nsenders);
      rcvr->Commit();
      return false;
    }
    else if (cmd == "q" || cmd == "quit")
    {
      pthread_mutex_lock(&main_lock);
      done = true;
      pthread_cond_signal(&main_cond);
      pthread_mutex_unlock(&main_lock);
      free(buf);
      return false;
    }
    else
    {
      std::cerr << "received unknown message from sender side: " << buf << "\n";
      free(buf);
      return true;
    }
  }

  std::cerr << "connection broke!\n";
  pthread_mutex_lock(&main_lock);
  done = true;
  pthread_cond_signal(&main_cond);
  pthread_mutex_unlock(&main_lock);
  free(buf);
  return true;
}

int main(int argc,  char *argv[])
{
  string statefile("");
  char *dbgarg;
  bool dbg = false;
  bool atch = false;
  int width = 1920, height = 1080;
  bool override_windowsize = false;
  int base_port = 1900;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-P")) base_port = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-f")) fuzz = atof(argv[++i]);
    else if (!strcmp(argv[i], "-C")) cinema = true, cdb = argv[++i];
    else if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-s")) 
    {
        width = atoi(argv[++i]);
        height = atoi(argv[++i]);
        override_windowsize = true;
    }
    else if (!strcmp(argv[i], "-N")) maxLoopCount = atoi(argv[++i]);
    else if (statefile == "")   statefile = argv[i];
    else syntax(argv[0]);
  }

  if (statefile == "")
    syntax(argv[0]);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  Renderer::Initialize();
  Receiver::Register();
  Partitioning::Register();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  theApplication.Run();

  long t_run_start = my_time();

  if (mpiRank == 0)
  {
    thePartitioning = Partitioning::NewP();

    theRenderer = Renderer::NewP();
    theRenderer->SetPartitioning(thePartitioning);
    theRenderer->Commit();

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

    theDatasets = Datasets::NewP();
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

    theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
    for (auto v : theVisualizations)
      if (! v->Commit(theDatasets))
      {
        std::cerr << "error committing a visualization\n";
        theApplication.QuitApplication();
        theApplication.Wait();
        exit(1);
      }

  theRenderingSet = RenderingSet::NewP();

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

    receiver  = Receiver::NewP();
    receiver->SetGeometry(particles);
    receiver->SetBasePort(base_port);
    receiver->SetFuzz(fuzz);
    receiver->Commit();

    receiver->Start(master_handler);
    receiver->Run();

    pthread_mutex_lock(&main_lock);
    while (! done)
      pthread_cond_wait(&main_cond, &main_lock);

    pthread_mutex_unlock(&main_lock);

    receiver->Stop();
    receiver = nullptr;

    thePartitioning = nullptr;
    theRenderingSet = nullptr;
    theDatasets = nullptr;
    theRenderer = nullptr;

    theApplication.DumpLog();

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
