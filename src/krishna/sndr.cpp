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

float xmin, xmax, ymin, ymax, zmin, zmax;

void
syntax(char *a)
{
  if (mpi_rank == 0)
  {
    std::cerr << "syntax: " << a << " box hostfile baseport\n";
    std::cerr << "box is a file containing six space separated values: xmin xmax ymin ymax  zmin, zmax\n";
    std::cerr << "hostfile is a file containing the hosts that are in play on the Galaxy side\n";
    std::cerr << "The i'th host will be listening on port (baseport + i)\n";
    std::cerr << "baseport is the port being listened to on the rank-0 Galaxy server\n";
  }

  MPI_Finalize();
  exit(1);
}


static int frame = 0;

inline float frand() { return float(rand()) / RAND_MAX; }

bool
SendSomeData(string destination, int port)
{
  if (frame == 0)
    srand((unsigned int) getpid());
  
  ClientSkt c(destination, port);
  if (c.Connect())
  {
    int dsz = sizeof(float) + 100*4*sizeof(float);
    char *data = (char *)malloc(dsz);

    *(int *)data = 100;

    float *ptr = (float *)(data + sizeof(int));

    for (int i = 0; i < 100; i++)
    {
      *ptr++ = xmin + frand()*(xmax - xmin);
      *ptr++ = ymin + frand()*(ymax - ymin);
      *ptr++ = zmin + frand()*(zmax - zmin);
    }

    for (int i = 0; i < 100; i++)
      *ptr++ = frand();

    c.Send(data, dsz);
    char *rply = c.Receive();
    int r = *(int *)rply;
    c.Disconnect();

    free(data);
    free(rply);

    return r == 1;
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

  if (argc != 4)
    syntax(argv[0]);

  vector<string> gxy_hosts;
  ifstream ifs(argv[2]);
  string s;
  while (ifs >> s)
    gxy_hosts.push_back(s);
  ifs.close();
  
  ifs.open(argv[1]);
  ifs >> xmin >> xmax >> ymin >> ymax >> zmin >> zmax;
  ifs.close();

  string destination = gxy_hosts[mpi_rank % gxy_hosts.size()];
  int port = atoi(argv[3]) + (mpi_rank % gxy_hosts.size());

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
