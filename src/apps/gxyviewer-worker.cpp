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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "Application.h"
#include "Renderer.h"
#include "AsyncRendering.h"

using namespace gxy;
using namespace std;

int mpiRank, mpiSize;

#include "Debug.h"

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options]" << endl;
  cerr << "options:" << endl;
  cerr << "  -D         run debugger" << endl;
  cerr << "  -A         wait for attachment" << endl;
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
	char *dbgarg;

  Application theApplication(&argc, &argv);
  theApplication.Start();

  AsyncRendering::RegisterClass();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) { dbg = true, atch = true; }
    else if (!strcmp(argv[i], "-D")) { dbg = true, dbgarg = argv[i] + 2; break; }
		else { syntax(argv[0]); }
	}


  Renderer::Initialize();
  GetTheApplication()->Run();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

	GetTheApplication()->Wait();

  return 0;
}
