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
#include <string>
#include <sstream>

using namespace std;

class Debug
{
public:
  Debug(const char *executable, bool attach)
  {
    cerr << "Debug ctor" << endl;
    bool dbg = true;
    std::stringstream cmd;
    pid_t pid = getpid();

    if (attach)
      std::cerr << "Attach to PID " << pid << endl;
    else
    {
      cmd << "~/dbg_script " << executable << " " << pid << " &";
      system(cmd.str().c_str());
    }

    while (dbg)
      sleep(1);

    std::cerr << "running\n";
  }
};
