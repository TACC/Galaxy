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

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#include "Skt.hpp"

using namespace gxy;

int mpi_rank, mpi_size;

void
syntax(char *a)
{
  if (mpi_rank == 0) std::cerr << "syntax: " << a << " hostfile baseport\n";
  MPI_Finalize();
  exit(1);
}


static int frame = 0;

bool
SendSomeData(string destination, int port)
{
#if 0
  if (mpi_rank == 1)
  {
    std::cerr << getpid() << "\n";
    int dbg = 1;
    while(dbg)
      sleep(1);
  }
#endif
    
  ClientSkt c(destination, port);
  if (c.Connect())
  {
    int msg[2] = {mpi_rank, frame++};
    c.Send((char *)&msg, sizeof(msg));
    std::cerr << mpi_rank << ": send data\n";
    char *buf = c.Receive();
    int rply = *(int *)buf;
    std::cerr << mpi_rank << ": got " << rply << "\n";
    c.Disconnect();
    return rply == 1;
  }
  else
    return false;
}

int
main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  if (argc != 3)
    syntax(argv[0]);

  vector<string> gxy_hosts;
  ifstream ifs(argv[1]);

  string s;
  while (ifs >> s)
    gxy_hosts.push_back(s);

  string destination = gxy_hosts[mpi_rank % gxy_hosts.size()];
  int port = atoi(argv[2]) + (mpi_rank % gxy_hosts.size());

  std::cerr << mpi_rank << ": " << port << "\n";

  int done = 0;
  char cmd = 's';

  while (! done)
  {
    if (cmd == 's')
      SendSomeData(destination, port);
    else
      std::cerr << mpi_rank << " " << destination << " " << port << "\n";

    if (mpi_rank == 0)
    {
      cout << "? ";
      cin >> cmd;
      if (cin.eof())
        cmd = 'q';

      std::cerr << "rank 0: cmd " << cmd << "\n";
    }

    MPI_Bcast(&cmd, 1, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (cmd == 'q')
      done = 1;
  }
  
  MPI_Finalize();
  exit(0);
}
