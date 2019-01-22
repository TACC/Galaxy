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
#include <vector>
#include <sstream>
#include <regex>

using namespace std;

#include <string.h>
#include <dlfcn.h>

#include "Application.h"
#include "MultiServer.h"
#include "MultiServerHandler.h"

using namespace gxy;

int mpiRank, mpiSize;

#include "Debug.h"

void
syntax(char *a)
{
  if (mpiRank == 0)
  {
    cerr << "syntax: " << a << " [options]" << endl;
    cerr << "options:" << endl;
    cerr << "  -D         run debugger" << endl;
    cerr << "  -A         wait for attachment" << endl;
    cerr << "  -P port    port to use (5001)" << endl;
  }
  MPI_Finalize();
  exit(1);
}

int 
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
  char *dbgarg;
  int port = 5001;

  Application theApplication(&argc, &argv);
  
  theApplication.Start();

  mpiRank = GetTheApplication()->GetRank();
  mpiSize = GetTheApplication()->GetSize();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strncmp(argv[i], "-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
    else
      syntax(argv[0]);
  }

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  GetTheApplication()->Run();

  MultiServer::NewP();

  if (mpiRank == 0)
  {
    MultiServer::Get()->Start(port);

    string cmd = "", tmp;
    cerr << "? ";
    while (getline(cin, tmp))
    {
      size_t n = tmp.find(";");
      if (n != string::npos)
      {
        tmp.resize(n);
        cmd = cmd + tmp;

        regex r("^\\W*q");
        smatch sm;
        if (regex_search(cmd, sm, r))
          break;

        if (cmd.c_str()[0] == 'q')
          break;
        else if (cmd == "help")
          cout << "control-D, q or quit to quit\n";
        else 
          cerr << "huh? ";
      }
      else
        cmd = cmd + tmp;
    }

    GetTheApplication()->QuitApplication();
  }
  else
  {
    GetTheApplication()->Wait();
  }
}
