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

#pragma once

/*! \file Debug.h 
 * \brief helper class to attach a debugger to a Galaxy process
 * \ingroup framework
 */

#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

//! helper class to attach a debugger to a Galaxy process
/*! \ingroup framework */
class Debug
{
public:
  Debug(const char *executable, bool attach, const char *arg) : Debug(executable, attach, const_cast<char*>(arg)) {}

  Debug(const char *executable, bool attach, char *arg)
  { 
    bool dbg = true;
    std::stringstream cmd;
    pid_t pid = getpid();
		bool do_me, first = true;

    if (! *arg)
      do_me = true;
    else
    {
      do_me = false;
      while (! do_me && *arg)
      {
        char *narg = NULL;

        int i; 
        for (i = 0; i < strlen(arg); i++)
          if (arg[i] == ',')
          {
            narg = arg + i + 1;
            arg[i] = '\0';
            break;
          }
        
        for (i = 0; i < strlen(arg); i++)
          if (arg[i] == '-')
            break;
        
        if (i < strlen(arg))
        { 
          arg[i] = 0;
          int s = atoi(arg); 
          int e = atoi(arg + i + 1);
          if ((mpiRank >= s) && (mpiRank <= e))
            do_me = true;
        }
        else
          do_me = (mpiRank == atoi(arg));
    
        if (! narg) break;
        arg = narg;
      }
    }

    if (do_me)
    { 
      if (attach)
        cerr << "Attach to PID " << pid << endl;
      else
      { 
				char scriptname[1024]; bool found = false;

				char *home = getenv("HOME");
				if (home)
				{
					sprintf(scriptname, "%s/dbg_script", home);
					struct stat buffer;   
					found = stat(scriptname, &buffer) == 0; 
				}

				if (! found)
				{
					char *galaxy = getenv("GALAXY_INSTALL");
					if (galaxy)
					{
						sprintf(scriptname, "%s/scripts/dbg_script", galaxy);
						struct stat buffer;   
						found = stat(scriptname, &buffer) == 0; 
					}
				}

				if (found)
				{
					cmd << scriptname << " " << executable << " " << pid << " &";
					system(cmd.str().c_str());
					while (dbg)
						sleep(1);
				}
				else 
				{
					cerr << "no debug script found, can't debug\n";
					do_me = false;
				}
      }
    }
  }
};

