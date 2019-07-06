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

#include "gxy/Application.h"
#include "gxy/KeyedDataObject.h"
#include "gxy/DataObjects.h"
#include "gxy/Datasets.h"
#include "gxy/Volume.h"

#include "RungeKutta.h"

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

class TestInterpolateMsg : public Work
{
public:
  TestInterpolateMsg(Key k, vec3f p) : TestInterpolateMsg(sizeof(Key) + sizeof(p)) 
  {
    unsigned char *g = (unsigned char *)get();
    *(Key *)g = k;
    g += sizeof(Key);
    memcpy(g, &p, sizeof(p));
  }
  ~TestInterpolateMsg() {}
  WORK_CLASS(TestInterpolateMsg, true)

public:
  bool Action(int s)
  {
    std::cerr << "ACTION\n";
    unsigned char *g = (unsigned char *)get();
    VolumeP v = Volume::GetByKey(*(Key *)g);
    g += sizeof(Key);
    vec3f p;
    memcpy(&p, g, sizeof(p));
    float r[3];

    int me = GetTheApplication()->GetRank();
    cerr << "(" << me << ") ";

    int whose = v->PointOwner(p);
    if (whose == me)
      std::cerr << "mine ";
    else
      std::cerr << "NOT mine ";

    if (v->Sample(p, r))
    {
      if (whose == me)
        std::cerr << r[0] << " " << r[1] << " " << r[2] << "\n";
      else
        std::cerr << " ghost " << r[0] << " " << r[1] << " " << r[2] << " ";
    }

    if (whose != me && whose != -1)
    {
      std::cerr << "sending to " << whose << "\n";
      TestInterpolateMsg msg(v->getkey(), p);
      msg.Send(whose);
    }
    else if (whose == -1)
      std::cerr << "unowned\n";

    return false;
  }
};

WORK_CLASS_TYPE(TestInterpolateMsg)

#include "gxy/Debug.h"

int main(int argc,  char *argv[])
{
  string statefile("tester.state");
  char *dbgarg;
  bool dbg = false;
  bool atch = false;
  int nsteps = 100;

  for (int i = 1; i < argc; i++)
  {
    if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i],"-n")) nsteps = atoi(argv[++i]);
    else if (statefile == "")   statefile = argv[i];
    else syntax(argv[0]);
  }

  if (statefile == "")
    syntax(argv[0]);

  Application theApplication(&argc, &argv);

  RegisterDataObjects();
  RungeKutta::RegisterRK();
  TestInterpolateMsg::Register();

  theApplication.Start();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  theApplication.Run();

  if (mpiRank == 0)
  {
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

    VolumeP vp = Volume::Cast(theDatasets->Find("vectors"));

    RungeKuttaP rkp = RungeKutta::NewP();
    rkp->set_max_steps(nsteps);
    rkp->Commit();

    bool done = false;
    while (! done)
    {
      vec3f p;
      cin >> p.x >> p.y >> p.z;

      done = cin.eof();
      if (!done)
      {
        std::cerr << "me,X,Y,Z,VX,VY,VZ,t\n";
#if 0
        TestInterpolateMsg msg(theDatasets->FindKey("vectors"), p);
        msg.Send(0);
#else
        rkp->Trace(0, p, vp);
#endif

      }
    }

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
