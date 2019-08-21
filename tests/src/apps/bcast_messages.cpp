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

/* 
Demonstration/Test of different broadcast message modes.   Collective messages
take place in the message loop and have access to the MPI communicator, while 
non-collective messages are queued for asynchronous execution.  Blocking messages'
Broadcast call will not complete until the Action/CollectiveAction method completes
on the root process.   Note that a true blocking message can be implemented by
adding a MPI_Barrier call to a CollectiveAction method of a Collective message, in 
in which case the local CollectiveAction will wait for the corresponding methods
to run on the other processes.
*/

#include <iostream>

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>

#include <Application.h>

using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;
#include "Debug.h"

class BcastMsg : public Work
{
public:
	WORK_CLASS(BcastMsg, true)

public:
	bool CollectiveAction(MPI_Comm c, bool is_root)
	{
    int r = GetTheApplication()->GetRank();
    if (r > 0)
    {
      sleep(2+r);
      std::cerr << "CollectiveAction: " << GetTheApplication()->GetRank() << "\n";
    }
    else
    {
      std::cerr << "CollectiveAction: " << GetTheApplication()->GetRank() << "\n";
      sleep(1);
    }
		return false;
	}

	bool Action(int s)
	{
    int r = GetTheApplication()->GetRank();
    if (r > 0)
    {
      sleep(2+r);
      std::cerr << "Action: " << GetTheApplication()->GetRank() << "\n";
    }
    else
    {
      std::cerr << "Action: " << GetTheApplication()->GetRank() << "\n";
      sleep(1);
    }
		return false;
	}
};

class SyncBcastMsg : public Work
{
public:
	WORK_CLASS(SyncBcastMsg, true)

public:
	bool CollectiveAction(MPI_Comm c, bool is_root)
	{
    int r = GetTheApplication()->GetRank();
    sleep(2+r);
    std::cerr << "CollectiveAction: " << GetTheApplication()->GetRank() << "\n";
    MPI_Barrier(c);
		return false;
	}
};

WORK_CLASS_TYPE(BcastMsg)
WORK_CLASS_TYPE(SyncBcastMsg)

void
syntax(char *a)
{
  if (mpiRank == 0)
  {
    std::cerr << "syntax: " << a << " [options] " << endl;
    std::cerr << "options:" << endl;
    std::cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  }
  exit(1);
}

int
main(int argc, char * argv[])
{
  char *dbgarg;
  bool dbg = false;
  bool atch = false;

	Application theApplication(&argc, &argv);
	theApplication.Start();

  for (int i = 1; i < argc; i++)
    if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else syntax(argv[0]);

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

	theApplication.Run();

	BcastMsg::Register();
	SyncBcastMsg::Register();

	if (mpiRank == 0)
	{
    BcastMsg m;
    
    for (int i = 0; i < 4; i++)
    {
      bool is_collective = i & 0x1;
      bool is_blocking   = i & 0x2;
      std::cerr << i << " " << is_collective << " " << is_blocking << "\n";
      std::cerr << (is_collective ? "" : "NOT ") << "Collective " << 
                   (is_blocking ? "" : "NOT ") << "Blocking\n";

      m.Broadcast(is_collective, is_blocking);
      std::cerr << "rank 0 return\n";
      sleep(5);
      std::cerr << "\n";
    }

    SyncBcastMsg s;
    std::cerr << "\nBroadcast Full Blocking\n";
    s.Broadcast(true, true);
    std::cerr << "rank 0 return\n";

		theApplication.QuitApplication();
	}

	theApplication.Wait();
}
