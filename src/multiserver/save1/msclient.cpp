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
#include <regex>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "MultiServerHandler.h"

using namespace gxy;
using namespace std;

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] statefile" << endl;
  cerr << "options:" << endl;
  cerr << "  -H host          host (localhost)" << endl;
  cerr << "  -P port          port (5001)" << endl;
  cerr << "  -so sofile       interface SO (libgxy_module_ping.so)\n";
  exit(1);
}

int
main(int argc, char *argv[])
{

  bool dbg = false, atch = false;
  string host = "localhost";
  int port = 5001;

  string sofile = "libgxy_module_ping.so";

  char *dbgarg;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-H")) host = argv[++i];
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
    else if (!strncmp(argv[i], "-D", 2)) { dbg = true, dbgarg = argv[i] + 2; break; }
    else if (!strcmp(argv[i], "-so")) { sofile = argv[++i]; }
    else
    { syntax(argv[0]); }
  }

  MultiServerHandler msh(host.c_str(), port);

  string rply;

  if (! msh.CSendRecv(sofile))
  {
    cerr << "Sending sofile failed\n";
    exit(1);
  }

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

      if (! msh.CSendRecv(cmd))
      {
        cerr << "send/receive failed\n";
        exit(1);
      }

      if (cmd != "ok")
        cout << "reply: " << cmd << "\n";

      cmd = "";
      cerr << "? ";
    }
    else
      cmd = cmd + tmp;
  }

  return 0;
}
