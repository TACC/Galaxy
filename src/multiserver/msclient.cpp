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

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "SocketHandler.h"
#include "CommandLine.h"

using namespace gxy;
using namespace std;

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options]" << endl;
  cerr << "options:" << endl;
  cerr << "  -H host          host (localhost or GXY_HOST)" << endl;
  cerr << "  -P port          port (5001)" << endl;
  cerr << "  -so sofile       interface SO (libgxy_module_ping.so)\n";
  cerr << "  file ...         optional list of files of commands\n";
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;

  int port = 5001;
  char *file = NULL;
  vector<string> files;

  string host = (getenv("GXY_HOST") != NULL) ? getenv("GXY_HOST") : "localhost";

  string sofile = "libgxy_module_ping.so";

  char *dbgarg;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-H")) host = argv[++i];
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
    else if (!strncmp(argv[i], "-D", 2)) { dbg = true, dbgarg = argv[i] + 2; break; }
    else if (!strcmp(argv[i], "-so")) { sofile = argv[++i]; }
    else if (!strcmp(argv[i], "-h")) syntax(argv[0]);
    else files.push_back(argv[i]);
  }

  SocketHandler *sh = new SocketHandler();

  if (! sh->Connect(host.c_str(), port))
  {
    cerr << "Unable to connect to Galaxy server\n";
    exit(1);
  }

  string loadcmd = string("load ") + sofile;
  if (! sh->CSendRecv(loadcmd))
  {
    cerr << "Sending sofile failed\n";
    exit(1);
  }
  
  CommandLine cmd;
  
  if (files.size() > 0)
  {
    for (auto i : files)
      cmd.Run(i, sh);
  }
  else
    cmd.Run(sh);
    
  if (sh) delete sh;

  return 0;
}
