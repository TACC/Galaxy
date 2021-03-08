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
#include <random>
#include <climits>
#include <memory>

using namespace std;

#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h>
#include <time.h> 
#include <pthread.h> 

#include "Skt.hpp"
#include "bufhdr.h"

using namespace gxy;

float xmin = -1.0, xmax = 1.0, ymin = -1.0, ymax = 1.0, zmin = -1.0, zmax = 1.0;
int nPer = 100;
int n_senders = 1;
int running_count;
float fuzz = 0.0;

int mpi_rank = 0, mpi_size = 1;

void
syntax(char *a)
{
  std::cerr << "syntax: " << a << " [options]\n";
  std::cerr << "options:\n";
  std::cerr << " -b box      a file containing six space separated values: xmin xmax ymin ymax zmin zmax (-1 1 -1 1 -1 1)\n";
  std::cerr << " -h hostfile a file containing a host name for each receiver process (localhost)\n";
  std::cerr << " -p npart    number of particles to send to each receiver (100)\n";
  std::cerr << " -n nproc    number of senders to run (1)\n";
  std::cerr << " -f fuzz     max radius of points - creates empty border region (0)\n";
  
  exit(1);
}

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

enum
{
  CREATE_SOME_DATA, SEND_SOME_DATA
} command;

class SenderThread
{
  static void *sender_thread(void *d)
  {
    SenderThread *me = (SenderThread *)d;

    if (command == SEND_SOME_DATA)
      me->SendSomeData();
    else 
      me->CreateSomeData();

    pthread_exit(NULL);
    return NULL;
  }
    
public:
  SenderThread(int n, string host, int port) : sender_id(n), host(host), port(port)
  {
    long seed = getpid() + rand();
    std::cerr << "seed: " << seed << "\n";
    mt_rand.seed(seed);
  }

  ~SenderThread()
  {
    if (tid != NULL)
    {
      pthread_join(tid, NULL);
      tid = NULL;
    }
  }

  void
  Run()
  {
    pthread_create(&tid, NULL, sender_thread, (void *)this);
  }

  void
  Wait()
  {
    pthread_join(tid, NULL);
    tid = NULL;
  }

private:
  void
  CreateSomeData()
  {
    if (dsz == -1)
    {
      dsz = sizeof(int) + sizeof(bufhdr)+ nPer*4*sizeof(float);
      data = (char *)malloc(dsz);
    }

    *(int *)data = dsz;

    bufhdr *hdr = (bufhdr *)(data + sizeof(int));
    float *ptr =   (float *)(data + sizeof(int) + sizeof(bufhdr));

    hdr->type          = bufhdr::Particles;
    hdr->origin        = sender_id;
    hdr->has_data      = true;
    hdr->npoints       = nPer;
    hdr->nconnectivity = 0;

    std::cerr << xmin << " -> " << xmax << "\n";
    std::cerr << ymin << " -> " << ymax << "\n";
    std::cerr << zmin << " -> " << zmax << "\n";

    for (int i = 0; i < nPer; i++)
    {
      *ptr++ = xmin + frand()*(xmax - xmin);
      *ptr++ = ymin + frand()*(ymax - ymin);
      *ptr++ = zmin + frand()*(zmax - zmin);
    }

    // float v = (n_senders == 1) ? 0.0 : float(sender_id) / float(n_senders - 1);
      
    for (int i = 0; i < nPer; i++)
      *ptr++ = sender_id;
  }

  bool
  SendSomeData()
  {
    std::cerr << "send data to " << host << "::" << port << "\n";
#if 0
    return true;
#else
    ClientSkt c(host, port);
    if (c.Connect())
    {
      c.Send(data, dsz);
      char *rply = c.Receive();
      int r = *(int *)rply;
      c.Disconnect();
      free(rply);

      return r == 1;
    }
    else
      return false;
#endif
  }

  float
  frand()
  {
    return float(mt_rand()) / UINT_MAX;
  }

private:
  mt19937 mt_rand;
  pthread_t tid;
  int sender_id;
  int dsz = -1;
  char *data = NULL;
  int frame = 0;
  string host;
  int port;
};

vector< shared_ptr<SenderThread> > senders;

void
SendSomeData()
{
  command = SEND_SOME_DATA;

  for (auto s : senders)
    s->Run();

  for (auto s : senders)
    s->Wait();
}

void
CreateSomeData()
{
  command = CREATE_SOME_DATA;

  for (auto s : senders)
    s->Run();

  for (auto s : senders)
    s->Wait();
}

int
main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  vector<string> hosts;
  hosts.push_back("localhost");

  for (int i = 1; i < argc; i++)
    if (! strcmp(argv[i], "-b"))
    {
      ifstream ifs(argv[++i]);
      ifs >> xmin >> xmax >> ymin >> ymax >> zmin >> zmax;
      ifs.close();
    }
    else if (! strcmp(argv[i], "-h"))
    {
      hosts.clear();
      ifstream ifs(argv[++i]);
      string s;
      while (ifs >> s)
        hosts.push_back(s);
      ifs.close();
    }
    else if (! strcmp(argv[i], "-p"))
    {
      nPer = atoi(argv[++i]);
    }
    else if (! strcmp(argv[i], "-n"))
    {
      n_senders = atoi(argv[++i]);
    }
    else if (! strcmp(argv[i], "-f"))
    {
      fuzz = atof(argv[++i]);
    }
    else
      syntax(argv[0]);

  xmin = xmin + fuzz;
  ymin = ymin + fuzz;
  zmin = zmin + fuzz;
  xmax = xmax - fuzz;
  ymax = ymax - fuzz;
  zmax = zmax - fuzz;

  int senders_per_rank = n_senders / mpi_size;
  int sender_start = mpi_rank * senders_per_rank;
  int sender_end   = (mpi_rank == mpi_size-1) ? n_senders : (mpi_rank+1) * senders_per_rank;

  for (int i = sender_start;  i < sender_end; i++)
  {
    int host_id = i % hosts.size();
    senders.push_back(shared_ptr<SenderThread>(new SenderThread(i, hosts[host_id], 1900 + host_id)));
  }

  CreateSomeData();

  bool done = false;
  char cmd = 's';

  while (! done)
  {
    if (cmd == 's')
      SendSomeData();
    else if (cmd == 'c')
      CreateSomeData();
    else if (cmd == 'S')
    {
      CreateSomeData();
      SendSomeData();
    }

    if (mpi_rank == 0)
    {
      cout << "? ";
      cin >> cmd;
      if (cin.eof())
          cmd = 'q';
    }

    MPI_Bcast(&cmd, 1, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (cmd == 'q')
      done = true;
  }
  
  MPI_Finalize();
  exit(0);
}
