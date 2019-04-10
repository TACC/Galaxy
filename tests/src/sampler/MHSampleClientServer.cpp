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

/*! \file MHSampleClientServer.cpp 
 * \brief unit tests for sampler MHSampleClientServer class
 * \ingroup unittest
 */


#include "MHSampleClientServer.h"
#include "UnitTest.h"

#include <cstring>
#include <iostream>
#include <sstream>

using namespace gxy;
using namespace std;

void
syntax(char *a)
{
  cerr << "unit tests for sampler/MHSampleClientServer" << endl;
  cerr << "syntax: " << a << " [options] " << endl;
  cerr << "options:" << endl;
  cerr << "  -h, --help       this message" << endl;
  cerr << "  -w               treat warnings as errors" << endl;
  exit(1);
}

/*! unit tests for src/sampler/MHSampleClientServer */
int main(int argc, char * argv[])
{
	bool warn_as_errors = false;
	for (int i=1; i < argc; ++i)
	{
		if (!strncmp(argv[i], "-h", 2) || !strcmp(argv[i], "--help")) { syntax(argv[0]); exit(1); }
		if (!strcmp(argv[i], "-w")) { warn_as_errors = true; }
	}

	UnitTest test("sampler/MHSampleClientServer");
	test.start();

	// TODO: add unit tests

	test.finish();

	return warn_as_errors ? test.warnings() + test.errors() : test.errors();
}
