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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <dtypes.h>
#include <Application.h>

#include <pthread.h>

using namespace gxy;
using namespace std;

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

class PPongMsg : public Work
{
public:
	PPongMsg(std::string m) : PPongMsg(m.size() + 1) { strcpy((char *)get(), m.c_str()); }
	~PPongMsg() { std::cerr << "pp dtor\n"; }
	WORK_CLASS(PPongMsg, true)

public:
	bool Action(int s)
	{
		int rank = GetTheApplication()->GetRank();
		int size = GetTheApplication()->GetSize();

		if (rank != 0)
		{ 
			PPongMsg m("ppong");
			m.Send((rank == (size-1)) ? 0 : rank + 1);
		}
		else
		{
			std::cerr << "ppong signalling\n";
			pthread_mutex_lock(&lck);
			pthread_cond_signal(&w8);
			pthread_mutex_unlock(&lck);
		}

#ifdef GXY_LOGGING
		APP_LOG(<< rank << ": ppong");
#endif

		return false;
	}
};

class BcastMsg : public Work
{
public:
	BcastMsg(std::string m) : BcastMsg(m.size() + 1) { strcpy((char *)get(), m.c_str()); }
	~BcastMsg() {}
	WORK_CLASS(BcastMsg, true)

public:
	bool CollectiveAction(MPI_Comm s, bool isRoot) { std::cerr << "bcast action" << std::endl; return false;}
};

WORK_CLASS_TYPE(PPongMsg)
WORK_CLASS_TYPE(BcastMsg)


int mpiRank = 0, mpiSize = 1;
#include "Debug.h"

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

	PPongMsg::Register();
	BcastMsg::Register();

	if (mpiRank == 0)
	{

		pthread_mutex_lock(&lck);

		if (mpiSize > 1)
		{
			PPongMsg *m = new PPongMsg("hello");
			m->Send(1);
      delete m;

			pthread_cond_wait(&w8, &lck);
			std::cerr << "lock signalled" << std::endl;
			pthread_mutex_unlock(&lck);
		}

		BcastMsg b0("bcast");
		b0.Broadcast(true, true);

		BcastMsg *b1 = new BcastMsg;
		b1->Broadcast(true, true);
		delete b1;

		if (mpiSize > 1)
		{
			PPongMsg m("goodbye");
			m.Send(1);

			pthread_cond_wait(&w8, &lck);
			std::cerr << "lock signalled" << std::endl;
			pthread_mutex_unlock(&lck);
		}

		theApplication.QuitApplication();
	}

	theApplication.Wait();
  std::cerr << "done\n";
}
