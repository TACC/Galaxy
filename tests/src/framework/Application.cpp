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

/*! \file Application.cpp 
 * \brief unit tests for framework Application class
 * \ingroup unittest
 */


#include "Application.h"
#include "UnitTest.h"

#include <cstring>
#include <iostream>
#include <sstream>

using namespace gxy;
using namespace std;

void
syntax(char *a)
{
  cerr << "unit tests for framework/Application" << endl;
  cerr << "syntax: " << a << " [options] " << endl;
  cerr << "options:" << endl;
  cerr << "  -h, --help       this message" << endl;
  cerr << "  -w               treat warnings as errors" << endl;
  exit(1);
}

/*! unit tests for src/framework/Application */
int main(int argc, char * argv[])
{
	bool warn_as_errors = false;
	for (int i=1; i < argc; ++i)
	{
		if (!strncmp(argv[i], "-h", 2) || !strcmp(argv[i], "--help")) { syntax(argv[0]); exit(1); }
		if (!strcmp(argv[i], "-w")) { warn_as_errors = true; }
	}

	UnitTest test("framework/Application");
	test.start();

	Application theApplication(&argc, &argv);
	test.update("allocated Application object");

	theApplication.Start();
	test.update("started Application");

	int mpiRank = -1;
	int mpiSize = -1;
	stringstream message;
	mpiRank = theApplication.GetRank();
	mpiSize = theApplication.GetSize();
	message << "received MPI rank " << mpiRank << " and MPI size " << mpiSize;
	if (mpiRank < 0 || mpiSize < 0 || mpiRank >= mpiSize) 
	{
		test.error(message.str());
	}
	else 
	{
		test.update(message.str());
	}

	test.update("running Application");
	theApplication.Run();

	if (mpiRank == 0)
	{
		test.update("rank 0 quitting Application");
		theApplication.QuitApplication();
	}

	test.update("waiting for quit acks");
	theApplication.Wait();

	test.finish();

	return warn_as_errors ? test.warnings() + test.errors() : test.errors();
}