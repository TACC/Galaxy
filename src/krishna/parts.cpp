// ========================================================================== //
//                                                                            //
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

#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#include "rapidjson/document.h"
using namespace rapidjson;

#include "Application.h"
#include "Partitioning.hpp"
#include "Datasets.h"
#include "Particles.h"
using namespace gxy;

static int mpiRank, mpiSize;
#include "Debug.h"

void
syntax(char *a)
{
  if (mpiRank == 0)
  {
    std::cerr << "syntax: " << a << " [ -l | -i ] [-D] pfile\n";
    std::cerr << "options:\n";
    std::cerr << "  -l        load pfile on host and distribute\n";
    std::cerr << "  -i        parallel import and gather\n";
    std::cerr << "  -D        usual debug stuff\n";
  }
  exit(1);
}

class ShowPartitioningMsg : public Work
{
public:
  ShowPartitioningMsg(PartitioningP p) : ShowPartitioningMsg(sizeof(Key))
  {
    *(Key *)get() = p->getkey();
  }

  ~ShowPartitioningMsg() {}

  WORK_CLASS(ShowPartitioningMsg, true)

public:
  bool Action(int s)
  {
    PartitioningP p = Partitioning::GetByKey(*(Key *)get());
    for (int i = 0; i < p->number_of_partitions(); i++)
    {
      Partitioning::Extent *e = p->get_extent(i);
      std::cerr << i << ": " << e->xmin << " " << e->xmax << " " << 
                                e->ymin << " " << e->ymax << " " << 
                                e->zmin << " " << e->zmax << "\n";
    }

    return false;
  }
};

WORK_CLASS_TYPE(ShowPartitioningMsg)

int
main(int argc, char **argv)
{
  bool dbg = false;
  char *dbgarg;
  bool load = true;

  string pfile = "";

  Application theApplication(&argc, &argv);
  theApplication.Start();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  for (int i = 1; i < argc; i++)
    if (! strcmp(argv[i], "-i")) load = false; 
    else if (! strcmp(argv[i], "-l")) load = true;
    else if (!strncmp(argv[i], "-D", 2)) dbg = true,  dbgarg = argv[i] + 2;
    else if (pfile == "") pfile = argv[i];
    else
      syntax(argv[0]);

  if (pfile == "")
    syntax(argv[0]);

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  theApplication.Run();

  KeyedDataObject::Register();
  Partitioning::Register();
  ShowPartitioningMsg::Register();

  if (mpiRank == 0)
  {
    PartitioningP partitioning = Partitioning::NewP();

    if (load)
    {
      if (! partitioning->Load(argv[1]))
      {
        std::cerr << "can't open part file\n";
        exit(1);
      }

      for (int i = 0; i < partitioning->number_of_partitions(); i++)
      {
        Partitioning::Extent *e = partitioning->get_extent(i);
        std::cerr << i << ": " << e->xmin << " " << e->xmax << " " <<
                                  e->ymin << " " << e->ymax << " " <<
                                  e->zmin << " " << e->zmax << "\n";
      }
    }
    else
    {
       ParticlesP particles = Particles::NewP();
       particles->Import(pfile);
       particles->Commit();
      
        if (! partitioning->Gather(particles))
        {
          std::cerr << "can't open part file\n";
          exit(1);
        }
    }

    while (true)
    {
      int n;
      std::cout << "? ";
      std::cin >> n;
      if (n < 0 || std::cin.eof())
        break;

      ShowPartitioningMsg *msg = new ShowPartitioningMsg(partitioning);
      msg->Send(n);
      delete msg;
    }
    
    theApplication.QuitApplication();
  }
  else
    theApplication.Wait();
}

