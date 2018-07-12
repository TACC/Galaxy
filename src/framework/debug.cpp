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

#include <string>
#include "Application.h"

using namespace std;

namespace gxy
{
static string executable;

void setup_debugger(char *e)
{
	executable = e;
}

static bool debugger_running = false;

void debugger(char *arg)
{
	if (arg == NULL)
		return;

  int rank = GetTheApplication()->GetRank();

  auto c = arg;
  bool done = false, dbg = *c == 0;
  while (!done && !dbg)
  {
    auto d = c;
    while (*d && *d != ',') d++;
    done = (*d == 0);
		auto r = atoi(c);
		dbg = ((r == -1) || (r == rank));
    c = d + 1;
  }

  if (dbg)
	{
		if (! debugger_running)
		{
			std::stringstream cmd;
			pid_t pid = GetTheApplication()->get_pid();

			cmd << "~/dbg_script " << executable << " " << pid << " " << GetTheApplication()->GetRank() << " &";
			std::cerr << "running command: " << cmd.str() << "\n";
			system(cmd.str().c_str());
		}

		int dbg = 1;
		while (dbg)
			sleep(1);
	}
}

}