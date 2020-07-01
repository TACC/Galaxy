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

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>


#include "dtypes.h"
#include "Application.h"
#include "Events.h"

using namespace gxy;
using namespace std;

int mpiRank;
int mpiSize;

#include "Debug.h"

class BcastEvent : public Event
{ 
public:
 BcastEvent(int t) : typ(t) {}

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << ((typ == 0) ? "BCAST COLLECTIVE" : "BCAST ASYNC");
  }

  int typ;
};

class BcastCollectiveMsg : public Work
{
public:
	BcastCollectiveMsg(std::string m) : BcastCollectiveMsg(m.size() + 1) { strcpy((char *)get(), m.c_str()); }
	~BcastCollectiveMsg() {}

	WORK_CLASS(BcastCollectiveMsg, true)

public:
	bool CollectiveAction(MPI_Comm s, bool isRoot)
	{
    MPI_Barrier(s);
		GetTheEventTracker()->Add(new BcastEvent(0));
		APP_PRINT(<< (char *)get());
		return false;
	}
};

WORK_CLASS_TYPE(BcastCollectiveMsg)

class BcastAsyncMsg : public Work
{
public:
	BcastAsyncMsg(std::string m) : BcastAsyncMsg(m.size() + 1) { strcpy((char *)get(), m.c_str()); }
	~BcastAsyncMsg() {}

	WORK_CLASS(BcastAsyncMsg, true)

public:
	bool Action(int s)
	{
		GetTheEventTracker()->Add(new BcastEvent(1));
		APP_PRINT(<< (char *)get());
		return false;
	}
};

WORK_CLASS_TYPE(BcastAsyncMsg)

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] " << endl;
  cerr << "optons:" << endl;
  cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  exit(1);
}


int
main(int argc, char * argv[])
{
  bool dbg = false;
  char *dbgarg;

	Application theApplication(&argc, &argv);
	theApplication.Start();

  for (int i = 1; i < argc; i++)
  {
    if (!strncmp(argv[i],"-D", 2)) dbg = true, dbgarg = argv[i] + 2;
    else syntax(argv[0]);
  }

	theApplication.Run();

	BcastCollectiveMsg::Register();
	BcastAsyncMsg::Register();

	mpiRank = theApplication.GetRank();
	mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

	if (mpiRank == 0)
	{
		BcastCollectiveMsg b0("collective");
		b0.Broadcast(true, true);

    BcastAsyncMsg *b1 = new BcastAsyncMsg("async");
		b1->Broadcast(false, true);
    delete b1;

    sleep(1);

		theApplication.QuitApplication();
	}
  else
    theApplication.Wait();
}
