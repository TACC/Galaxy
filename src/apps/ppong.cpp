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

// #include <galaxy.h>
#include <dtypes.h>
#include <Application.h>

#include <pthread.h>

using namespace gxy;

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

#define LOGGING 0

class PPongMsg : public Work
{
public:
	PPongMsg() : PPongMsg(100) { strcpy((char *)get(), (char *)"ppong"); /* std::cerr << "PPongMsg ctor\n"; */}
	~PPongMsg(){ /* std::cerr << GetTheApplication()->GetRank() << ": PPongMsg dtor\n"; */}
	WORK_CLASS(PPongMsg, true)

public:
	bool Action(int s)
	{
		int rank = GetTheApplication()->GetRank();
		int size = GetTheApplication()->GetSize();

		if (rank != 0)
		{ 
			PPongMsg m;
			m.Send((rank == (size-1)) ? 0 : rank + 1);
		}
		else
		{
			std::cerr << "ppong signalling\n";
			pthread_mutex_lock(&lck);
			pthread_cond_signal(&w8);
			pthread_mutex_unlock(&lck);
		}

#if LOGGING
		APP_LOG(<< rank << ": ppong");
#endif

		return false;
	}
};

class BcastMsg : public Work
{
public:
	BcastMsg() : BcastMsg(101) { strcpy((char *)get(), (char *)"bcast"); /* std::cerr << "BcastMsg ctor\n"; */ }
	~BcastMsg(){ /* std::cerr << GetTheApplication()->GetRank() << ": BcastMsg dtor\n"; */ }
	WORK_CLASS(BcastMsg, true)

public:
	bool CollectiveAction(MPI_Comm s, bool isRoot) { std:cerr << "bcast action\n"; return false;}
};

WORK_CLASS_TYPE(PPongMsg)
WORK_CLASS_TYPE(BcastMsg)

int
main(int argc, char * argv[])
{
	Application theApplication(&argc, &argv);
	theApplication.Start();

  setup_debugger(argv[0]);
  debugger(argv[1]);

	theApplication.Run();

	PPongMsg::Register();
	BcastMsg::Register();

	int r = theApplication.GetRank();
	int s = theApplication.GetSize();

	if (r == 0)
	{

		pthread_mutex_lock(&lck);

		if (s > 1)
		{
			PPongMsg m;
			m.Send(1);

			pthread_cond_wait(&w8, &lck);
			std::cerr << "lock signalled\n";
			pthread_mutex_unlock(&lck);
		}

		BcastMsg b0;
		b0.Broadcast(true);

		BcastMsg *b1 = new BcastMsg;
		b1->Broadcast(true);
		delete b1;

		if (s > 1)
		{
			PPongMsg m;
			m.Send(1);

			pthread_cond_wait(&w8, &lck);
			std::cerr << "lock signalled\n";
			pthread_mutex_unlock(&lck);
		}

		theApplication.QuitApplication();
	}

	theApplication.Wait();
}
