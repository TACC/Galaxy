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
#include <iostream>

#include "Application.h"
#include "Events.h"

int mpiRank, mpiSize;

using namespace std;

#include "Debug.h"

using namespace gxy;

class TestEvent : public Event
{
public:
 TestEvent(string s) : msg(s) {}

protected:
  void print(std::ostream& o)
  {
    Event::print(o);
    o << "TestEvent: " << msg;
  }

private:
	string msg;
};

class TestEventMsg : public Work
{
public:
	TestEventMsg(string s) : TestEventMsg(s.length() + 1)
	{
		strcpy((char *)contents->get(), s.c_str());
	}

	WORK_CLASS(TestEventMsg, true);

public:

	bool CollectiveAction(MPI_Comm coll_comm, bool isRoot)
	{
		char *p = (char *)contents->get();
		sleep(GetTheApplication()->GetRank());
		GetTheEventTracker()->Add(new TestEvent(p));
		return false;
	}
};

WORK_CLASS_TYPE(TestEventMsg)

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] data" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D[which]           run debugger" << endl;
}

int
main(int argc, char *argv[])
{
  char *dbgarg;
  bool dbg = false;

	Application theApplication(&argc, &argv);
  theApplication.Start();

	for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
      switch (argv[i][1])
      {
        case 'D': dbg = true, dbgarg = argv[i] + 2; break;
        default:
          syntax(argv[0]);
      }
    else syntax(argv[0]);
  }

  theApplication.Run();

  TestEventMsg::Register();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

	Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

	if (mpiRank == 0)
	{
		TestEventMsg t(string("hello"));
		t.Broadcast(true, true);

		theApplication.QuitApplication();
	}

	theApplication.Wait();
	return 0;
}

