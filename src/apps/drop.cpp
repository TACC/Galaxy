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
#include <string.h>
#include <sys/types.h>

#include <Application.h>

#include "TestObject.h"

using namespace gxy;

int mpiRank, mpiSize;

using namespace std;

#include "Debug.h"


int
main(int argc, char * argv[])
{
  Application theApplication(&argc, &argv);
  theApplication.Start();

  theApplication.Run();

  TestObject::Register();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug(argv[0], false, "");

  if (theApplication.GetRank() == 0)
  {
    {
      TestObjectP top = TestObject::NewP();
      top->doit();
      std::cerr << "doit done\n";

      std::cerr << "? ";
      char c;
      std::cin >> c;

      Delete(top);
      std::cerr << "top deleted\n";
    }

    sleep(4);
    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
