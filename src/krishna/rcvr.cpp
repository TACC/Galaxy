// ========================================================================== //
//                                                                            //
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

#include <string>

using namespace std;

#include "Application.h"
#include "data.h"

#include "Receiver.hpp"

using namespace gxy;

int mpiRank, mpiSize;

#include "Debug.h"

void
syntax(char *a)
{
  if (GetTheApplication()->GetRank() == 0)
    std::cerr << "syntax: partfile nsenders\n";

  exit(1);
}

int
main(int argc, char *argv[])
{
  Application theApplication(&argc, &argv);
  theApplication.Start();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  std::string pfile = "";
  int nsenders = -1;

  bool dbg = false; char *dbgarg;

  for (int i = 1; i < argc; i++)
    if (! strncmp(argv[i], "-D", 2)) { dbg = true; dbgarg = argv[i] + 2; }
    else if (pfile == "") pfile = argv[i];
    else if (nsenders == -1) nsenders = atoi(argv[i]);
    else
      syntax(argv[0]);

  if (pfile == "" || nsenders == -1)
      syntax(argv[0]);

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  theApplication.Run();

  KeyedDataObject::Register();
  Partitioning::Register();
  Receiver::Register();

  if (mpiRank == 0)
  {

    ParticlesP particles = Particles::NewP();
    particles->Import(pfile);
    particles->Commit();


    PartitioningP partitioning = Partitioning::NewP();
    partitioning->Gather(particles);
    partitioning->Commit();

    ReceiverP  receiver  = Receiver::NewP();
    receiver->SetGeometry(particles);
    receiver->SetPartitioning(partitioning);
    receiver->SetNSenders(nsenders);
    receiver->SetBasePort(1900);
    receiver->Commit();

    receiver->Start();

    int done = 0;
    while (! done)
    {
      char cmd;
      cout << "? ";
      cin >> cmd;
      if (cmd == 'q' || cin.eof())
        done = 1;
    }

    receiver->Stop();

    Delete(receiver);
    Delete(particles);

    theApplication.QuitApplication();
  }
  else
    theApplication.Wait();
}
