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
#include <sstream>
#include <cstdio>

using namespace std;

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "KeyedObject.h"
#include "SocketConnector.hpp"
#include "Volume.h"

#include <vtkNew.h>
#include <vtkDataSetReader.h>
#include <vtkImageData.h>

namespace gxy
{

WORK_CLASS_TYPE(SocketConnector::ConnectionMsg);

void
SocketConnector::Register()
{
  RegisterClass();
  ConnectionMsg::Register();
}

KEYED_OBJECT_CLASS_TYPE(SocketConnector)

SocketConnector::~SocketConnector() {}

void
SocketConnector::initialize()
{
  super::initialize();
  variables = Datasets::NewP();
}

VolumeP
SocketConnector::findVariable(std::string name)
{
  return Volume::Cast(variables->Find(name));
}

int
SocketConnector::serialSize()
{
  return 2 * sizeof(int) + variables->serialSize();
}

unsigned char *
SocketConnector::serialize(unsigned char *p)
{
  p = super::deserialize(p);

  *(int *)p = port;
  p += sizeof(int);

  *(int *)p = wait_time;
  p += sizeof(int);

  p = variables->serialize(p);

  return p;
}

unsigned char *
SocketConnector::deserialize(unsigned char *p)
{
  p = super::deserialize(p);

  port = *(int *)p;
  p += sizeof(int);

  wait_time = *(int *)p;
  p += sizeof(int);

  p = variables->deserialize(p);

  return p;
}

void
SocketConnector::publish(DatasetsP datasets)
{
  for (auto a = variables->begin(); a != variables->end(); a++)
    if (! datasets->Find(a->first))
      datasets->Insert(a->first, a->second);
}

void
SocketConnector::commit_datasets()
{
  for (auto a = variables->begin(); a != variables->end(); a++)
    a->second->Commit();
}

bool
SocketConnector::local_commit(MPI_Comm c)
{
  local_port = port + GetTheApplication()->GetRank();
  return false;
}

bool 
SocketConnector::has(std::string name)
{
  return variables->Find(name) != NULL;
}

void
SocketConnector::addVariable(std::string name, VolumeP volume)
{
  if (! variables->Find(name))
    variables->Insert(name, volume);
}

void
SocketConnector::Open()
{
  ConnectionMsg msg(this, ConnectionMsg::Open);
  msg.Broadcast(true, false);
}

void
SocketConnector::Close()
{
  ConnectionMsg msg(this, ConnectionMsg::Close);
  msg.Broadcast(true, false);
}

bool
SocketConnector::local_open(MPI_Comm c)
{
  sskt = vtkServerSocket::New();
  sskt->CreateServer(port + GetTheApplication()->GetRank()) ;

  return false;
}

bool
SocketConnector::local_close(MPI_Comm c)
{
  if (sskt)
  {
    sskt->CloseSocket();
    sskt->Delete();
    sskt = NULL;
  }

  return false;
}

void
SocketConnector::Accept()
{
  waiter.busy = true;
  
  ConnectionMsg msg(this, ConnectionMsg::Accept);
  msg.Broadcast(true, false);
}

bool SocketConnector::local_accept(MPI_Comm c)
{
  vtkClientSocket *cskt = sskt->WaitForConnection(wait_time);

  int allflag, flag = cskt == NULL  ? 0 : 1;
  MPI_Allreduce(&flag, &allflag, 1, MPI_INT, MPI_MIN, c);

  if (allflag == 0)
    return false;

  float min, max;
  int point_count;

  struct header
  {
    int nvars;
    int ijk[3];
    int grid[3];
    int pgrid[3];
    int ghosts[6];
    int neighbors[6];
    int extent[6];
    double origin[3];
    double spacing[3];
  } hdr;

  if (! cskt->Receive(&hdr, sizeof(hdr), 1))
  {
    std::cerr << "error receiving partition header\n";
    exit(0);
  }
    
  vec3f p0, p1;

  p0.x = hdr.origin[0] + hdr.spacing[0];
  p0.y = hdr.origin[1] + hdr.spacing[1];
  p0.z = hdr.origin[2] + hdr.spacing[2];

  p1.x = p0.x + (hdr.grid[0] - 3)*hdr.spacing[0];
  p1.y = p0.y + (hdr.grid[1] - 3)*hdr.spacing[1];
  p1.z = p0.z + (hdr.grid[2] - 3)*hdr.spacing[2];

  Box gb = Box(p0, p1);

  p0.x = hdr.origin[0] + (hdr.extent[0] + hdr.ghosts[0])*hdr.spacing[0];
  p0.y = hdr.origin[1] + (hdr.extent[2] + hdr.ghosts[2])*hdr.spacing[1];
  p0.z = hdr.origin[2] + (hdr.extent[4] + hdr.ghosts[4])*hdr.spacing[2];

  p1.x = hdr.origin[0] + (hdr.extent[1] - hdr.ghosts[1])*hdr.spacing[0];
  p1.y = hdr.origin[1] + (hdr.extent[3] - hdr.ghosts[3])*hdr.spacing[1];
  p1.z = hdr.origin[2] + (hdr.extent[5] - hdr.ghosts[5])*hdr.spacing[2];

  Box lb = Box(p0, p1);

  point_count = ((hdr.extent[1] - hdr.extent[0])+1)
                * ((hdr.extent[3] - hdr.extent[2])+1)
                * ((hdr.extent[5] - hdr.extent[4])+1);

  for (auto i = 0; i < hdr.nvars; i++)
  {
    int n;
    if (! cskt->Receive((void *)&n, sizeof(n), 1))
    {
      std::cerr << "error... unable to read name size\n";
      exit(0);
    }

    char nbuf[1000];
    if (! cskt->Receive(nbuf, n, 1))
    {
      std::cerr << "error... unable to read name\n";
      exit(0);
    }

    std::string name = nbuf;

    int nc;
    if (! cskt->Receive((void *)&nc, sizeof(nc), 1))
    {
      std::cerr << "error... unable to read number of components\n";
      exit(0);
    }

    VolumeP volume = findVariable(name);
    if (! volume->get_samples())
    {
      volume->set_type(Volume::FLOAT);

      volume->set_ijk(hdr.ijk[0], hdr.ijk[1], hdr.ijk[2]);

      volume->set_global_partitions(hdr.pgrid[0], hdr.pgrid[1], hdr.pgrid[2]);
      volume->set_global_counts(hdr.grid[0], hdr.grid[1], hdr.grid[2]);

      volume->set_global_origin(hdr.origin[0], hdr.origin[1], hdr.origin[2]);
      volume->set_deltas(hdr.spacing[0], hdr.spacing[0], hdr.spacing[2]);

      volume->set_ghosted_local_offset(hdr.extent[0], hdr.extent[2], hdr.extent[4]);
      volume->set_local_offset(hdr.extent[0] + hdr.ghosts[0], 
                               hdr.extent[2] + hdr.ghosts[2], 
                               hdr.extent[4] + hdr.ghosts[4]);

      int gcounts[3] = {
        (hdr.extent[1] - hdr.extent[0]) + 1,
        (hdr.extent[3] - hdr.extent[2]) + 1,
        (hdr.extent[5] - hdr.extent[4]) + 1
      };

      int lcounts[3] = {
        gcounts[0] - (hdr.ghosts[0] + hdr.ghosts[1]),
        gcounts[1] - (hdr.ghosts[2] + hdr.ghosts[5]),
        gcounts[2] - (hdr.ghosts[4] + hdr.ghosts[5])
      };

      volume->set_ghosted_local_counts(gcounts[0], gcounts[1], gcounts[2]);
      volume->set_local_counts(lcounts[0], lcounts[1], lcounts[2]);

      volume->set_neighbors(hdr.neighbors);
      volume->set_boxes(lb, gb);

      volume->set_samples(malloc(gcounts[0] * gcounts[1] * gcounts[2] * nc * sizeof(float)));
      // volume->local_commit(c);
    }

    int ii, jj, kk;
    volume->get_ghosted_local_counts(ii, jj, kk);
    size_t sz = ii * jj * kk * nc * sizeof(float);

    if (! cskt->Receive((void *)volume->get_samples(), sz, 1))
    {
      std::cerr << "error... unable to read name size\n";
      exit(0);
    }
  }

  for (auto it = variables->begin(); it != variables->end(); it++)
  {
    VolumeP v = Volume::Cast(it->second);

    struct S
    {
      float mm, MM;
      int flag;
    } l, rcv[GetTheApplication()->GetSize()];

    l.flag = v->get_samples() != NULL;

    if (l.flag)
    {
      if (v->get_number_of_components() == 1)
      {
        float *s = (float *)v->get_samples();
        l.MM = l.mm = *s; 

        for (int i = 1; i < point_count; i++)
        {
          float d = *s++;
          if (d < l.mm) l.mm = d;
          if (d > l.MM) l.MM = d;
        }
      }
      else
      {
        float *s = (float *)v->get_samples();
        l.MM = l.mm = sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
        s += 3;

        for (int i = 1; i < point_count; i++)
        {
          float d = sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
          s += 3;
          if (d < l.mm) l.mm = d;
          if (d > l.MM) l.MM = d;
        }
      }
    }

    v->set_local_minmax(l.mm, l.MM);
    MPI_Allgather((const void *)&l, sizeof(l), MPI_CHAR, (void *)&rcv, sizeof(l), MPI_CHAR, c);

    for (int i = 0; i < GetTheApplication()->GetSize(); i++)
      if (rcv[i].flag)
      {
        if (rcv[i].mm < l.mm) l.mm = rcv[i].mm;
        if (rcv[i].MM > l.MM) l.MM = rcv[i].MM;
      }

    v->set_global_minmax(l.mm, l.MM);
  }

  cskt->CloseSocket();
  cskt->Delete();

  return false;
}

SocketConnector::ConnectionMsg::ConnectionMsg(SocketConnector* s, todo t) : ConnectionMsg(sizeof(Key) + sizeof(todo))
{
  unsigned char *p = contents->get();

  *(Key *)p = s->getkey();
  p += sizeof(Key);
  *(todo *)p = t;
}

bool SocketConnector::ConnectionMsg::CollectiveAction(MPI_Comm comm, bool is_root)
{
  unsigned char *p = contents->get();
  SocketConnectorP c = SocketConnector::GetByKey(*(Key *)p);
  p +=sizeof(Key);
  todo t = *(todo *)p;

  bool r;
  std::string s;
  
  switch(t)
  {
    case Open:
      r = c->local_open(comm);
      s = "open";
      break;
    case Close:
      r = c->local_close(comm);
      s = "close";
      break;
    case Accept:
      r = c->local_accept(comm);
      s = "accept";
      break;
  }

  /*
  This looks weird.   Here's the thing: when doing an async transfer,
  the sim main loop doesn't wait for the Accept to complete; this is so
  a single sim flow of control can tell Galaxy to accept a timestep then
  immediately send it. For example, the sim loop MPI process might look like:

  while (doing timesteps) {
    compute time step synchronize

    if (root process)
      tell Galaxy to accept a timestep

    send the time step
  }

  So consider the ownership of the connection itself.  Its initially
  owned by the master thread.   Then, while the transfer is underway,
  its also owned by various bits of the communications infrastructure,
  including a local reference in the CollectiveAction that is doing
  the transfer.   Since thats collective (so that the global
  properties of the updated object can be gathered) it occurs
  directly in the message loop.   Suppose that ownership is the
  last reference on the connection; when the CollectiveAction
  completes and removes its local reference, the object will get
  deleted.

  Problem is that the deletion itself issues a collective operation
  (causing all the nodes to drop their bit of the object).   This
  will deadlock since that will never get picked up by the message
  loop (since the message loop will be waiting for it to complete).

  We need to require the explicit wait for the completion of the
  transfer so that the master doesn't remove its reference on the
  object until the transfer completes and the CollectiveAction
  reference to it has been removed.   Thus the final reference will
  be removed outside the message loop and it won't deadlock.   This
  makes the calling code look more like:

  while (doing timesteps) {
    compute time step synchronize

    if (root process)
      tell Galaxy to accept a timestep

    send the time step

    if (root process)
      wait for Galaxy to receive the time step
  }

  This also enables the caller to wait for the transfer to complete
  so that it can single buffer.

  */

  Waiter *waiter = &c->waiter;

  c = NULL;
  
  if (is_root)
  {
    pthread_mutex_lock(&waiter->lock);
    waiter->busy = false;
    pthread_cond_signal(&waiter->cond);
    pthread_mutex_unlock(&waiter->lock);
  }

  return r;
}

}
