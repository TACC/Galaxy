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
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>


#include <dtypes.h>
#include <Application.h>
#include <Events.h>

using namespace std;
using namespace pvol;

class BcastEvent : public Event
{ 
public:
 BcastEvent() {}

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "BCAST";
  }
};

class BcastMsg : public Work
{
public:
	BcastMsg() : BcastMsg(10*sizeof(int)) 
	{
		int *p = (int *)contents->get();
		for (int i = 0; i < 10; i++) *p++ = i + GetTheApplication()->GetRank();
	}
	~BcastMsg() {}

	WORK_CLASS(BcastMsg, true)

public:
	bool CollectiveAction(MPI_Comm s, bool isRoot)
	{
		GetTheEventTracker()->Add(new BcastEvent);
		stringstream xx;
		for (int i = 0; i < 10; i++)
			xx << i << " ";
		APP_PRINT(<< xx.str());
		return false;
	}
};

WORK_CLASS_TYPE(BcastMsg)

int
main(int argc, char * argv[])
{
	Application theApplication(&argc, &argv);
	theApplication.Start();

  setup_debugger(argv[0]);
  debugger(argv[1]);

	theApplication.Run();

	BcastMsg::Register();

	int r = theApplication.GetRank();
	int s = theApplication.GetSize();

	if (r == 0)
	{
		BcastMsg b0;
		b0.Broadcast(true, true);
		sleep(100000);
		theApplication.QuitApplication();
	}
	else
	{
		stringstream xx;
		xx << "hello " << GetTheApplication()->GetRank();
		APP_PRINT(<< xx.str());
	}

	theApplication.Wait();
}
